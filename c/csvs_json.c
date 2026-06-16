/*
 * csvs - JSON conversion (SON format ↔ csvs structs)
 *
 * Parallel to entry/try_from.rs + entry/into_value.rs,
 * grain/try_from.rs + grain/into_value.rs,
 * schema/try_from.rs in the Rust implementation.
 */

#include "csvs_internal.h"
#include <cJSON.h>

/* ── csvs_entry_from_json: SON JSON → csvs_entry ─────────────────── */

csvs_entry csvs_entry_from_json(const cJSON *j)
{
    csvs_entry e;
    memset(&e, 0, sizeof(e));

    if (!cJSON_IsObject(j)) {
        e.base = strdup("");
        return e;
    }

    cJSON *base_j = cJSON_GetObjectItemCaseSensitive(j, "_");
    const char *base = (base_j && cJSON_IsString(base_j)) ? base_j->valuestring : "";
    e.base = strdup(base);

    cJSON *bv_j = cJSON_GetObjectItemCaseSensitive(j, base);
    if (bv_j && cJSON_IsString(bv_j))
        e.base_value = strdup(bv_j->valuestring);

    cJSON *lv_j = cJSON_GetObjectItemCaseSensitive(j, "__");
    if (lv_j && cJSON_IsString(lv_j))
        e.leader_value = strdup(lv_j->valuestring);

    cJSON *child;
    cJSON_ArrayForEach(child, j) {
        const char *key = child->string;
        if (!key) continue;
        if (strcmp(key, "_") == 0) continue;
        if (strcmp(key, base) == 0) continue;
        if (strcmp(key, "__") == 0) continue;

        /* prose keys */
        if (key[0] == '@') {
            if (cJSON_IsString(child)) {
                const char *lang = (strlen(key) == 1) ? NULL : key + 1;
                csvs_entry_add_prose(&e, lang, child->valuestring);
            }
            continue;
        }

        csvs_leaf *lf = csvs_entry_add_leaf(&e, key);

        if (cJSON_IsString(child)) {
            csvs_entry ce = csvs_entry_new(key);
            ce.base_value = strdup(child->valuestring);
            csvs_leaf_push(lf, ce);
        } else if (cJSON_IsArray(child)) {
            cJSON *item;
            cJSON_ArrayForEach(item, child) {
                if (cJSON_IsString(item)) {
                    csvs_entry ce = csvs_entry_new(key);
                    ce.base_value = strdup(item->valuestring);
                    csvs_leaf_push(lf, ce);
                } else if (cJSON_IsObject(item)) {
                    csvs_entry ce = csvs_entry_from_json(item);
                    csvs_leaf_push(lf, ce);
                }
            }
        } else if (cJSON_IsObject(child)) {
            csvs_entry ce = csvs_entry_from_json(child);
            csvs_leaf_push(lf, ce);
        }
    }

    return e;
}

/* ── csvs_entry_to_json: csvs_entry → SON JSON ──────────────────── */

cJSON *csvs_entry_to_json(const csvs_entry *e)
{
    cJSON *j = cJSON_CreateObject();

    cJSON_AddStringToObject(j, "_", e->base ? e->base : "");

    if (e->base_value)
        cJSON_AddStringToObject(j, e->base, e->base_value);

    if (e->leader_value)
        cJSON_AddStringToObject(j, "__", e->leader_value);

    /* prose */
    for (size_t i = 0; i < e->nprose; i++) {
        char keybuf[64];
        const char *key;
        if (e->prose[i].lang) {
            snprintf(keybuf, sizeof(keybuf), "@%s", e->prose[i].lang);
            key = keybuf;
        } else {
            key = "@";
        }
        cJSON_AddStringToObject(j, key, e->prose[i].content);
    }

    /* leaves */
    for (size_t i = 0; i < e->nleaves; i++) {
        const csvs_leaf *lf = &e->leaves[i];
        if (lf->nentries == 0) continue;

        if (lf->nentries == 1) {
            const csvs_entry *child = &lf->entries[0];
            if (child->nleaves == 0 && child->nprose == 0) {
                if (child->base_value)
                    cJSON_AddStringToObject(j, lf->name, child->base_value);
            } else {
                cJSON_AddItemToObject(j, lf->name, csvs_entry_to_json(child));
            }
        } else {
            cJSON *arr = cJSON_CreateArray();
            for (size_t k = 0; k < lf->nentries; k++) {
                const csvs_entry *child = &lf->entries[k];
                if (child->nleaves == 0 && child->nprose == 0) {
                    if (child->base_value)
                        cJSON_AddItemToArray(arr, cJSON_CreateString(child->base_value));
                } else {
                    cJSON_AddItemToArray(arr, csvs_entry_to_json(child));
                }
            }
            cJSON_AddItemToObject(j, lf->name, arr);
        }
    }

    return j;
}

/* ── csvs_grain_from_json: SON JSON → csvs_grain ─────────────────── */

csvs_grain csvs_grain_from_json(const cJSON *j)
{
    csvs_grain g;
    memset(&g, 0, sizeof(g));

    cJSON *base_j = cJSON_GetObjectItemCaseSensitive(j, "_");
    if (!base_j || !cJSON_IsString(base_j)) return g;
    g.base = strdup(base_j->valuestring);

    cJSON *bv_j = cJSON_GetObjectItemCaseSensitive(j, g.base);
    if (bv_j && cJSON_IsString(bv_j))
        g.base_value = strdup(bv_j->valuestring);

    /* find the leaf key: not "_" and not base */
    cJSON *child;
    cJSON_ArrayForEach(child, j) {
        if (!child->string) continue;
        if (strcmp(child->string, "_") == 0) continue;
        if (strcmp(child->string, g.base) == 0) continue;
        g.leaf = strdup(child->string);
        if (cJSON_IsString(child))
            g.leaf_value = strdup(child->valuestring);
        break;
    }

    return g;
}

/* ── csvs_grain_to_json: csvs_grain → SON JSON ──────────────────── */

cJSON *csvs_grain_to_json(const csvs_grain *g)
{
    cJSON *j = cJSON_CreateObject();
    cJSON_AddStringToObject(j, "_", g->base ? g->base : "");
    if (g->base_value)
        cJSON_AddStringToObject(j, g->base, g->base_value);
    if (g->leaf && g->leaf_value)
        cJSON_AddStringToObject(j, g->leaf, g->leaf_value);
    return j;
}

/* ── csvs_schema_to_json: csvs_schema → JSON ────────────────────── */

cJSON *csvs_schema_to_json(const csvs_schema *s)
{
    cJSON *j = cJSON_CreateObject();
    for (size_t i = 0; i < s->nbranches; i++) {
        const csvs_branch *b = &s->branches[i];
        cJSON *obj = cJSON_CreateObject();

        cJSON *trunks = cJSON_CreateArray();
        for (size_t k = 0; k < b->ntrunks; k++)
            cJSON_AddItemToArray(trunks, cJSON_CreateString(b->trunks[k]));
        cJSON_AddItemToObject(obj, "trunks", trunks);

        cJSON *leaves = cJSON_CreateArray();
        for (size_t k = 0; k < b->nleaves; k++)
            cJSON_AddItemToArray(leaves, cJSON_CreateString(b->leaves[k]));
        cJSON_AddItemToObject(obj, "leaves", leaves);

        cJSON_AddItemToObject(j, b->name, obj);
    }
    return j;
}
