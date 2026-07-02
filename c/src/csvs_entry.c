/*
 * csvs - Entry struct, lifecycle, leaf manipulation, mow, sow
 */

#include "csvs_internal.h"

/* ── Helpers ──────────────────────────────────────────────────────── */

static csvs_leaf csvs_leaf_new(const char *name)
{
    csvs_leaf lf;
    memset(&lf, 0, sizeof(lf));
    lf.name = csvs_strdup(name);
    return lf;
}

static csvs_leaf csvs_leaf_clone(const csvs_leaf *lf)
{
    csvs_leaf out;
    out.name     = csvs_strdup(lf->name);
    out.nentries = lf->nentries;
    out.cap      = lf->nentries;
    out.entries  = NULL;
    if (lf->nentries > 0) {
        out.entries = malloc(out.cap * sizeof(csvs_entry));
        for (size_t i = 0; i < lf->nentries; i++)
            out.entries[i] = csvs_entry_clone(&lf->entries[i]);
    }
    return out;
}

static void csvs_leaf_free_contents(csvs_leaf *lf)
{
    if (!lf) return;
    free(lf->name);
    for (size_t i = 0; i < lf->nentries; i++)
        csvs_entry_free(&lf->entries[i]);
    free(lf->entries);
}

static csvs_prose csvs_prose_clone_one(const csvs_prose *p)
{
    csvs_prose out;
    out.lang    = csvs_strdup(p->lang);
    out.content = csvs_strdup(p->content);
    return out;
}

static void csvs_prose_free_one(csvs_prose *p)
{
    if (!p) return;
    free(p->lang);
    free(p->content);
}

/* ── Entry lifecycle ──────────────────────────────────────────────── */

csvs_entry csvs_entry_new(const char *base)
{
    csvs_entry e;
    memset(&e, 0, sizeof(e));
    e.base = csvs_strdup(base);
    return e;
}

csvs_entry csvs_entry_clone(const csvs_entry *e)
{
    csvs_entry out;
    memset(&out, 0, sizeof(out));
    out.base         = csvs_strdup(e->base);
    out.base_value   = csvs_strdup(e->base_value);
    out.leader_value = csvs_strdup(e->leader_value);

    /* clone leaves */
    out.nleaves   = e->nleaves;
    out.leaves_cap = e->nleaves;
    out.leaves    = NULL;
    if (e->nleaves > 0) {
        out.leaves = malloc(out.leaves_cap * sizeof(csvs_leaf));
        for (size_t i = 0; i < e->nleaves; i++)
            out.leaves[i] = csvs_leaf_clone(&e->leaves[i]);
    }

    /* clone prose */
    out.nprose    = e->nprose;
    out.prose_cap = e->nprose;
    out.prose     = NULL;
    if (e->nprose > 0) {
        out.prose = malloc(out.prose_cap * sizeof(csvs_prose));
        for (size_t i = 0; i < e->nprose; i++)
            out.prose[i] = csvs_prose_clone_one(&e->prose[i]);
    }

    return out;
}

void csvs_entry_free(csvs_entry *e)
{
    if (!e) return;
    free(e->base);         e->base = NULL;
    free(e->base_value);   e->base_value = NULL;
    free(e->leader_value); e->leader_value = NULL;

    for (size_t i = 0; i < e->nleaves; i++)
        csvs_leaf_free_contents(&e->leaves[i]);
    free(e->leaves); e->leaves = NULL;
    e->nleaves = e->leaves_cap = 0;

    for (size_t i = 0; i < e->nprose; i++)
        csvs_prose_free_one(&e->prose[i]);
    free(e->prose); e->prose = NULL;
    e->nprose = e->prose_cap = 0;
}

/* ── Sorted leaf array: binary search ─────────────────────────────── */

/*
 * Binary search for a leaf by name.
 * Returns the index where the name is or would be inserted.
 * Sets *found to 1 if exact match, 0 otherwise.
 */
static size_t leaf_bsearch(const csvs_entry *e, const char *name, int *found)
{
    size_t lo = 0, hi = e->nleaves;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int cmp = strcmp(e->leaves[mid].name, name);
        if (cmp < 0)      lo = mid + 1;
        else if (cmp > 0)  hi = mid;
        else { *found = 1; return mid; }
    }
    *found = 0;
    return lo;
}

