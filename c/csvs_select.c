/*
 * csvs - Select: high-level read dispatch + iterator
 *
 * Dispatches to:
 *   base == "_"  → schema record
 *   base == "."  → version record
 *   has leaves   → query + build
 *   no leaves    → option (list values) + build
 */

#include "csvs_internal.h"

/* Forward declarations from query/option */
csvs_entry *csvs_query_execute(csvs_dataset *ds, const csvs_entry *query,
                               const csvs_tablet_plan *strategy, size_t nplans,
                               size_t *nresults);
csvs_entry *csvs_option_execute(csvs_dataset *ds, const csvs_entry *query,
                                const csvs_tablet_plan *strategy, size_t nplans,
                                size_t *nresults);

/* ── Iterator ────────────────────────────────────────────────────── */

struct csvs_iter {
    csvs_entry *entries;
    size_t       nentries;
    size_t       cursor;
};

csvs_iter *csvs_select(csvs_dataset *ds, const csvs_entry *queries,
                       size_t n, int light, int prose)
{
    csvs_entry *all = NULL;
    size_t ntotal = 0, total_cap = 0;

    /* For dedup across multiple queries */
    char **seen_bv = NULL;
    size_t nseen = 0, seen_cap = 0;
    int need_dedup = n > 1;

    for (size_t qi = 0; qi < n; qi++) {
        const csvs_entry *q = &queries[qi];

        /* Schema query */
        if (q->base && strcmp(q->base, "_") == 0) {
            csvs_entry schema_rec = csvs_select_schema_record(ds->dir);
            VEC_PUSH(all, ntotal, total_cap, schema_rec);
            continue;
        }

        /* Version query */
        if (q->base && strcmp(q->base, ".") == 0) {
            csvs_entry version_rec = csvs_select_version(ds->dir);
            VEC_PUSH(all, ntotal, total_cap, version_rec);
            continue;
        }

        int has_leaves = q->nleaves > 0;

        if (has_leaves) {
            /* Query path */
            const csvs_schema *schema = csvs_dataset_schema(ds);

            size_t nplans;
            csvs_tablet_plan *plans = csvs_plan_query(schema, q, &nplans);

            size_t nresults;
            csvs_entry *results = csvs_query_execute(ds, q, plans, nplans,
                                                     &nresults);

            for (size_t i = 0; i < nresults; i++) {
                /* Dedup */
                if (need_dedup && results[i].base_value) {
                    int dup = 0;
                    for (size_t s = 0; s < nseen; s++) {
                        if (strcmp(seen_bv[s], results[i].base_value) == 0) {
                            dup = 1; break;
                        }
                    }
                    if (dup) { csvs_entry_free(&results[i]); continue; }
                    VEC_PUSH(seen_bv, nseen, seen_cap,
                             csvs_strdup(results[i].base_value));
                }

                if (light) {
                    VEC_PUSH(all, ntotal, total_cap, results[i]);
                } else {
                    csvs_entry built = csvs_build_record(ds, results[i], prose);
                    csvs_entry_free(&results[i]);
                    VEC_PUSH(all, ntotal, total_cap, built);
                }
            }
            free(results);

            for (size_t i = 0; i < nplans; i++)
                csvs_tablet_plan_free(&plans[i]);
            free(plans);
        } else {
            /* Option path */
            const csvs_schema *schema = csvs_dataset_schema(ds);

            size_t nplans;
            csvs_tablet_plan *plans = csvs_plan_options(schema, q->base, &nplans);

            size_t nresults;
            csvs_entry *results = csvs_option_execute(ds, q, plans, nplans,
                                                      &nresults);

            for (size_t i = 0; i < nresults; i++) {
                /* Dedup */
                if (need_dedup && results[i].base_value) {
                    int dup = 0;
                    for (size_t s = 0; s < nseen; s++) {
                        if (strcmp(seen_bv[s], results[i].base_value) == 0) {
                            dup = 1; break;
                        }
                    }
                    if (dup) { csvs_entry_free(&results[i]); continue; }
                    VEC_PUSH(seen_bv, nseen, seen_cap,
                             csvs_strdup(results[i].base_value));
                }

                if (light) {
                    VEC_PUSH(all, ntotal, total_cap, results[i]);
                } else {
                    csvs_entry built = csvs_build_record(ds, results[i], prose);
                    csvs_entry_free(&results[i]);
                    VEC_PUSH(all, ntotal, total_cap, built);
                }
            }
            free(results);

            for (size_t i = 0; i < nplans; i++)
                csvs_tablet_plan_free(&plans[i]);
            free(plans);
        }
    }

    for (size_t i = 0; i < nseen; i++) free(seen_bv[i]);
    free(seen_bv);

    csvs_iter *it = calloc(1, sizeof(csvs_iter));
    it->entries = all;
    it->nentries = ntotal;
    it->cursor = 0;
    return it;
}

const csvs_entry *csvs_next(csvs_iter *it)
{
    if (!it || it->cursor >= it->nentries) return NULL;
    return &it->entries[it->cursor++];
}

void csvs_iter_free(csvs_iter *it)
{
    if (!it) return;
    for (size_t i = 0; i < it->nentries; i++)
        csvs_entry_free(&it->entries[i]);
    free(it->entries);
    free(it);
}
