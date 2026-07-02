/*
 * csvs - Prose store: read/write/search prose blobs
 * Stub — Phase 4
 */

#include "csvs_internal.h"

int csvs_prose_read(const char *dir __attribute__((unused)),
                    const char *value __attribute__((unused)),
                    csvs_prose **out, size_t *nout)
{
    *out = NULL;
    *nout = 0;
    return 0;
}

int csvs_prose_write(const char *dir __attribute__((unused)),
                     const char *value __attribute__((unused)),
                     const char *lang __attribute__((unused)),
                     const char *content __attribute__((unused)))
{
    csvs_set_error("prose write not yet implemented");
    return -1;
}

char **csvs_prose_search(const char *dir __attribute__((unused)),
                         const char *pattern __attribute__((unused)),
                         const char *lang __attribute__((unused)),
                         size_t *nout)
{
    *nout = 0;
    return NULL;
}
