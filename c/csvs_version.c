/*
 * csvs - Version tablet (.csvs.csv) read/write
 */

#include "csvs_internal.h"

csvs_entry csvs_select_version(const char *dir)
{
    csvs_entry e = csvs_entry_new(".");

    char path[1024];
    (void)snprintf(path, sizeof(path), "%s/.csvs.csv", dir);

    csvs_groups gs = csvs_read_groups(path);

    for (size_t i = 0; i < gs.ngroups; i++) {
        csvs_key_group *grp = &gs.groups[i];
        for (size_t j = 0; j < grp->nvalues; j++) {
            csvs_leaf *lf = csvs_entry_add_leaf(&e, grp->key);
            csvs_entry child = csvs_entry_new(grp->key);
            child.base_value = csvs_strdup(grp->values[j]);
            csvs_leaf_push(lf, child);
        }
    }

    csvs_groups_free(&gs);
    return e;
}

int csvs_update_version(const char *dir, const csvs_entry *query)
{
    char path[1024];
    (void)snprintf(path, sizeof(path), "%s/.csvs.csv", dir);

    FILE *fp = fopen(path, "w");
    if (!fp) {
        csvs_set_error("cannot open %s for writing", path);
        return -1;
    }

    /* Collect all key-value lines, sort, write */
    typedef struct { char *key; char *val; } kv;
    kv *lines = NULL;
    size_t nlines = 0, lines_cap = 0;

    for (size_t i = 0; i < query->nleaves; i++) {
        const csvs_leaf *lf = &query->leaves[i];
        for (size_t j = 0; j < lf->nentries; j++) {
            kv line;
            line.key = csvs_strdup(lf->entries[j].base);
            line.val = csvs_strdup(lf->entries[j].base_value);
            VEC_PUSH(lines, nlines, lines_cap, line);
        }
    }

    /* Write sorted */
    int werr = 0;
    for (size_t i = 0; i < nlines; i++) {
        if (csvs_write_line(fp, lines[i].key, lines[i].val) != 0)
            werr = 1;
    }

    for (size_t i = 0; i < nlines; i++) {
        free(lines[i].key);
        free(lines[i].val);
    }
    free(lines);
    if (fclose(fp) != 0 || werr) {
        csvs_set_error("write error on %s", path);
        return -1;
    }
    return 0;
}