csvs_leaf *csvs_entry_find_leaf(const csvs_entry *e, const char *name)
{
    if (!e || !e->leaves || e->nleaves == 0) return NULL;
    int found = 0;
    size_t idx = leaf_bsearch(e, name, &found);
    return found ? &e->leaves[idx] : NULL;
}

csvs_leaf *csvs_entry_add_leaf(csvs_entry *e, const char *name)
{
    int found = 0;
    size_t idx = leaf_bsearch(e, name, &found);
    if (found) return &e->leaves[idx];

    /* grow if needed */
    if (e->nleaves >= e->leaves_cap) {
        size_t nc = e->leaves_cap ? e->leaves_cap * 2 : 4;
        csvs_leaf *np = realloc(e->leaves, nc * sizeof(csvs_leaf));
        if (!np) return NULL;
        e->leaves = np;
        e->leaves_cap = nc;
    }

    /* shift right to make room */
    if (idx < e->nleaves)
        memmove(&e->leaves[idx + 1], &e->leaves[idx],
                (e->nleaves - idx) * sizeof(csvs_leaf));

    e->leaves[idx] = csvs_leaf_new(name);
    e->nleaves++;
    return &e->leaves[idx];
}

void csvs_leaf_push(csvs_leaf *lf, csvs_entry child)
{
    VEC_PUSH(lf->entries, lf->nentries, lf->cap, child);
}

/* ── Prose manipulation ───────────────────────────────────────────── */

void csvs_entry_add_prose(csvs_entry *e, const char *lang, const char *content)
{
    /* check if this lang already exists, overwrite */
    for (size_t i = 0; i < e->nprose; i++) {
        int match = (lang == NULL && e->prose[i].lang == NULL) ||
                    (lang && e->prose[i].lang && strcmp(lang, e->prose[i].lang) == 0);
        if (match) {
            free(e->prose[i].content);
            e->prose[i].content = csvs_strdup(content);
            return;
        }
    }
    csvs_prose p;
    p.lang    = csvs_strdup(lang);
    p.content = csvs_strdup(content);
    VEC_PUSH(e->prose, e->nprose, e->prose_cap, p);
}

const char *csvs_entry_get_prose(const csvs_entry *e, const char *lang)
{
    for (size_t i = 0; i < e->nprose; i++) {
        int match = (lang == NULL && e->prose[i].lang == NULL) ||
                    (lang && e->prose[i].lang && strcmp(lang, e->prose[i].lang) == 0);
        if (match) return e->prose[i].content;
    }
    return NULL;
}

/* ── Mow: decompose entry into grains for a given relationship ───── */

static void grains_push(csvs_grain **arr, size_t *len, size_t *cap,
                        csvs_grain g)
{
    VEC_PUSH(*arr, *len, *cap, g);
}

/*
 * mow when base == thing
 * Returns grains: { base, base_value, leaf=trait_, leaf_value=trait_values }
 */
static void mow_base_is_thing(const csvs_entry *e, const char *trait_,
                               const char *thing __attribute__((unused)),
                               csvs_grain **out, size_t *nout, size_t *cap)
{
    /* trait_ == base (self-referential) */
    if (strcmp(trait_, e->base) == 0) {
        grains_push(out, nout, cap,
            csvs_grain_new(e->base, e->base_value, trait_, e->base_value));
        return;
    }

    csvs_leaf *lf = csvs_entry_find_leaf(e, trait_);
    if (!lf || lf->nentries == 0) {
        /* no leaf values for trait_ — but we still need to produce
           a grain with the base value if it exists */
        return;
    }

    for (size_t i = 0; i < lf->nentries; i++) {
        grains_push(out, nout, cap,
            csvs_grain_new(e->base, e->base_value,
                           trait_, lf->entries[i].base_value));
    }
}

/*
 * mow when base == trait_
 * Returns grains: { base, base_value, leaf=thing, leaf_value=thing_values }
 */
