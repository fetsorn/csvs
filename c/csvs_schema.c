/*
 * csvs - Schema struct, to_schema, nesting level, crown, sort
 */

#include "csvs_internal.h"

/* ── Schema lifecycle ─────────────────────────────────────────────── */

csvs_schema csvs_schema_new(void)
{
    csvs_schema s;
    memset(&s, 0, sizeof(s));
    return s;
}

static csvs_branch csvs_branch_new(const char *name)
{
    csvs_branch b;
    memset(&b, 0, sizeof(b));
    b.name = csvs_strdup(name);
    return b;
}

static csvs_branch csvs_branch_clone(const csvs_branch *b)
{
    csvs_branch out;
    out.name = csvs_strdup(b->name);

    out.ntrunks   = b->ntrunks;
    out.trunks_cap = b->ntrunks;
    out.trunks    = NULL;
    if (b->ntrunks > 0) {
        out.trunks = malloc(out.trunks_cap * sizeof(char *));
        for (size_t i = 0; i < b->ntrunks; i++)
            out.trunks[i] = csvs_strdup(b->trunks[i]);
    }

    out.nleaves   = b->nleaves;
    out.leaves_cap = b->nleaves;
    out.leaves    = NULL;
    if (b->nleaves > 0) {
        out.leaves = malloc(out.leaves_cap * sizeof(char *));
        for (size_t i = 0; i < b->nleaves; i++)
            out.leaves[i] = csvs_strdup(b->leaves[i]);
    }

    return out;
}

static void csvs_branch_free(csvs_branch *b)
{
    if (!b) return;
    free(b->name);
    for (size_t i = 0; i < b->ntrunks; i++) free(b->trunks[i]);
    free(b->trunks);
    for (size_t i = 0; i < b->nleaves; i++) free(b->leaves[i]);
    free(b->leaves);
}

csvs_schema csvs_schema_clone(const csvs_schema *s)
{
    csvs_schema out;
    out.nbranches = s->nbranches;
    out.cap       = s->nbranches;
    out.branches  = NULL;
    if (s->nbranches > 0) {
        out.branches = malloc(out.cap * sizeof(csvs_branch));
        for (size_t i = 0; i < s->nbranches; i++)
            out.branches[i] = csvs_branch_clone(&s->branches[i]);
    }
    return out;
}

void csvs_schema_free(csvs_schema *s)
{
    if (!s) return;
    for (size_t i = 0; i < s->nbranches; i++)
        csvs_branch_free(&s->branches[i]);
    free(s->branches);
    s->branches = NULL;
    s->nbranches = s->cap = 0;
}

/* ── Binary search on sorted branch array ─────────────────────────── */

static size_t branch_bsearch(const csvs_schema *s, const char *name, int *found)
{
    size_t lo = 0, hi = s->nbranches;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int cmp = strcmp(s->branches[mid].name, name);
        if (cmp < 0)      lo = mid + 1;
        else if (cmp > 0)  hi = mid;
        else { *found = 1; return mid; }
    }
    *found = 0;
    return lo;
}

csvs_branch *csvs_schema_find(const csvs_schema *s, const char *name)
{
    if (!s || !s->branches || s->nbranches == 0) return NULL;
    int found = 0;
    size_t idx = branch_bsearch(s, name, &found);
    return found ? &s->branches[idx] : NULL;
}

csvs_branch *csvs_schema_add(csvs_schema *s, const char *name)
{
    int found = 0;
    size_t idx = branch_bsearch(s, name, &found);
    if (found) return &s->branches[idx];

    if (s->nbranches >= s->cap) {
        size_t nc = s->cap ? s->cap * 2 : 8;
        csvs_branch *np = realloc(s->branches, nc * sizeof(csvs_branch));
        if (!np) return NULL;
        s->branches = np;
        s->cap = nc;
    }

    if (idx < s->nbranches)
        memmove(&s->branches[idx + 1], &s->branches[idx],
                (s->nbranches - idx) * sizeof(csvs_branch));

    s->branches[idx] = csvs_branch_new(name);
    s->nbranches++;
    return &s->branches[idx];
}

/* ── Helper: add string to a string array if not already present ── */

static void strvec_push(char ***arr, size_t *len, size_t *cap, const char *s)
{
    /* check for duplicates */
    for (size_t i = 0; i < *len; i++)
        if (strcmp((*arr)[i], s) == 0) return;

    if (*len >= *cap) {
        size_t nc = *cap ? *cap * 2 : 4;
        char **np = realloc(*arr, nc * sizeof(char *));
        if (!np) return;
        *arr = np;
        *cap = nc;
    }
    (*arr)[(*len)++] = csvs_strdup(s);
}

