/*
 * csvs - Query: plan and execute tablet-based queries
 *
 * Translates Rust's async stream-based query into a synchronous
 * coroutine-style state machine. Each query tablet is a sorted CSV
 * file; the query executor does a nested-loop join across tablets.
 */

#include "csvs_internal.h"
#include <regex.h>
#include <sys/stat.h>

/* ── Strategy: plan_query ────────────────────────────────────────── */

/* Gather all leaf keys from a query entry (recursive) */
static void gather_keys(const csvs_entry *q, char ***out, size_t *nout, size_t *cap)
{
    for (size_t i = 0; i < q->nleaves; i++) {
        const csvs_leaf *lf = &q->leaves[i];
        const char *name = lf->name;

        /* skip if same as base_value (when base_value acts as a key) */
        if (q->base_value && strcmp(name, q->base_value) == 0) continue;

        /* add the leaf name */
        int dup = 0;
        for (size_t j = 0; j < *nout; j++) {
            if (strcmp((*out)[j], name) == 0) { dup = 1; break; }
        }
        if (!dup) {
            VEC_PUSH(*out, *nout, *cap, csvs_strdup(name));
        }

        /* recurse into children */
        for (size_t j = 0; j < lf->nentries; j++) {
            if (lf->entries[j].nleaves > 0)
                gather_keys(&lf->entries[j], out, nout, cap);
        }
    }
}

/* Find the trunk that connects branch to query base */
static char *find_trunk_for_base(const csvs_schema *s, const char *base,
                                 const char *branch)
{
    csvs_branch *b = csvs_schema_find(s, branch);
    if (!b) return NULL;

    /* Prefer direct: trunk == base */
    for (size_t i = 0; i < b->ntrunks; i++) {
        if (strcmp(b->trunks[i], base) == 0)
            return csvs_strdup(base);
    }

    /* Otherwise first trunk connected to base */
    for (size_t i = 0; i < b->ntrunks; i++) {
        if (csvs_is_connected(s, base, b->trunks[i]))
            return csvs_strdup(b->trunks[i]);
    }

    return NULL;
}

csvs_tablet_plan *csvs_plan_query(const csvs_schema *s, const csvs_entry *q,
                                  size_t *nout)
{
    /* Gather queried branch names */
    char **branches = NULL;
    size_t nbranches = 0, bcap = 0;
    gather_keys(q, &branches, &nbranches, &bcap);

    /* Sort ascending by nesting level */
    csvs_sort_ascending(s, branches, nbranches);

    /* Build tablet plans */
    csvs_tablet_plan *plans = NULL;
    size_t nplans = 0, pcap = 0;

    for (size_t i = 0; i < nbranches; i++) {
        char *trunk = find_trunk_for_base(s, q->base, branches[i]);
        if (!trunk) continue;

        csvs_tablet_plan plan;
        memset(&plan, 0, sizeof(plan));

        size_t tlen = strlen(trunk);
        size_t blen = strlen(branches[i]);
        plan.filename = malloc(tlen + 1 + blen + 4 + 1);
        sprintf(plan.filename, "%s-%s.csv", trunk, branches[i]);

        plan.thing = csvs_strdup(trunk);
        plan.trait_ = csvs_strdup(branches[i]);
        plan.base = csvs_strdup(trunk);
        plan.thing_is_first = 1;
        plan.trait_is_first = 0;
        plan.trait_is_regex = 1;
        plan.querying = 1;
        plan.eager = 1;
        plan.accumulating = 0;

        VEC_PUSH(plans, nplans, pcap, plan);
        free(trunk);
    }

    for (size_t i = 0; i < nbranches; i++) free(branches[i]);
    free(branches);

    *nout = nplans;
    return plans;
}

void csvs_tablet_plan_free(csvs_tablet_plan *t)
{
    if (!t) return;
    free(t->filename);
    free(t->thing);
    free(t->trait_);
    free(t->base);
}

/* ── Query execution ─────────────────────────────────────────────── */

/* State passed between tablets during query */
typedef struct {
    csvs_entry query;
    csvs_entry *entry;   /* NULL initially */
    char *thing_querying; /* parent join key */
} query_state;

static query_state query_state_new(const csvs_entry *q)
{
    query_state s;
    s.query = csvs_entry_clone(q);
    s.entry = NULL;
    s.thing_querying = NULL;
    return s;
}

static void query_state_free(query_state *s)
{
    csvs_entry_free(&s->query);
    if (s->entry) { csvs_entry_free(s->entry); free(s->entry); }
    free(s->thing_querying);
}

static query_state query_state_clone(const query_state *s)
{
    query_state out;
    out.query = csvs_entry_clone(&s->query);
    if (s->entry) {
        out.entry = malloc(sizeof(csvs_entry));
        *out.entry = csvs_entry_clone(s->entry);
    } else {
        out.entry = NULL;
    }
    out.thing_querying = csvs_strdup(s->thing_querying);
    return out;
}

