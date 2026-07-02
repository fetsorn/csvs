/*
 * csvs - Dataset handle: open, close, schema loading
 */

#include "csvs_internal.h"
#include <sys/stat.h>

/* ── Open ────────────────────────────────────────────────────────── */

csvs_dataset *csvs_open(const char *dir)
{
    char path[1024];

    /* Check for .csvs.csv in dir */
    (void)snprintf(path, sizeof(path), "%s/.csvs.csv", dir);
    struct stat st;
    if (stat(path, &st) == 0) {
        csvs_dataset *ds = calloc(1, sizeof(csvs_dataset));
        ds->dir = csvs_strdup(dir);
        return ds;
    }

    /* Check for csvs/ subdirectory */
    char nested[1024];
    (void)snprintf(nested, sizeof(nested), "%s/csvs", dir);
    (void)snprintf(path, sizeof(path), "%s/csvs/.csvs.csv", dir);
    if (stat(path, &st) == 0) {
        csvs_dataset *ds = calloc(1, sizeof(csvs_dataset));
        ds->dir = csvs_strdup(nested);
        return ds;
    }

    csvs_set_error("no dataset at %s", dir);
    return NULL;
}

/* ── Close ───────────────────────────────────────────────────────── */

void csvs_close(csvs_dataset *ds)
{
    if (!ds) return;
    free(ds->dir);
    if (ds->schema) {
        csvs_schema_free(ds->schema);
        free(ds->schema);
    }
    free(ds);
}

/* ── Schema loading ──────────────────────────────────────────────── */

csvs_entry csvs_select_schema_record(const char *dir)
{
    csvs_entry e = csvs_entry_new("_");

    char path[1024];
    (void)snprintf(path, sizeof(path), "%s/_-_.csv", dir);

    csvs_groups gs = csvs_read_groups(path);

    for (size_t i = 0; i < gs.ngroups; i++) {
        csvs_key_group *grp = &gs.groups[i];
        const char *trunk = grp->key;

        for (size_t j = 0; j < grp->nvalues; j++) {
            const char *leaf_name = grp->values[j];

            csvs_leaf *lf = csvs_entry_add_leaf(&e, trunk);

            csvs_entry child = csvs_entry_new(trunk);
            child.base_value = csvs_strdup(leaf_name);
            csvs_leaf_push(lf, child);
        }
    }

    csvs_groups_free(&gs);
    return e;
}

int csvs_update_schema_record(const char *dir, const csvs_entry *query)
{
    char path[1024];
    (void)snprintf(path, sizeof(path), "%s/_-_.csv", dir);

    /* Collect all trunk-leaf pairs as lines */
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

    /* Sort by key then value */
    for (size_t i = 1; i < nlines; i++) {
        for (size_t j = i; j > 0; j--) {
            int cmp = strcmp(lines[j - 1].key, lines[j].key);
            if (cmp == 0) cmp = strcmp(lines[j - 1].val, lines[j].val);
            if (cmp <= 0) break;
            kv tmp = lines[j - 1];
            lines[j - 1] = lines[j];
            lines[j] = tmp;
        }
    }

    FILE *fp = fopen(path, "w");
    if (!fp) {
        csvs_set_error("cannot open %s for writing", path);
        for (size_t i = 0; i < nlines; i++) { free(lines[i].key); free(lines[i].val); }
        free(lines);
        return -1;
    }

    int werr = 0;
    for (size_t i = 0; i < nlines; i++) {
        if (csvs_write_line(fp, lines[i].key, lines[i].val) != 0)
            werr = 1;
    }

    for (size_t i = 0; i < nlines; i++) { free(lines[i].key); free(lines[i].val); }
    free(lines);
    if (fclose(fp) != 0 || werr) {
        csvs_set_error("write error on %s", path);
        return -1;
    }
    return 0;
}

int csvs_schema_load(csvs_dataset *ds)
{
    if (ds->schema) {
        csvs_schema_free(ds->schema);
        free(ds->schema);
    }

    csvs_entry rec = csvs_select_schema_record(ds->dir);
    csvs_schema s = csvs_to_schema(&rec);
    csvs_entry_free(&rec);

    ds->schema = malloc(sizeof(csvs_schema));
    *ds->schema = s;
    return 0;
}

const csvs_schema *csvs_dataset_schema(csvs_dataset *ds)
{
    if (!ds->schema)
        csvs_schema_load(ds);
    return ds->schema;
}
