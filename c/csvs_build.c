/*
 * csvs - Build: enrich a matched entry with all connected tablet data
 *
 * After query finds matching entries (partial), build fills in all
 * remaining leaf data from the schema crown.
 */

#include "csvs_internal.h"

/* ── Strategy: plan_build ────────────────────────────────────────── */

csvs_tablet_plan *csvs_plan_build(const csvs_schema *s, const csvs_entry *q,
                                  size_t *nout)
{
    size_t ncrown;
    char **crown = csvs_find_crown(s, q->base, &ncrown);

    /* Filter out base itself, sort descending by nesting */
    char **filtered = NULL;
    size_t nf = 0, fcap = 0;
    for (size_t i = 0; i < ncrown; i++) {
        if (strcmp(crown[i], q->base) != 0) {
            VEC_PUSH(filtered, nf, fcap, csvs_strdup(crown[i]));
        }
        free(crown[i]);
    }
    free(crown);

    csvs_sort_descending(s, filtered, nf);

    csvs_tablet_plan *plans = NULL;
    size_t nplans = 0, pcap = 0;

    for (size_t i = 0; i < nf; i++) {
        const char *branch = filtered[i];
        csvs_branch *binfo = csvs_schema_find(s, branch);
        if (!binfo) { free(filtered[i]); continue; }

        /* For each trunk of this branch that's in the crown */
        for (size_t t = 0; t < binfo->ntrunks; t++) {
            /* Check trunk is in full crown (reachable from base) */
            if (!csvs_is_connected(s, q->base, binfo->trunks[t]) &&
                strcmp(binfo->trunks[t], q->base) != 0)
                continue;

            csvs_tablet_plan plan;
            memset(&plan, 0, sizeof(plan));

            plan.filename = malloc(strlen(binfo->trunks[t]) + 1 +
                                   strlen(branch) + 4 + 1);
            sprintf(plan.filename, "%s-%s.csv", binfo->trunks[t], branch);

            plan.thing = csvs_strdup(branch);
            plan.trait_ = csvs_strdup(binfo->trunks[t]);
            plan.base = csvs_strdup(binfo->trunks[t]);
            plan.thing_is_first = 0;
            plan.trait_is_first = 1;
            plan.trait_is_regex = 0;
            plan.querying = 0;
            plan.eager = strcmp(binfo->trunks[t], q->base) == 0;
            plan.accumulating = 0;

            VEC_PUSH(plans, nplans, pcap, plan);
        }
        free(filtered[i]);
    }
    free(filtered);

    *nout = nplans;
    return plans;
}

/* ── Build execution ─────────────────────────────────────────────── */

/* Build a single tablet: scan CSV for matching keys, sow into entry */
static csvs_entry build_tablet(const char *dir,
                               const csvs_tablet_plan *tablet,
                               csvs_entry entry)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, tablet->filename);

    csvs_groups gs = csvs_read_groups(path);
    if (gs.ngroups == 0) return entry;

    /* Get grains from entry for this tablet's relationship */
    size_t ngrains;
    csvs_grain *grains = csvs_mow(&entry, tablet->trait_, tablet->thing, &ngrains);

    char *fst = NULL;
    int is_match = 0;

    for (size_t gi = 0; gi < gs.ngroups; gi++) {
        csvs_key_group *grp = &gs.groups[gi];

        for (size_t vi = 0; vi < grp->nvalues; vi++) {
            const char *key = grp->key;
            const char *val = grp->values[vi];

            int fst_is_new = !fst || strcmp(fst, key) != 0;
            free(fst);
            fst = csvs_strdup(key);

            /* Check if end of group and we had a match */
            if (tablet->eager && fst_is_new && is_match) {
                /* Group complete — but build doesn't yield early,
                   it just continues accumulating */
            }

            /* Build grain from line */
            csvs_grain grain_new = csvs_grain_new(
                tablet->trait_, key,
                tablet->thing, val);

            /* Check match against entry grains */
            for (size_t g = 0; g < ngrains; g++) {
                const char *re_str = grains[g].base_value ? grains[g].base_value : "";
                int match = strcmp(re_str, key) == 0;

                if (!is_match) is_match = match;

                if (match) {
                    csvs_entry new_entry = csvs_sow(&entry, &grain_new,
                                                    tablet->trait_, tablet->thing);
                    csvs_entry_free(&entry);
                    entry = new_entry;
                }
            }

            csvs_grain_free(&grain_new);
        }
    }

    free(fst);
    for (size_t g = 0; g < ngrains; g++) csvs_grain_free(&grains[g]);
    free(grains);
    csvs_groups_free(&gs);

    return entry;
}

csvs_entry csvs_build_record(csvs_dataset *ds, csvs_entry query, int prose)
{
    const csvs_schema *schema = csvs_dataset_schema(ds);

    size_t nplans;
    csvs_tablet_plan *plans = csvs_plan_build(schema, &query, &nplans);

    csvs_entry entry = csvs_entry_clone(&query);

    for (size_t i = 0; i < nplans; i++) {
        entry = build_tablet(ds->dir, &plans[i], entry);
    }

    if (prose) {
        /* TODO: attach prose blobs */
        (void)prose;
    }

    for (size_t i = 0; i < nplans; i++)
        csvs_tablet_plan_free(&plans[i]);
    free(plans);

    return entry;
}