/* Make initial state for a tablet */
static query_state make_state_initial(const query_state *prev,
                                      const csvs_tablet_plan *tablet)
{
    int is_same_base = prev->entry &&
                       strcmp(tablet->base, prev->query.base) == 0;
    int do_discard = !prev->entry || is_same_base;

    csvs_entry entry_initial;
    if (do_discard) {
        entry_initial = csvs_entry_new(tablet->base);
    } else {
        /* Sow previous entry identity into new base */
        csvs_entry e = csvs_entry_new(tablet->base);
        csvs_grain g = csvs_grain_new(
            prev->entry->base, prev->entry->base_value,
            prev->entry->base, NULL);
        entry_initial = csvs_sow(&e, &g, tablet->base, prev->entry->base);
        csvs_entry_free(&e);
        csvs_grain_free(&g);
    }

    int entry_base_changed = !prev->entry ||
                             strcmp(prev->entry->base, entry_initial.base) != 0;

    query_state out;
    out.query = csvs_entry_clone(&prev->query);
    out.entry = malloc(sizeof(csvs_entry));
    *out.entry = entry_initial;
    out.thing_querying = entry_base_changed ? NULL :
                         csvs_strdup(prev->thing_querying);

    return out;
}

/* Match a key group against query grains.
 * Returns 1 if matched, updates state in place. */
static int match_group(const csvs_key_group *group,
                       const csvs_tablet_plan *tablet,
                       const csvs_grain *grains, size_t ngrains,
                       query_state *state)
{
    csvs_entry group_entry = csvs_entry_clone(state->entry);
    csvs_entry group_query = csvs_entry_clone(&state->query);
    int matched = 0;
    char *group_thing_querying = NULL;

    for (size_t vi = 0; vi < group->nvalues; vi++) {
        const char *trait_ = tablet->trait_is_first ? group->key : group->values[vi];
        const char *thing = tablet->thing_is_first ? group->key : group->values[vi];

        /* Build grain from this CSV line */
        csvs_grain grain_new;
        if (tablet->thing_is_first) {
            if (tablet->trait_is_first) {
                grain_new = csvs_grain_new(tablet->thing, group->key,
                                           tablet->thing, NULL);
            } else {
                grain_new = csvs_grain_new(tablet->thing, group->key,
                                           tablet->trait_, group->values[vi]);
            }
        } else if (tablet->trait_is_first) {
            grain_new = csvs_grain_new(tablet->trait_, group->key,
                                       tablet->thing, group->values[vi]);
        } else {
            grain_new = csvs_grain_new(tablet->base, NULL,
                                       tablet->base, NULL);
        }

        /* Check all query grains match */
        int grains_match = 1;
        for (size_t gi = 0; gi < ngrains; gi++) {
            const char *re_str = tablet->trait_is_first
                ? (grains[gi].base_value ? grains[gi].base_value : "")
                : (grains[gi].leaf_value ? grains[gi].leaf_value : "");

            int is_match;
            if (tablet->trait_is_regex) {
                /* Unanchored regex match using POSIX ERE
                 * (matches Rust's Regex::is_match behavior) */
                regex_t re;
                int rc = regcomp(&re, re_str, REG_EXTENDED | REG_NOSUB);
                if (rc != 0) {
                    is_match = 0;
                } else {
                    is_match = regexec(&re, trait_, 0, NULL, 0) == 0;
                    regfree(&re);
                }
            } else {
                is_match = strcmp(re_str, trait_) == 0;
            }

            if (!is_match) { grains_match = 0; break; }
        }

        /* Check parent join key */
        int is_match_querying = 1;
        if (state->thing_querying) {
            is_match_querying = strcmp(state->thing_querying, thing) == 0;
        }

        if (grains_match && is_match_querying) {
            matched = 1;
            free(group_thing_querying);
            group_thing_querying = csvs_strdup(thing);

            csvs_entry new_entry = csvs_sow(&group_entry, &grain_new,
                                            tablet->trait_, tablet->thing);
            csvs_entry_free(&group_entry);
            group_entry = new_entry;

            csvs_entry new_query = csvs_sow(&group_query, &grain_new,
                                            tablet->trait_, tablet->thing);
            csvs_entry_free(&group_query);
            group_query = new_query;
        }

        csvs_grain_free(&grain_new);
    }

    if (matched) {
        csvs_entry_free(state->entry);
        *state->entry = group_entry;
        csvs_entry_free(&state->query);
        state->query = group_query;
        free(state->thing_querying);
        state->thing_querying = group_thing_querying;
    } else {
        csvs_entry_free(&group_entry);
        csvs_entry_free(&group_query);
        free(group_thing_querying);
    }

    return matched;
}

/* Execute query across tablets, return all matching entries.
 * Uses a counter-based nested-loop join (mirrors Rust's stream approach). */
