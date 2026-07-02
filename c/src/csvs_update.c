/*
 * csvs - Update: modify existing records in-place
 * Stub — Phase 4
 */

#include "csvs_internal.h"

csvs_tablet_rw *csvs_plan_update(const csvs_schema *s __attribute__((unused)),
                                 const csvs_entry *q __attribute__((unused)),
                                 size_t *nout)
{
    *nout = 0;
    return NULL;
}

void csvs_tablet_rw_free(csvs_tablet_rw *t)
{
    if (!t) return;
    free(t->filename);
    free(t->trunk);
    free(t->branch);
}

int csvs_update(csvs_dataset *ds __attribute__((unused)),
                const csvs_entry *queries __attribute__((unused)),
                size_t n __attribute__((unused)))
{
    csvs_set_error("update not yet implemented");
    return -1;
}
