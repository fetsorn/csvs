/*
 * csvs - Grain struct: a single key-value pair from a tablet
 */

#include "csvs_internal.h"

csvs_grain csvs_grain_new(const char *base, const char *base_value,
                           const char *leaf, const char *leaf_value)
{
    csvs_grain g;
    g.base       = csvs_strdup(base);
    g.base_value = csvs_strdup(base_value);   /* NULL-safe */
    g.leaf       = csvs_strdup(leaf);
    g.leaf_value = csvs_strdup(leaf_value);    /* NULL-safe */
    return g;
}

csvs_grain csvs_grain_clone(const csvs_grain *g)
{
    return csvs_grain_new(g->base, g->base_value, g->leaf, g->leaf_value);
}

void csvs_grain_free(csvs_grain *g)
{
    if (!g) return;
    free(g->base);       g->base = NULL;
    free(g->base_value); g->base_value = NULL;
    free(g->leaf);       g->leaf = NULL;
    free(g->leaf_value); g->leaf_value = NULL;
}
