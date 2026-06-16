/*
 * csvs - Option: list all unique values for a branch
 *
 * When a query has no leaf constraints (just { "_": "actname" }),
 * option scans all tablets where the branch appears and collects
 * unique values.
 */

#include "csvs_internal.h"
#include <regex.h>

/* ── Strategy: plan_options ──────────────────────────────────────── */

csvs_tablet_plan *csvs_plan_options(const csvs_schema *s, const char *base,
                                    size_t *nout)
{
    csvs_branch *b = csvs_schema_find(s, base);
    if (!b) { *nout = 0; return NULL; }

    csvs_tablet_plan *plans = NULL;
    size_t nplans = 0, pcap = 0;

    /* Leaf tablets: base is first column (keys) */
    for (size_t i = 0; i < b->nleaves; i++) {
        csvs_tablet_plan plan;
        memset(&plan, 0, sizeof(plan));

        size_t flen = strlen(base) + 1 + strlen(b->leaves[i]) + 4 + 1;
        plan.filename = malloc(flen);
        if (!plan.filename) {
            (void)fprintf(stderr, "csvs: out of memory\n");
            abort();
        }
        (void)sprintf(plan.filename, "%s-%s.csv", base, b->leaves[i]);

        plan.thing = csvs_strdup(base);
        plan.trait_ = csvs_strdup(base);
        plan.base = csvs_strdup(base);
        plan.thing_is_first = 1;
        plan.trait_is_first = 1;
        plan.trait_is_regex = 1;
        plan.querying = 0;
        plan.eager = 1;
        plan.accumulating = 1;

        VEC_PUSH(plans, nplans, pcap, plan);
    }

    /* Trunk tablets: base is second column (values) */
    for (size_t i = 0; i < b->ntrunks; i++) {
        csvs_tablet_plan plan;
        memset(&plan, 0, sizeof(plan));

        size_t flen = strlen(b->trunks[i]) + 1 + strlen(base) + 4 + 1;
        plan.filename = malloc(flen);
        if (!plan.filename) {
            (void)fprintf(stderr, "csvs: out of memory\n");
            abort();
        }
        (void)sprintf(plan.filename, "%s-%s.csv", b->trunks[i], base);

        plan.thing = csvs_strdup(base);
        plan.trait_ = csvs_strdup(base);
        plan.base = csvs_strdup(base);
        plan.thing_is_first = 0;
        plan.trait_is_first = 0;
        plan.trait_is_regex = 1;
        plan.querying = 0;
        plan.eager = 1;
        plan.accumulating = 1;

        VEC_PUSH(plans, nplans, pcap, plan);
    }

    *nout = nplans;
    return plans;
}

/* ── Option execution ────────────────────────────────────────────── */

/* A set of seen base values to avoid duplicates */
typedef struct {
    char **keys;
    size_t nkeys;
    size_t cap;
} seen_set;

static int seen_contains(const seen_set *s, const char *key)
{
    for (size_t i = 0; i < s->nkeys; i++)
        if (strcmp(s->keys[i], key) == 0) return 1;
    return 0;
}

static void seen_add(seen_set *s, const char *key)
{
    if (seen_contains(s, key)) return;
    VEC_PUSH(s->keys, s->nkeys, s->cap, csvs_strdup(key));
}

static void seen_free(seen_set *s)
{
    for (size_t i = 0; i < s->nkeys; i++) free(s->keys[i]);
    free(s->keys);
}

csvs_entry *csvs_option_execute(csvs_dataset *ds, const csvs_entry *query,
                                const csvs_tablet_plan *strategy, size_t nplans,
                                size_t *nresults)
{
    csvs_entry *results = NULL;
    size_t nres = 0, res_cap = 0;

    seen_set seen;
    memset(&seen, 0, sizeof(seen));

    /* Compile base_value regex if present */
    regex_t bv_re;
    int has_bv_re = 0;
    if (query->base_value) {
        /* Unanchored regex (matches Rust's Regex::is_match behavior) */
        if (regcomp(&bv_re, query->base_value, REG_EXTENDED | REG_NOSUB) == 0)
            has_bv_re = 1;
    }

    for (size_t ti = 0; ti < nplans; ti++) {
        const csvs_tablet_plan *tablet = &strategy[ti];

        char path[1024];
        (void)snprintf(path, sizeof(path), "%s/%s", ds->dir, tablet->filename);

        csvs_groups gs = csvs_read_groups(path);

        csvs_entry cur = csvs_entry_new(tablet->base);
        char *fst = NULL;
        int is_match = 0;

        for (size_t gi = 0; gi < gs.ngroups; gi++) {
            csvs_key_group *grp = &gs.groups[gi];

            for (size_t vi = 0; vi < grp->nvalues; vi++) {
                const char *key = grp->key;
                const char *val = grp->values[vi];

                int fst_is_new = !fst || strcmp(fst, key) != 0;
                if (fst_is_new) {
                    /* Flush previous group if matched */
                    if (is_match) {
                        VEC_PUSH(results, nres, res_cap, cur);
                        cur = csvs_entry_new(tablet->base);
                        is_match = 0;
                    }
                }
                free(fst);
                fst = csvs_strdup(key);

                /* Extract the "base" value depending on direction */
                const char *base_val = tablet->thing_is_first ? key : val;

                csvs_grain grain_new = csvs_grain_new(
                    tablet->base, base_val,
                    tablet->base, NULL);

                /* Check if this value matches the query pattern */
                int val_match;
                if (has_bv_re) {
                    val_match = regexec(&bv_re, base_val, 0, NULL, 0) == 0;
                } else {
                    val_match = 1;  /* no filter = match all */
                }

                int is_new = !seen_contains(&seen, base_val);

                if (!is_match) {
                    is_match = val_match && is_new;
                }

                if (val_match && is_new) {
                    seen_add(&seen, base_val);
                    csvs_entry new_cur = csvs_sow(&cur, &grain_new,
                                                  tablet->base, tablet->base);
                    csvs_entry_free(&cur);
                    cur = new_cur;
                }

                csvs_grain_free(&grain_new);
            }
        }

        /* Flush final group */
        if (is_match) {
            VEC_PUSH(results, nres, res_cap, cur);
        } else {
            csvs_entry_free(&cur);
        }

        free(fst);
        csvs_groups_free(&gs);
    }

    if (has_bv_re) regfree(&bv_re);
    seen_free(&seen);

    *nresults = nres;
    return results;
}