/* ── to_schema: convert a schema SON record to Schema struct ──────── */

csvs_schema csvs_to_schema(const csvs_entry *rec)
{
    csvs_schema s = csvs_schema_new();

    /* Schema record must have base == "_" */
    if (!rec || !rec->base || strcmp(rec->base, "_") != 0)
        return s;

    /* Each leaf group in the record is a trunk->leaf relationship */
    for (size_t i = 0; i < rec->nleaves; i++) {
        const csvs_leaf *lf = &rec->leaves[i];
        const char *trunk = lf->name;

        for (size_t j = 0; j < lf->nentries; j++) {
            const char *leaf = lf->entries[j].base_value;
            if (!leaf) continue;

            /* Add trunk branch: append leaf to its leaves list */
            csvs_branch *tb = csvs_schema_add(&s, trunk);
            strvec_push(&tb->leaves, &tb->nleaves, &tb->leaves_cap, leaf);

            /* Add leaf branch: append trunk to its trunks list */
            csvs_branch *lb = csvs_schema_add(&s, leaf);
            strvec_push(&lb->trunks, &lb->ntrunks, &lb->trunks_cap, trunk);
        }
    }

    return s;
}

/* ── is_connected: check if branch is reachable from base ─────────── */

int csvs_is_connected(const csvs_schema *s, const char *base,
                      const char *branch)
{
    if (strcmp(base, branch) == 0) return 1;

    csvs_branch *b = csvs_schema_find(s, branch);
    if (!b) return 0;

    for (size_t i = 0; i < b->ntrunks; i++) {
        if (strcmp(b->trunks[i], base) == 0) return 1;
        if (csvs_is_connected(s, base, b->trunks[i])) return 1;
    }

    return 0;
}

/* ── find_crown: all branches connected to base ───────────────────── */

char **csvs_find_crown(const csvs_schema *s, const char *base, size_t *nout)
{
    char **out = NULL;
    size_t n = 0, cap = 0;

    for (size_t i = 0; i < s->nbranches; i++) {
        if (csvs_is_connected(s, base, s->branches[i].name)) {
            if (n >= cap) {
                size_t nc = cap ? cap * 2 : 8;
                char **np = realloc(out, nc * sizeof(char *));
                if (!np) break;
                out = np;
                cap = nc;
            }
            out[n++] = csvs_strdup(s->branches[i].name);
        }
    }

    *nout = n;
    return out;
}

/* ── nesting_level: distance from leaf nodes ──────────────────────── */

int csvs_nesting_level(const csvs_schema *s, const char *branch)
{
    csvs_branch *b = csvs_schema_find(s, branch);
    if (!b) return 0;

    int level = -1;
    for (size_t i = 0; i < b->ntrunks; i++) {
        int tl = csvs_nesting_level(s, b->trunks[i]);
        if (tl > level) level = tl;
    }

    return level + 1;
}

/* ── sort by nesting level ────────────────────────────────────────── */

/* Context for qsort_r-style comparisons.  Since qsort_r is not portable,
 * we use a static (thread-local) schema pointer for the comparator. */
static _Thread_local const csvs_schema *sort_schema;

static int cmp_ascending(const void *a, const void *b)
{
    const char *na = *(const char *const *)a;
    const char *nb = *(const char *const *)b;
    int la = csvs_nesting_level(sort_schema, na);
    int lb = csvs_nesting_level(sort_schema, nb);

    /* Higher nesting = first (descending by level = ascending by depth from leaves) */
    if (la > lb) return -1;
    if (la < lb) return  1;

    /* Tie-break: reverse alphabetic (b < a -> -1) to match JS */
    return strcmp(nb, na);
}

static int cmp_descending(const void *a, const void *b)
{
    const char *na = *(const char *const *)a;
    const char *nb = *(const char *const *)b;
    int la = csvs_nesting_level(sort_schema, na);
    int lb = csvs_nesting_level(sort_schema, nb);

    /* Lower nesting = first */
    if (la < lb) return -1;
    if (la > lb) return  1;

    /* Tie-break: alphabetic (a < b -> -1) to match JS */
    return strcmp(na, nb);
}

void csvs_sort_ascending(const csvs_schema *s, char **names, size_t n)
{
    sort_schema = s;
    qsort(names, n, sizeof(char *), cmp_ascending);
}

void csvs_sort_descending(const csvs_schema *s, char **names, size_t n)
{
    sort_schema = s;
    qsort(names, n, sizeof(char *), cmp_descending);
}