static void mow_base_is_trait(const csvs_entry *e, const char *trait_,
                               const char *thing,
                               csvs_grain **out, size_t *nout, size_t *cap)
{
    csvs_leaf *lf = csvs_entry_find_leaf(e, thing);
    if (!lf || lf->nentries == 0) {
        /* no thing values — return grain with NULL leaf_value */
        grains_push(out, nout, cap,
            csvs_grain_new(trait_, e->base_value, thing, NULL));
        return;
    }

    for (size_t i = 0; i < lf->nentries; i++) {
        grains_push(out, nout, cap,
            csvs_grain_new(e->base, e->base_value,
                           thing, lf->entries[i].base_value));
    }
}

/*
 * mow when entry has trait_ as a direct leaf key
 * Trait is an object within the record.
 */
static void mow_trait_is_object(const csvs_entry *e, const char *trait_,
                                 const char *thing,
                                 csvs_grain **out, size_t *nout, size_t *cap)
{
    csvs_leaf *trait_leaf = csvs_entry_find_leaf(e, trait_);
    if (!trait_leaf) return;

    for (size_t i = 0; i < trait_leaf->nentries; i++) {
        csvs_entry *trunk_item = &trait_leaf->entries[i];
        const char *trunk_value = trunk_item->base_value;

        csvs_leaf *thing_leaf = csvs_entry_find_leaf(trunk_item, thing);
        if (thing_leaf && thing_leaf->nentries > 0) {
            for (size_t j = 0; j < thing_leaf->nentries; j++) {
                grains_push(out, nout, cap,
                    csvs_grain_new(trait_, trunk_value,
                                   thing, thing_leaf->entries[j].base_value));
            }
        } else {
            /* trunk item has no thing values */
            grains_push(out, nout, cap,
                csvs_grain_new(trait_, trunk_value, thing, NULL));
        }
    }
}

/*
 * mow when trait_ is deeper in nested objects — recurse into leaf entries
 */
static void mow_trait_is_nested(const csvs_entry *e, const char *trait_,
                                 const char *thing,
                                 csvs_grain **out, size_t *nout, size_t *cap)
{
    for (size_t i = 0; i < e->nleaves; i++) {
        csvs_leaf *lf = &e->leaves[i];
        for (size_t j = 0; j < lf->nentries; j++) {
            /* recurse: mow on each leaf item */
            size_t sub_n = 0, sub_cap = 0;
            csvs_grain *sub = NULL;
            /* full mow dispatch on the child */
            csvs_entry *child = &lf->entries[j];

            if (strcmp(child->base, thing) == 0)
                mow_base_is_thing(child, trait_, thing, &sub, &sub_n, &sub_cap);
            else if (strcmp(child->base, trait_) == 0)
                mow_base_is_trait(child, trait_, thing, &sub, &sub_n, &sub_cap);
            else if (csvs_entry_find_leaf(child, trait_))
                mow_trait_is_object(child, trait_, thing, &sub, &sub_n, &sub_cap);
            else
                mow_trait_is_nested(child, trait_, thing, &sub, &sub_n, &sub_cap);

            for (size_t k = 0; k < sub_n; k++)
                grains_push(out, nout, cap, sub[k]);
            free(sub);
        }
    }
}

csvs_grain *csvs_mow(const csvs_entry *e, const char *trait_,
                      const char *thing, size_t *nout)
{
    csvs_grain *out = NULL;
    size_t n = 0, cap = 0;

    if (strcmp(e->base, thing) == 0)
        mow_base_is_thing(e, trait_, thing, &out, &n, &cap);
    else if (strcmp(e->base, trait_) == 0)
        mow_base_is_trait(e, trait_, thing, &out, &n, &cap);
    else if (csvs_entry_find_leaf(e, trait_))
        mow_trait_is_object(e, trait_, thing, &out, &n, &cap);
    else
        mow_trait_is_nested(e, trait_, thing, &out, &n, &cap);

    *nout = n;
    return out;
}

/* ── Sow: inject a grain back into an entry ──────────────────────── */

static csvs_entry sow_base_is_thing(const csvs_entry *e, const csvs_grain *g,
                                     const char *trait_ __attribute__((unused)),
                                     const char *thing __attribute__((unused)))
{
    csvs_entry out = csvs_entry_clone(e);

    /* if grain has no base_value, nothing to do */
    if (!g->base_value) return out;

    /* if entry already has a base_value, don't overwrite */
    if (out.base_value) return out;

    out.base_value = csvs_strdup(g->base_value);
    return out;
}