csvs_entry *csvs_query_execute(csvs_dataset *ds, const csvs_entry *query,
                               const csvs_tablet_plan *strategy, size_t nplans,
                               size_t *nresults)
{
    csvs_entry *results = NULL;
    size_t nres = 0, res_cap = 0;

    if (nplans == 0) {
        *nresults = 0;
        return NULL;
    }

    /* For each tablet, we load its groups and iterate.
     * state_stack[c] = current state entering tablet c
     * group_idx[c] = current group index in tablet c
     * groups_stack[c] = loaded groups for tablet c */

    query_state *state_stack = calloc(nplans, sizeof(query_state));
    size_t *group_idx = calloc(nplans, sizeof(size_t));
    csvs_groups *groups_stack = calloc(nplans, sizeof(csvs_groups));
    /* initial_states: the initialized state for each tablet */
    query_state *init_stack = calloc(nplans, sizeof(query_state));
    int *initialized = calloc(nplans, sizeof(int));

    /* Start with tablet 0 */
    state_stack[0] = query_state_new(query);

    size_t counter = 0;

    while (1) {
        /* Initialize tablet if needed */
        if (!initialized[counter]) {
            /* Load groups for this tablet */
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", ds->dir, strategy[counter].filename);

            struct stat st;
            int file_exists = stat(path, &st) == 0;

            if (!file_exists) {
                if (counter > 0) {
                    /* Non-first tablet missing: pass through */
                    if (counter == nplans - 1) {
                        /* Last tablet: yield */
                        if (state_stack[counter].entry) {
                            VEC_PUSH(results, nres, res_cap,
                                     csvs_entry_clone(state_stack[counter].entry));
                        }
                    } else {
                        /* Middle tablet: advance to next */
                        counter++;
                        state_stack[counter] = query_state_clone(&state_stack[counter - 1]);
                        continue;
                    }
                }
                /* First tablet missing or yielded: backtrack */
                if (counter == 0) break;
                query_state_free(&state_stack[counter]);
                memset(&state_stack[counter], 0, sizeof(query_state));
                initialized[counter] = 0;
                counter--;
                continue;
            }

            groups_stack[counter] = csvs_read_groups(path);
            group_idx[counter] = 0;

            init_stack[counter] = make_state_initial(&state_stack[counter],
                                                     &strategy[counter]);
            initialized[counter] = 1;
        }

        /* Try next group in current tablet */
        int found = 0;
        while (group_idx[counter] < groups_stack[counter].ngroups) {
            size_t gi = group_idx[counter]++;

            /* Create a fresh state from init for this group */
            query_state trial = query_state_clone(&init_stack[counter]);

            /* Mow grains from query for this tablet */
            size_t ngrains;
            csvs_grain *grains = csvs_mow(&trial.query,
                                          strategy[counter].trait_,
                                          strategy[counter].thing,
                                          &ngrains);

            if (match_group(&groups_stack[counter].groups[gi],
                           &strategy[counter], grains, ngrains, &trial)) {
                found = 1;

                if (counter == nplans - 1) {
                    /* Last tablet: yield entry */
                    if (trial.entry) {
                        VEC_PUSH(results, nres, res_cap,
                                 csvs_entry_clone(trial.entry));
                    }
                    for (size_t g = 0; g < ngrains; g++) csvs_grain_free(&grains[g]);
                    free(grains);
                    query_state_free(&trial);
                    /* Continue to next group in last tablet */
                    continue;
                } else {
                    /* Not last: advance to next tablet */
                    counter++;
                    state_stack[counter] = trial;
                    for (size_t g = 0; g < ngrains; g++) csvs_grain_free(&grains[g]);
                    free(grains);
                    break;
                }
            }

            for (size_t g = 0; g < ngrains; g++) csvs_grain_free(&grains[g]);
            free(grains);
            query_state_free(&trial);
        }

        if (!found) {
            /* Exhausted this tablet: clean up and backtrack */
            csvs_groups_free(&groups_stack[counter]);
            memset(&groups_stack[counter], 0, sizeof(csvs_groups));
            query_state_free(&init_stack[counter]);
            memset(&init_stack[counter], 0, sizeof(query_state));
            query_state_free(&state_stack[counter]);
            memset(&state_stack[counter], 0, sizeof(query_state));
            initialized[counter] = 0;

            if (counter == 0) break;
            counter--;
        }
    }

    /* Cleanup */
    for (size_t i = 0; i < nplans; i++) {
        if (initialized[i]) {
            csvs_groups_free(&groups_stack[i]);
            query_state_free(&init_stack[i]);
        }
        /* state_stack[i] may have been freed in backtrack */
    }
    free(state_stack);
    free(group_idx);
    free(groups_stack);
    free(init_stack);
    free(initialized);

    *nresults = nres;
    return results;
}
