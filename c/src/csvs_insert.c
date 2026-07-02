/*
 * csvs - Insert: add new records
 * Stub — Phase 4
 */

#include "csvs_internal.h"

csvs_tablet_rw *csvs_plan_insert(const csvs_schema *s __attribute__((unused)),
                                 const csvs_entry *q __attribute__((unused)),
                                 size_t *nout)
{
    *nout = 0;
    return NULL;
}

int csvs_insert(csvs_dataset *ds __attribute__((unused)),
                const csvs_entry *queries __attribute__((unused)),
                size_t n __attribute__((unused)))
{
    csvs_set_error("insert not yet implemented");
    return -1;
}
