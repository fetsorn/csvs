/*
 * csvs - internal declarations shared across translation units
 * Not installed as a public header.
 */

#ifndef CSVS_INTERNAL_H
#define CSVS_INTERNAL_H

#include "csvs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Error handling ───────────────────────────────────────────────── */

/* Thread-local error buffer */
#define CSVS_ERRBUF_SIZE 512
void csvs_set_error(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

/* ── String helpers ───────────────────────────────────────────────── */

static inline char *csvs_strdup(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = malloc(len + 1);
    if (d) memcpy(d, s, len + 1);
    return d;
}

static inline char *csvs_strndup(const char *s, size_t n)
{
    if (!s) return NULL;
    char *d = malloc(n + 1);
    if (d) { memcpy(d, s, n); d[n] = '\0'; }
    return d;
}

/* ── Generic growable array ───────────────────────────────────────── */

/*
 * VEC_PUSH(arr, len, cap, item)
 * Appends `item` to the array, growing if needed.
 * arr  - pointer to array base (reallocated in place)
 * len  - size_t current length
 * cap  - size_t current capacity
 * item - value to append
 */
#define VEC_PUSH(arr, len, cap, item) do {              \
    if ((len) >= (cap)) {                               \
        size_t _nc = (cap) ? (cap) * 2 : 4;            \
        void *_np = realloc((arr), _nc * sizeof(*(arr))); \
        if (!_np) {                                     \
            (void)fprintf(stderr, "csvs: out of memory\n"); \
            abort();                                    \
        }                                               \
        (arr) = _np;                                    \
        (cap) = _nc;                                    \
    }                                                   \
    (arr)[(len)++] = (item);                            \
} while (0)

/* ── CSV line I/O (csvs_line.c) ───────────────────────────────────── */

/* Escape literal newlines for CSV storage: \n -> \\n */
char *csvs_escape_newline(const char *s);
/* Unescape: \\n -> \n */
char *csvs_unescape_newline(const char *s);

/* Key group: a run of values sharing the same key in a sorted tablet */
typedef struct {
    char   *key;
    char  **values;
    size_t  nvalues;
    size_t  cap;
} csvs_key_group;

typedef struct {
    csvs_key_group *groups;
    size_t          ngroups;
    size_t          cap;
} csvs_groups;

csvs_groups csvs_read_groups(const char *filepath);
void        csvs_groups_free(csvs_groups *gs);

/* Parse a single CSV line into key and value (caller frees both).
 * Returns 0 on success, -1 on error. */
int csvs_parse_line(const char *line, size_t len,
                    char **key_out, char **val_out);

/* Write a CSV line (escaped) to fp.  Returns 0 on success. */
int csvs_write_line(FILE *fp, const char *key, const char *value);

/* ── Version tablet (csvs_version.c) ──────────────────────────────── */

csvs_entry csvs_select_version(const char *dir);
int        csvs_update_version(const char *dir, const csvs_entry *query);

/* ── Schema tablet (csvs_dataset.c / csvs_schema.c) ──────────────── */

csvs_entry csvs_select_schema_record(const char *dir);
int        csvs_update_schema_record(const char *dir, const csvs_entry *query);

/* ── File helpers ─────────────────────────────────────────────────── */

/* Check if a file is empty or missing.  Returns 1 if empty/missing. */
int csvs_file_is_empty(const char *path);

/* Sort a tablet file by unescaped key byte order. */
int csvs_sort_file(const char *dir, const char *filename);

/* ── Prose store (csvs_prose.c) ───────────────────────────────────── */

int   csvs_prose_read(const char *dir, const char *value,
                      csvs_prose **out, size_t *nout);
int   csvs_prose_write(const char *dir, const char *value,
                       const char *lang, const char *content);
char **csvs_prose_search(const char *dir, const char *pattern,
                         const char *lang, size_t *nout);

/* ── Strategy types (shared by query/build/update/insert/delete) ── */

typedef struct {
    char *filename;
    char *thing;
    char *trait_;
    char *base;
    int   thing_is_first;
    int   trait_is_first;
    int   trait_is_regex;
    int   querying;
    int   eager;
    int   accumulating;
} csvs_tablet_plan;

void csvs_tablet_plan_free(csvs_tablet_plan *t);

typedef struct {
    char *filename;
    char *trunk;
    char *branch;
} csvs_tablet_rw;

void csvs_tablet_rw_free(csvs_tablet_rw *t);

/* ── Strategy planners ────────────────────────────────────────────── */

csvs_tablet_plan *csvs_plan_query(const csvs_schema *s, const csvs_entry *q,
                                  size_t *nout);
csvs_tablet_plan *csvs_plan_build(const csvs_schema *s, const csvs_entry *q,
                                  size_t *nout);
csvs_tablet_plan *csvs_plan_options(const csvs_schema *s, const char *base,
                                    size_t *nout);
csvs_tablet_rw   *csvs_plan_update(const csvs_schema *s, const csvs_entry *q,
                                   size_t *nout);
csvs_tablet_rw   *csvs_plan_insert(const csvs_schema *s, const csvs_entry *q,
                                   size_t *nout);

typedef struct {
    char *filename;
    char *trait_value;
    int   trait_is_first;
} csvs_tablet_prune;

void csvs_tablet_prune_free(csvs_tablet_prune *t);

csvs_tablet_prune *csvs_plan_delete(const csvs_schema *s, const csvs_entry *q,
                                    size_t *nout);

/* ── Query / Option execution ─────────────────────────────────────── */

csvs_entry *csvs_query_execute(csvs_dataset *ds, const csvs_entry *query,
                               const csvs_tablet_plan *strategy, size_t nplans,
                               size_t *nresults);
csvs_entry *csvs_option_execute(csvs_dataset *ds, const csvs_entry *query,
                                const csvs_tablet_plan *strategy, size_t nplans,
                                size_t *nresults);

/* ── Build ────────────────────────────────────────────────────────── */

csvs_entry csvs_build_record(csvs_dataset *ds, csvs_entry query, int prose);

#endif /* CSVS_INTERNAL_H */