static csvs_entry sow_base_is_trait(const csvs_entry *e, const csvs_grain *g,
                                     const char *trait_ __attribute__((unused)),
                                     const char *thing)
{
    csvs_entry out = csvs_entry_clone(e);

    /* if grain has no leaf_value, nothing to do */
    if (!g->leaf_value) return out;

    csvs_leaf *lf = csvs_entry_add_leaf(&out, thing);
    csvs_entry child = csvs_entry_new(thing);
    child.base_value = csvs_strdup(g->leaf_value);
    csvs_leaf_push(lf, child);

    return out;
}

static csvs_entry sow_trait_is_object(const csvs_entry *e, const csvs_grain *g,
                                       const char *trait_, const char *thing)
{
    csvs_entry out = csvs_entry_clone(e);

    csvs_leaf *trait_leaf = csvs_entry_find_leaf(&out, trait_);
    if (!trait_leaf) return out;

    for (size_t i = 0; i < trait_leaf->nentries; i++) {
        csvs_entry *trunk_item = &trait_leaf->entries[i];

        /* check if this trunk item matches the grain's base_value */
        int is_match = 0;
        if (g->base_value && trunk_item->base_value)
            is_match = strcmp(trunk_item->base_value, g->base_value) == 0;
        else if (!g->base_value && !trunk_item->base_value)
            is_match = 1;

        if (is_match) {
            csvs_leaf *thing_leaf = csvs_entry_add_leaf(trunk_item, thing);
            csvs_entry child = csvs_entry_new(g->leaf);
            child.base_value = csvs_strdup(g->leaf_value);
            csvs_leaf_push(thing_leaf, child);
        }
    }

    return out;
}

static csvs_entry sow_trait_is_nested(const csvs_entry *e, const csvs_grain *g,
                                       const char *trait_, const char *thing)
{
    csvs_entry out;
    memset(&out, 0, sizeof(out));
    out.base         = csvs_strdup(e->base);
    out.base_value   = csvs_strdup(e->base_value);
    out.leader_value = csvs_strdup(e->leader_value);

    /* clone prose */
    out.nprose    = e->nprose;
    out.prose_cap = e->nprose;
    out.prose     = NULL;
    if (e->nprose > 0) {
        out.prose = malloc(out.prose_cap * sizeof(csvs_prose));
        for (size_t i = 0; i < e->nprose; i++)
            out.prose[i] = csvs_prose_clone_one(&e->prose[i]);
    }

    /* recurse into each leaf */
    out.nleaves   = e->nleaves;
    out.leaves_cap = e->nleaves;
    out.leaves    = NULL;
    if (e->nleaves > 0) {
        out.leaves = malloc(out.leaves_cap * sizeof(csvs_leaf));
        for (size_t i = 0; i < e->nleaves; i++) {
            const csvs_leaf *src = &e->leaves[i];
            csvs_leaf dst;
            dst.name     = csvs_strdup(src->name);
            dst.nentries = src->nentries;
            dst.cap      = src->nentries;
            dst.entries  = NULL;
            if (src->nentries > 0) {
                dst.entries = malloc(dst.cap * sizeof(csvs_entry));
                for (size_t j = 0; j < src->nentries; j++) {
                    /* recurse sow into objects only */
                    if (src->entries[j].nleaves > 0) {
                        dst.entries[j] = csvs_sow(&src->entries[j], g, trait_, thing);
                    } else {
                        dst.entries[j] = csvs_entry_clone(&src->entries[j]);
                    }
                }
            }
            out.leaves[i] = dst;
        }
    }

    return out;
}

csvs_entry csvs_sow(const csvs_entry *e, const csvs_grain *g,
                     const char *trait_, const char *thing)
{
    if (strcmp(e->base, thing) == 0)
        return sow_base_is_thing(e, g, trait_, thing);

    if (strcmp(e->base, trait_) == 0)
        return sow_base_is_trait(e, g, trait_, thing);

    if (csvs_entry_find_leaf(e, trait_))
        return sow_trait_is_object(e, g, trait_, thing);

    return sow_trait_is_nested(e, g, trait_, thing);
}
