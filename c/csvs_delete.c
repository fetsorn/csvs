/*
 * csvs - Delete: remove records
 * Stub — Phase 4
 */

#include "csvs_internal.h"

csvs_tablet_prune *csvs_plan_delete(const csvs_schema *s __attribute__((unused)),
                                    const csvs_entry *q __attribute__((unused)),
                                    size_t *nout)
{
    *nout = 0;
    return NULL;
}

void csvs_tablet_prune_free(csvs_tablet_prune *t)
{
    if (!t) return;
    free(t->filename);
    free(t->trait_value);
}

int csvs_delete(csvs_dataset *ds __attribute__((unused)),
                const csvs_entry *queries __attribute__((unused)),
                size_t n __attribute__((unused)))
{
    csvs_set_error("delete not yet implemented");
    return -1;
}
