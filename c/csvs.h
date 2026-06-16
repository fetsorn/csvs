/*
 * csvs - Comma-Separated Value Store
 * Public API header
 *
 * Copyright (c) 2026 Anton Davydov
 * SPDX-License-Identifier: LGPL-3.0
 */

#ifndef CSVS_H
#define CSVS_H

#include <stddef.h>
#include <cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Forward declarations ─────────────────────────────────────────── */

typedef struct csvs_entry csvs_entry;
typedef struct csvs_iter  csvs_iter;

/* ── Leaf group: named collection of child entries ────────────────── */

typedef struct {
    char         *name;     /* leaf collection name (owned) */
    csvs_entry   *entries;  /* array of child entries */
    size_t        nentries;
    size_t        cap;
} csvs_leaf;

/* ── Prose blob: optional language tag + content ──────────────────── */

typedef struct {
    char *lang;     /* NULL = untagged ("@"), otherwise BCP 47 tag (owned) */
    char *content;  /* blob content (owned) */
} csvs_prose;

/* ── Entry: a SON record ──────────────────────────────────────────── */

struct csvs_entry {
    char         *base;         /* collection name (owned) */
    char         *base_value;   /* NULL when absent (owned) */
    char         *leader_value; /* NULL when absent (owned) */
    csvs_leaf    *leaves;       /* sorted array by name */
    size_t        nleaves;
    size_t        leaves_cap;
    csvs_prose   *prose;        /* array of prose blobs */
    size_t        nprose;
    size_t        prose_cap;
};

/* ── Grain: a single key-value pair from a tablet ─────────────────── */

typedef struct {
    char *base;       /* (owned) */
    char *base_value; /* may be NULL (owned) */
    char *leaf;       /* (owned) */
    char *leaf_value; /* may be NULL (owned) */
} csvs_grain;

/* ── Schema ───────────────────────────────────────────────────────── */

typedef struct {
    char   *name;       /* branch name (owned) */
    char  **trunks;     /* array of trunk names (each owned) */
    size_t  ntrunks;
    size_t  trunks_cap;
    char  **leaves;     /* array of leaf names (each owned) */
    size_t  nleaves;
    size_t  leaves_cap;
} csvs_branch;

typedef struct {
    csvs_branch *branches; /* sorted array by name */
    size_t       nbranches;
    size_t       cap;
} csvs_schema;

/* ── Dataset handle ───────────────────────────────────────────────── */

typedef struct {
    char         *dir;    /* dataset directory path (owned) */
    csvs_schema  *schema; /* NULL = not cached */
} csvs_dataset;

/* ── Entry lifecycle ──────────────────────────────────────────────── */

csvs_entry  csvs_entry_new(const char *base);
csvs_entry  csvs_entry_clone(const csvs_entry *e);
void        csvs_entry_free(csvs_entry *e);

/* JSON conversion (SON format) */
csvs_entry  csvs_entry_from_json(const cJSON *j);
cJSON      *csvs_entry_to_json(const csvs_entry *e);

/* Leaf manipulation on an entry (sorted insert / binary search find) */
csvs_leaf  *csvs_entry_find_leaf(const csvs_entry *e, const char *name);
csvs_leaf  *csvs_entry_add_leaf(csvs_entry *e, const char *name);
void        csvs_leaf_push(csvs_leaf *lf, csvs_entry child);

/* Prose manipulation */
void        csvs_entry_add_prose(csvs_entry *e, const char *lang,
                                 const char *content);
const char *csvs_entry_get_prose(const csvs_entry *e, const char *lang);

/* ── Grain lifecycle ──────────────────────────────────────────────── */

csvs_grain  csvs_grain_new(const char *base, const char *base_value,
                            const char *leaf, const char *leaf_value);
csvs_grain  csvs_grain_clone(const csvs_grain *g);
void        csvs_grain_free(csvs_grain *g);

/* JSON conversion (SON format) */
csvs_grain  csvs_grain_from_json(const cJSON *j);
cJSON      *csvs_grain_to_json(const csvs_grain *g);

/* ── Mow / Sow (pure, no I/O) ────────────────────────────────────── */

csvs_grain *csvs_mow(const csvs_entry *e, const char *trait_,
                      const char *thing, size_t *nout);
csvs_entry  csvs_sow(const csvs_entry *e, const csvs_grain *g,
                      const char *trait_, const char *thing);

/* ── Schema operations (pure, no I/O) ─────────────────────────────── */

csvs_schema  csvs_schema_new(void);
csvs_schema  csvs_schema_clone(const csvs_schema *s);
void         csvs_schema_free(csvs_schema *s);

/* JSON conversion */
cJSON       *csvs_schema_to_json(const csvs_schema *s);
csvs_branch *csvs_schema_find(const csvs_schema *s, const char *name);
csvs_branch *csvs_schema_add(csvs_schema *s, const char *name);

csvs_schema  csvs_to_schema(const csvs_entry *schema_record);
int          csvs_is_connected(const csvs_schema *s, const char *base,
                               const char *branch);
char       **csvs_find_crown(const csvs_schema *s, const char *base,
                             size_t *nout);
int          csvs_nesting_level(const csvs_schema *s, const char *branch);
void         csvs_sort_ascending(const csvs_schema *s, char **names, size_t n);
void         csvs_sort_descending(const csvs_schema *s, char **names, size_t n);

/* ── Dataset lifecycle ────────────────────────────────────────────── */

csvs_dataset *csvs_open(const char *dir);
csvs_dataset *csvs_create(const char *dir, int bare);
void          csvs_close(csvs_dataset *ds);

/* Schema caching */
int           csvs_schema_load(csvs_dataset *ds);
const csvs_schema *csvs_dataset_schema(csvs_dataset *ds);

/* ── Read operations ──────────────────────────────────────────────── */

csvs_iter        *csvs_select(csvs_dataset *ds, const csvs_entry *queries,
                              size_t n, int light, int prose);
const csvs_entry *csvs_next(csvs_iter *it);
void              csvs_iter_free(csvs_iter *it);

/* ── Write operations ─────────────────────────────────────────────── */

int csvs_update(csvs_dataset *ds, const csvs_entry *queries, size_t n);
int csvs_insert(csvs_dataset *ds, const csvs_entry *queries, size_t n);
int csvs_delete(csvs_dataset *ds, const csvs_entry *queries, size_t n);

/* ── Error reporting ──────────────────────────────────────────────── */

const char *csvs_errmsg(void);

#ifdef __cplusplus
}
#endif

#endif /* CSVS_H */
