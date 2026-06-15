# C rewrite of csvs

- Status: active
- Date: 2026-06-15

## Goal

Rewrite csvs as a C11 library (`libcsvs`) that passes `~/mm/codes/csvs/test/`.
Single-threaded, pull-iterator API (readdir-style). POSIX filesystem calls,
WASI-compatible by construction.

## Prior art

The Rust implementation (`rs/`) is the structural reference:

- `Entry { base, base_value, leader_value, leaves: HashMap<String, Vec<Entry>>, prose }` struct
- `Grain { base, base_value, leaf, leaf_value }` struct
- `Schema(HashMap<String, Branch>)` where `Branch { trunks, leaves }`
- `Dataset { dir, schema_cache }` handle with optional cached schema
- Strategy pattern: `plan_query`, `plan_build`, `plan_update`, `plan_insert`, `plan_delete`
- Sort-merge join across sorted tablet files (ADR-0002)
- Tablets sorted by unescaped key byte order (ADR-0001)

The JS implementation (`js/`) adds streaming context: ReadableStream pull-iterators
for query, option, and select. The C iterator design follows the same pull pattern.

## External dependencies

| Dep                                                 | Purpose                          | WASI-safe                        |
|-----------------------------------------------------|----------------------------------|----------------------------------|
| [rgamble/libcsv](https://github.com/rgamble/libcsv) | RFC 4180 CSV parse/write         | yes (pure C, no syscalls)        |
| cJSON                                               | JSON parse for test harness only | yes                              |
| POSIX `<regex.h>` ERE                               | query matching                   | WASI preview 2 (polyfill for p1) |

libcsv is vendored or submoduled. cJSON is test-only, not linked into libcsvs.

## Data structures

### Entry

```c
typedef struct csvs_entry csvs_entry;

/* A leaf group: named collection of child entries */
typedef struct {
    char *name;          /* leaf collection name */
    csvs_entry *entries; /* array of child entries */
    size_t nentries;
    size_t cap;          /* allocated capacity */
} csvs_leaf;

/* A prose blob: optional language tag + content */
typedef struct {
    char *lang;          /* NULL = untagged ("@"), otherwise BCP 47 tag */
    char *content;
} csvs_prose;

struct csvs_entry {
    char *base;
    char *base_value;    /* NULL when absent (query without value) */
    char *leader_value;  /* NULL when absent */
    csvs_leaf *leaves;   /* sorted array of leaf groups */
    size_t nleaves;
    size_t leaves_cap;
    csvs_prose *prose;   /* array of prose blobs */
    size_t nprose;
    size_t prose_cap;
};
```

Leaves stored as a **sorted array** (by `name`), looked up via binary search.
This matches the on-disk sort order of tablets and is cache-friendly.
Insertions into a small sorted array (typically <20 leaves) are fast with memmove.

### Grain

```c
typedef struct {
    char *base;
    char *base_value;    /* may be NULL */
    char *leaf;
    char *leaf_value;    /* may be NULL */
} csvs_grain;
```

### Schema

```c
typedef struct {
    char *name;
    char **trunks;  size_t ntrunks;  size_t trunks_cap;
    char **leaves;  size_t nleaves;  size_t leaves_cap;
} csvs_branch;

typedef struct {
    csvs_branch *branches; /* sorted array by name */
    size_t nbranches;
    size_t cap;
} csvs_schema;
```

Also sorted array with binary search. Schema is small (typically <30 branches).

### Dataset

```c
typedef struct {
    char *dir;              /* dataset directory path (owned) */
    csvs_schema *schema;    /* NULL = not cached; load on demand */
} csvs_dataset;
```

### Iterator

```c
typedef struct csvs_iter csvs_iter; /* opaque */
```

Iterator owns its current result entry. The pointer returned by `csvs_next()`
is valid until the next `csvs_next()` call or `csvs_iter_free()` (readdir
semantics). Caller must copy if they need to keep it.

## Public API (`csvs.h`)

```c
/* lifecycle */
csvs_dataset *csvs_open(const char *dir);
csvs_dataset *csvs_create(const char *dir, int bare);
void           csvs_close(csvs_dataset *ds);

/* schema */
csvs_schema *csvs_schema_load(csvs_dataset *ds);    /* caches on ds */
void          csvs_schema_free(csvs_schema *s);      /* only if you detach */

/* read */
csvs_iter       *csvs_select(csvs_dataset *ds, const csvs_entry *queries, size_t n, int light, int prose);
const csvs_entry *csvs_next(csvs_iter *it);
void              csvs_iter_free(csvs_iter *it);

/* write */
int csvs_update(csvs_dataset *ds, const csvs_entry *queries, size_t n);
int csvs_insert(csvs_dataset *ds, const csvs_entry *queries, size_t n);
int csvs_delete(csvs_dataset *ds, const csvs_entry *queries, size_t n);

/* entry helpers (pure, no I/O) */
csvs_grain *csvs_mow(const csvs_entry *e, const char *trait_, const char *thing, size_t *nout);
csvs_entry  csvs_sow(const csvs_entry *e, const csvs_grain *g, const char *trait_, const char *thing);
csvs_schema csvs_to_schema(const csvs_entry *schema_record);
void         csvs_sort_ascending(const csvs_schema *s, char **names, size_t n);
void         csvs_sort_descending(const csvs_schema *s, char **names, size_t n);
int          csvs_nesting_level(const csvs_schema *s, const char *branch);

/* entry lifecycle */
csvs_entry csvs_entry_new(const char *base);
void        csvs_entry_free(csvs_entry *e);
csvs_entry csvs_entry_clone(const csvs_entry *e);

/* grain lifecycle */
void csvs_grain_free(csvs_grain *g);

/* error */
const char *csvs_errmsg(void);   /* thread-local last error */
```

Return convention: functions return 0 on success, -1 on error.
`csvs_errmsg()` returns a thread-local static string describing the last error.

## File layout

```
c/
  Makefile
  csvs.h              public header
  csvs_entry.c        Entry struct, mow, sow, clone, free
  csvs_grain.c        Grain struct, free
  csvs_schema.c       Schema struct, to_schema, nesting, crown, sort, free
  csvs_line.c         CSV line read/write via libcsv, escape/unescape newlines
  csvs_dataset.c      Dataset handle, open, create, close, schema caching
  csvs_select.c       select iterator orchestrator (dispatches to query/option/build)
  csvs_query.c        query iterator, plan_query, tablet cursor, key groups
  csvs_option.c       option iterator, plan_options, tablet cursor
  csvs_build.c        build_record, plan_build, tablet scan
  csvs_update.c       update_record, plan_update, merge-insert into tablet
  csvs_insert.c       insert_record, plan_insert, append + sort tablet
  csvs_delete.c       delete_record, plan_delete, prune tablet
  csvs_version.c      .csvs.csv read/write
  csvs_init.c         create empty dataset
  csvs_prose.c        prose store: read/write/search blobs, uri encode/decode
  csvs_internal.h     shared internal declarations (not installed)
  csvs_vec.h          generic growable array macros (internal)
  lib/
    libcsv/           vendored rgamble/libcsv
  tests/
    test_main.c       test harness: reads JSON cases, runs against libcsvs
    cjson/            vendored cJSON (test only)
    Makefile          test build
```

## Build

Plain Makefile. Two targets:

- `make` builds `libcsvs.a` (static) and `libcsvs.so` (shared)
- `make test` builds and runs the test harness against `../test/`

```makefile
CC       = cc
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -g -O2
AR       = ar
ARFLAGS  = rcs

SRCS     = $(wildcard csvs_*.c)
OBJS     = $(SRCS:.c=.o) lib/libcsv/libcsv.o
```

pkg-config file `csvs.pc` for downstream consumers.

WASI cross-compile target deferred to a follow-up; the code avoids mmap,
signals, fork, and locale-dependent functions.

## Iterator architecture

The select iterator is the top-level orchestrator. Internally it composes:

1. **For queries with leaves** (filter conditions): query iterator
   - Plans a sequence of tablets via `plan_query`
   - Each tablet is scanned with a `FILE*` cursor, yielding key groups
   - Key groups are matched against grains (with regex for query tablets)
   - Multi-tablet join uses a counter + state stack (same as RS/JS)
   - Yields light entries; if `!light`, each is enriched by build

2. **For queries without leaves** (enumerate values): option iterator
   - Plans tablets via `plan_options`
   - Scans tablet, collects unique values seen across dataset
   - Yields one entry per unique value

3. **Build** (enrich a light entry): not an iterator, called per-entry
   - Plans tablets via `plan_build`
   - Scans each tablet linearly, sowing grains into the entry

4. **Special bases**: `_` returns schema record, `.` returns version record

Each tablet cursor holds:
```c
struct tablet_cursor {
    FILE *fp;
    /* libcsv parser state */
    struct csv_parser csv;
    /* current parsed line */
    char *key;
    char *value;
    /* key-group accumulation */
    char *current_group_key;
    char **group_values;
    size_t ngroup_values;
    /* EOF flag */
    int done;
};
```

The pull proceeds: `csvs_next()` -> select pulls from query/option iterator ->
query/option pulls from tablet cursor -> tablet cursor reads next line from `FILE*`.
Each layer only reads as far as needed to produce one result.

## Implementation order

### Phase 1: foundation (no I/O)
1. `csvs_vec.h` — generic growable array macros
2. `csvs_entry.c` — Entry new/clone/free, leaf insert/find (sorted array)
3. `csvs_grain.c` — Grain new/free
4. `csvs_schema.c` — Schema new/free, `to_schema`, `is_connected`, `find_crown`, `nesting_level`, sort ascending/descending
5. `csvs_entry.c` — mow, sow (pure functions)

Test: wire up cJSON, run `grain.json`, `entry.json`, `schema.json`, `to_schema.json`, `get_nesting_level.json`, `sort_ascending.json`, `sort_descending.json`, `mow.json`, `sow.json` cases.

### Phase 2: CSV I/O layer
6. `csvs_line.c` — parse/write CSV lines via libcsv, escape/unescape newlines
7. `csvs_version.c` — select_version, update_version
8. `csvs_init.c` — create dataset
9. `csvs_dataset.c` — open, close, schema_load (caching)

Test: run `init.json` cases.

### Phase 3: read path
10. `csvs_query.c` — key_groups, plan_query, query tablet cursor, query iterator
11. `csvs_build.c` — plan_build, build_tablet, build_record
12. `csvs_option.c` — plan_options, option tablet cursor, option iterator
13. `csvs_select.c` — select iterator orchestrator
14. `csvs_prose.c` — read_prose, search_prose, uri encode/decode

Test: run `select.json` cases (including prose select cases).

### Phase 4: write path
15. `csvs_insert.c` — plan_insert, insert_tablet, sort_file
16. `csvs_update.c` — plan_update, update_tablet (merge-insert), sort_file
17. `csvs_delete.c` — plan_delete, prune_tablet
18. `csvs_prose.c` — write_prose

Test: run `insert.json`, `update.json`, `delete.json`, `prose.json` cases.

### Phase 5: packaging
19. Makefile polish, `csvs.pc`, install target
20. Test under WASI (wasmtime) — identify and fix any POSIX gaps

## Test harness design

`test_main.c` links against both `libcsvs.a` and cJSON. For each test case file:

1. Parse `cases/{name}.json` with cJSON
2. For each test case object:
   - Copy `datasets/{initial}/` to a temp directory
   - Parse query record names through `records/{name}.json` -> cJSON -> `csvs_entry`
   - Call the operation (`csvs_select`, `csvs_update`, etc.)
   - Compare results against expected (SON comparison for reads, directory diff for writes)
3. Clean up temp directory

JSON<->Entry conversion lives in the test harness only, not in libcsvs.
This keeps the library free of JSON dependencies.

## Risks

- **Regex in WASI**: POSIX `<regex.h>` may not be available in WASI preview 1.
  Mitigation: the regex usage is simple (ERE literals, `.*`, `|`). Can shim with
  a tiny matcher if needed. Defer until phase 5.

- **Memory management**: C has no RAII. Every allocation needs a matching free.
  Mitigation: consistent ownership rules (entry owns its strings and children;
  iterator owns its current result; caller frees what they allocate).
  Valgrind on the test suite.

- **Sort stability**: tablet sort must match Rust/JS byte order (ADR-0001).
  Mitigation: use `strcmp` after unescape (C byte order = Rust `String::cmp`).
