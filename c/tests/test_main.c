/*
 * csvs test harness — reads JSON test cases from csvs-test and runs them
 * against libcsvs.
 */

#include "csvs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* csvs-test fixture loader */
#define CSVS_TEST_DIR "../test"
#include "csvs_test.h"

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define RUN_TEST(name, expr) do {                                    \
    tests_run++;                                                     \
    if (expr) {                                                      \
        tests_passed++;                                              \
    } else {                                                         \
        tests_failed++;                                              \
        fprintf(stderr, "  FAIL: %s\n", name);                      \
    }                                                                \
} while (0)

/* ── Entry struct JSON → csvs_entry ──────────────────────────────── */
/* This format is specific to test cases, not SON, so it stays here. */

static csvs_entry struct_json_to_entry(const cJSON *j)
{
    csvs_entry e;
    memset(&e, 0, sizeof(e));

    cJSON *base_j = cJSON_GetObjectItemCaseSensitive(j, "base");
    e.base = strdup((base_j && cJSON_IsString(base_j)) ? base_j->valuestring : "");

    cJSON *bv_j = cJSON_GetObjectItemCaseSensitive(j, "base_value");
    if (bv_j && cJSON_IsString(bv_j))
        e.base_value = strdup(bv_j->valuestring);

    cJSON *lv_j = cJSON_GetObjectItemCaseSensitive(j, "leader_value");
    if (lv_j && cJSON_IsString(lv_j))
        e.leader_value = strdup(lv_j->valuestring);

    cJSON *leaves_j = cJSON_GetObjectItemCaseSensitive(j, "leaves");
    if (leaves_j && cJSON_IsObject(leaves_j)) {
        cJSON *leaf_arr;
        cJSON_ArrayForEach(leaf_arr, leaves_j) {
            const char *name = leaf_arr->string;
            if (!name) continue;

            csvs_leaf *lf = csvs_entry_add_leaf(&e, name);
            if (cJSON_IsArray(leaf_arr)) {
                cJSON *item;
                cJSON_ArrayForEach(item, leaf_arr) {
                    csvs_entry ce = struct_json_to_entry(item);
                    csvs_leaf_push(lf, ce);
                }
            }
        }
    }

    return e;
}

/* ── Test: entry.json ─────────────────────────────────────────────── */

static void test_entry(void)
{
    printf("entry...\n");
    cJSON *cases = csvs_test_load_testcase("entry");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        cJSON *value_j = cJSON_GetObjectItemCaseSensitive(tc, "value");
        cJSON *entry_j = cJSON_GetObjectItemCaseSensitive(tc, "entry");

        csvs_entry value = csvs_entry_from_json(value_j);
        cJSON *roundtrip_j = csvs_entry_to_json(&value);

        int ok = csvs_test_json_equal(roundtrip_j, value_j);
        if (!ok) {
            char *got = cJSON_PrintUnformatted(roundtrip_j);
            char *exp = cJSON_PrintUnformatted(value_j);
            fprintf(stderr, "    entry roundtrip:\n      got:  %s\n      want: %s\n",
                    got, exp);
            free(got); free(exp);
        }
        RUN_TEST("entry roundtrip", ok);

        cJSON *exp_base_j = cJSON_GetObjectItemCaseSensitive(entry_j, "base");
        if (exp_base_j && cJSON_IsString(exp_base_j)) {
            int base_ok = strcmp(value.base, exp_base_j->valuestring) == 0;
            cJSON *exp_bv_j = cJSON_GetObjectItemCaseSensitive(entry_j, "base_value");
            if (exp_bv_j && cJSON_IsNull(exp_bv_j))
                base_ok = base_ok && (value.base_value == NULL);
            else if (exp_bv_j && cJSON_IsString(exp_bv_j))
                base_ok = base_ok && value.base_value &&
                          strcmp(value.base_value, exp_bv_j->valuestring) == 0;
            RUN_TEST("entry fields", base_ok);
        }

        cJSON_Delete(roundtrip_j);
        csvs_entry_free(&value);
    }

    cJSON_Delete(cases);
}

/* ── Test: grain.json ─────────────────────────────────────────────── */

static void test_grain(void)
{
    printf("grain...\n");
    cJSON *cases = csvs_test_load_testcase("grain");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        cJSON *value_j = cJSON_GetObjectItemCaseSensitive(tc, "value");
        cJSON *grain_j = cJSON_GetObjectItemCaseSensitive(tc, "grain");

        csvs_entry value = csvs_entry_from_json(value_j);

        cJSON *gb_j = cJSON_GetObjectItemCaseSensitive(grain_j, "base");
        cJSON *gbv_j = cJSON_GetObjectItemCaseSensitive(grain_j, "base_value");
        cJSON *gl_j = cJSON_GetObjectItemCaseSensitive(grain_j, "leaf");
        cJSON *glv_j = cJSON_GetObjectItemCaseSensitive(grain_j, "leaf_value");

        const char *gb = gb_j ? gb_j->valuestring : "";
        const char *gbv = (gbv_j && cJSON_IsString(gbv_j)) ? gbv_j->valuestring : NULL;
        const char *gl = gl_j ? gl_j->valuestring : "";
        const char *glv = (glv_j && cJSON_IsString(glv_j)) ? glv_j->valuestring : NULL;

        size_t ngrains;
        csvs_grain *grains = csvs_mow(&value, gb, gl, &ngrains);

        csvs_grain expected = csvs_grain_new(gb, gbv, gl, glv);

        int found = 0;
        for (size_t i = 0; i < ngrains; i++) {
            cJSON *got_j = csvs_grain_to_json(&grains[i]);
            cJSON *exp_j = csvs_grain_to_json(&expected);
            if (csvs_test_json_equal(got_j, exp_j)) found = 1;
            cJSON_Delete(got_j);
            cJSON_Delete(exp_j);
            if (found) break;
        }

        if (!found) {
            fprintf(stderr, "    grain: expected {%s,%s,%s,%s} not in %zu grains\n",
                    gb, gbv ? gbv : "null", gl, glv ? glv : "null", ngrains);
        }
        RUN_TEST("grain", found);

        csvs_grain_free(&expected);
        for (size_t i = 0; i < ngrains; i++) csvs_grain_free(&grains[i]);
        free(grains);
        csvs_entry_free(&value);
    }

    cJSON_Delete(cases);
}

/* ── Test: to_schema.json ─────────────────────────────────────────── */

static void test_to_schema(void)
{
    printf("to_schema...\n");
    cJSON *cases = csvs_test_load_testcase("to_schema");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        const char *name = cJSON_GetObjectItemCaseSensitive(tc, "name")->valuestring;
        const char *initial_name = cJSON_GetObjectItemCaseSensitive(tc, "initial")->valuestring;
        const char *expected_name = cJSON_GetObjectItemCaseSensitive(tc, "expected")->valuestring;

        cJSON *initial_j = csvs_test_load_record(initial_name);
        cJSON *expected_j = csvs_test_load_record(expected_name);
        if (!initial_j || !expected_j) {
            if (initial_j) cJSON_Delete(initial_j);
            if (expected_j) cJSON_Delete(expected_j);
            RUN_TEST(name, 0);
            continue;
        }

        csvs_entry initial = csvs_entry_from_json(initial_j);
        csvs_schema schema = csvs_to_schema(&initial);

        cJSON *schema_j = csvs_schema_to_json(&schema);

        int ok = csvs_test_json_equal(schema_j, expected_j);
        if (!ok) {
            char *got = cJSON_PrintUnformatted(schema_j);
            char *exp = cJSON_PrintUnformatted(expected_j);
            fprintf(stderr, "    to_schema %s:\n      got:  %s\n      want: %s\n",
                    name, got, exp);
            free(got); free(exp);
        }
        RUN_TEST(name, ok);

        cJSON_Delete(schema_j);
        csvs_schema_free(&schema);
        csvs_entry_free(&initial);
        cJSON_Delete(initial_j);
        cJSON_Delete(expected_j);
    }

    cJSON_Delete(cases);
}

/* ── Test: schema.json ────────────────────────────────────────────── */

static void test_schema(void)
{
    printf("schema...\n");
    cJSON *cases = csvs_test_load_testcase("schema");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        cJSON *entry_j = cJSON_GetObjectItemCaseSensitive(tc, "entry");
        cJSON *schema_j = cJSON_GetObjectItemCaseSensitive(tc, "schema");

        csvs_entry entry = struct_json_to_entry(entry_j);
        csvs_schema schema = csvs_to_schema(&entry);

        cJSON *got_j = csvs_schema_to_json(&schema);

        int ok = csvs_test_json_equal(got_j, schema_j);
        if (!ok) {
            char *got = cJSON_PrintUnformatted(got_j);
            char *exp = cJSON_PrintUnformatted(schema_j);
            fprintf(stderr, "    schema:\n      got:  %s\n      want: %s\n",
                    got, exp);
            free(got); free(exp);
        }
        RUN_TEST("schema", ok);

        cJSON_Delete(got_j);
        csvs_schema_free(&schema);
        csvs_entry_free(&entry);
    }

    cJSON_Delete(cases);
}

/* ── Test: get_nesting_level.json ─────────────────────────────────── */

static void test_get_nesting_level(void)
{
    printf("get_nesting_level...\n");
    cJSON *cases = csvs_test_load_testcase("get_nesting_level");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        const char *schema_name = cJSON_GetObjectItemCaseSensitive(tc, "schema")->valuestring;
        const char *branch = cJSON_GetObjectItemCaseSensitive(tc, "initial")->valuestring;
        int expected = (int)cJSON_GetObjectItemCaseSensitive(tc, "expected")->valuedouble;

        cJSON *schema_j = csvs_test_load_record(schema_name);
        csvs_entry schema_entry = csvs_entry_from_json(schema_j);
        csvs_schema schema = csvs_to_schema(&schema_entry);

        int got = csvs_nesting_level(&schema, branch);

        if (got != expected) {
            fprintf(stderr, "    nesting %s: got %d, want %d\n", branch, got, expected);
        }
        RUN_TEST(branch, got == expected);

        csvs_schema_free(&schema);
        csvs_entry_free(&schema_entry);
        cJSON_Delete(schema_j);
    }

    cJSON_Delete(cases);
}

/* ── Test: sort_ascending.json / sort_descending.json ─────────────── */

static void test_sort(const char *casename, int ascending)
{
    printf("%s...\n", casename);
    cJSON *cases = csvs_test_load_testcase(casename);
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        const char *schema_name = cJSON_GetObjectItemCaseSensitive(tc, "schema")->valuestring;
        cJSON *initial_j = cJSON_GetObjectItemCaseSensitive(tc, "initial");
        cJSON *expected_j = cJSON_GetObjectItemCaseSensitive(tc, "expected");

        cJSON *schema_j = csvs_test_load_record(schema_name);
        csvs_entry schema_entry = csvs_entry_from_json(schema_j);
        csvs_schema schema = csvs_to_schema(&schema_entry);

        int n = cJSON_GetArraySize(initial_j);
        char **names = malloc(n * sizeof(char *));
        for (int i = 0; i < n; i++)
            names[i] = strdup(cJSON_GetArrayItem(initial_j, i)->valuestring);

        if (ascending)
            csvs_sort_ascending(&schema, names, n);
        else
            csvs_sort_descending(&schema, names, n);

        int ok = 1;
        for (int i = 0; i < n; i++) {
            const char *exp = cJSON_GetArrayItem(expected_j, i)->valuestring;
            if (strcmp(names[i], exp) != 0) {
                fprintf(stderr, "    %s[%d]: got '%s', want '%s'\n",
                        casename, i, names[i], exp);
                ok = 0;
            }
        }
        RUN_TEST(casename, ok);

        for (int i = 0; i < n; i++) free(names[i]);
        free(names);
        csvs_schema_free(&schema);
        csvs_entry_free(&schema_entry);
        cJSON_Delete(schema_j);
    }

    cJSON_Delete(cases);
}

/* ── Test: mow.json ───────────────────────────────────────────────── */

static void test_mow(void)
{
    printf("mow...\n");
    cJSON *cases = csvs_test_load_testcase("mow");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        const char *name = cJSON_GetObjectItemCaseSensitive(tc, "name")->valuestring;
        const char *initial_name = cJSON_GetObjectItemCaseSensitive(tc, "initial")->valuestring;
        const char *trunk = cJSON_GetObjectItemCaseSensitive(tc, "trunk")->valuestring;
        const char *branch = cJSON_GetObjectItemCaseSensitive(tc, "branch")->valuestring;
        cJSON *expected_names = cJSON_GetObjectItemCaseSensitive(tc, "expected");

        cJSON *initial_j = csvs_test_load_record(initial_name);
        csvs_entry initial = csvs_entry_from_json(initial_j);

        size_t ngrains;
        csvs_grain *grains = csvs_mow(&initial, trunk, branch, &ngrains);

        int expected_n = cJSON_GetArraySize(expected_names);

        int ok = 1;
        for (int i = 0; i < expected_n; i++) {
            const char *exp_name = cJSON_GetArrayItem(expected_names, i)->valuestring;
            cJSON *exp_j = csvs_test_load_record(exp_name);
            if (!exp_j) { ok = 0; continue; }

            int found = 0;
            for (size_t g = 0; g < ngrains; g++) {
                cJSON *got_j = csvs_grain_to_json(&grains[g]);
                if (csvs_test_json_equal(got_j, exp_j)) { found = 1; cJSON_Delete(got_j); break; }
                cJSON_Delete(got_j);
            }
            if (!found) {
                char *exp_s = cJSON_PrintUnformatted(exp_j);
                fprintf(stderr, "    mow %s: expected grain %s not found: %s\n",
                        name, exp_name, exp_s);
                free(exp_s);
                ok = 0;
            }

            cJSON_Delete(exp_j);
        }

        if ((int)ngrains != expected_n) {
            fprintf(stderr, "    mow %s: got %zu grains, want %d\n",
                    name, ngrains, expected_n);
            ok = 0;
        }

        RUN_TEST(name, ok);

        for (size_t i = 0; i < ngrains; i++) csvs_grain_free(&grains[i]);
        free(grains);
        csvs_entry_free(&initial);
        cJSON_Delete(initial_j);
    }

    cJSON_Delete(cases);
}

/* ── Test: sow.json ───────────────────────────────────────────────── */

static void test_sow(void)
{
    printf("sow...\n");
    cJSON *cases = csvs_test_load_testcase("sow");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        const char *name = cJSON_GetObjectItemCaseSensitive(tc, "name")->valuestring;
        const char *initial_name = cJSON_GetObjectItemCaseSensitive(tc, "initial")->valuestring;
        const char *grain_name = cJSON_GetObjectItemCaseSensitive(tc, "grain")->valuestring;
        const char *trunk = cJSON_GetObjectItemCaseSensitive(tc, "trunk")->valuestring;
        const char *branch = cJSON_GetObjectItemCaseSensitive(tc, "branch")->valuestring;
        const char *expected_name = cJSON_GetObjectItemCaseSensitive(tc, "expected")->valuestring;

        cJSON *initial_j = csvs_test_load_record(initial_name);
        cJSON *grain_j = csvs_test_load_record(grain_name);
        cJSON *expected_j = csvs_test_load_record(expected_name);

        csvs_entry initial = csvs_entry_from_json(initial_j);
        csvs_grain grain = csvs_grain_from_json(grain_j);

        csvs_entry result = csvs_sow(&initial, &grain, trunk, branch);

        cJSON *result_j = csvs_entry_to_json(&result);

        int ok = csvs_test_json_equal(result_j, expected_j);
        if (!ok) {
            char *got = cJSON_PrintUnformatted(result_j);
            char *exp = cJSON_PrintUnformatted(expected_j);
            fprintf(stderr, "    sow %s:\n      got:  %s\n      want: %s\n",
                    name, got, exp);
            free(got); free(exp);
        }
        RUN_TEST(name, ok);

        cJSON_Delete(result_j);
        csvs_entry_free(&result);
        csvs_grain_free(&grain);
        csvs_entry_free(&initial);
        cJSON_Delete(initial_j);
        cJSON_Delete(grain_j);
        cJSON_Delete(expected_j);
    }

    cJSON_Delete(cases);
}

/* ── Test: select.json ─────────────────────────────────────────────── */

static void test_select(void)
{
    printf("select...\n");
    cJSON *cases = csvs_test_load_testcase("select");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        cJSON *name_j = cJSON_GetObjectItemCaseSensitive(tc, "name");
        const char *name = name_j ? name_j->valuestring : "?";
        const char *dataset_name = cJSON_GetObjectItemCaseSensitive(tc, "initial")->valuestring;
        cJSON *query_names = cJSON_GetObjectItemCaseSensitive(tc, "query");
        cJSON *expected_names = cJSON_GetObjectItemCaseSensitive(tc, "expected");

        csvs_dataset *ds = csvs_open(csvs_test_dataset_path(dataset_name));
        if (!ds) {
            fprintf(stderr, "    select %s: cannot open dataset %s\n",
                    name, dataset_name);
            RUN_TEST(name, 0);
            continue;
        }

        int nqueries = cJSON_GetArraySize(query_names);
        csvs_entry *queries = malloc(nqueries * sizeof(csvs_entry));
        for (int i = 0; i < nqueries; i++) {
            const char *qname = cJSON_GetArrayItem(query_names, i)->valuestring;
            cJSON *q_j = csvs_test_load_record(qname);
            queries[i] = csvs_entry_from_json(q_j);
            cJSON_Delete(q_j);
        }

        csvs_iter *it = csvs_select(ds, queries, nqueries, 0, 0);

        cJSON *got_arr = cJSON_CreateArray();
        const csvs_entry *result;
        while ((result = csvs_next(it)) != NULL) {
            cJSON_AddItemToArray(got_arr, csvs_entry_to_json(result));
        }

        int nexpected = cJSON_GetArraySize(expected_names);
        cJSON *exp_arr = cJSON_CreateArray();
        for (int i = 0; i < nexpected; i++) {
            const char *ename = cJSON_GetArrayItem(expected_names, i)->valuestring;
            cJSON *e_j = csvs_test_load_record(ename);
            cJSON_AddItemToArray(exp_arr, e_j);
        }

        int ok = 1;
        int ngot = cJSON_GetArraySize(got_arr);

        if (ngot != nexpected) {
            fprintf(stderr, "    select %s: got %d results, want %d\n",
                    name, ngot, nexpected);
            ok = 0;
        }

        for (int i = 0; i < nexpected && ok; i++) {
            cJSON *exp = cJSON_GetArrayItem(exp_arr, i);
            int found = 0;
            for (int j = 0; j < ngot; j++) {
                if (csvs_test_json_equal(cJSON_GetArrayItem(got_arr, j), exp)) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                char *exp_s = cJSON_PrintUnformatted(exp);
                fprintf(stderr, "    select %s: expected[%d] not found: %s\n",
                        name, i, exp_s);
                for (int j = 0; j < ngot; j++) {
                    char *got_s = cJSON_PrintUnformatted(cJSON_GetArrayItem(got_arr, j));
                    fprintf(stderr, "      got[%d]: %s\n", j, got_s);
                    free(got_s);
                }
                free(exp_s);
                ok = 0;
            }
        }

        RUN_TEST(name, ok);

        cJSON_Delete(got_arr);
        cJSON_Delete(exp_arr);
        csvs_iter_free(it);
        for (int i = 0; i < nqueries; i++) csvs_entry_free(&queries[i]);
        free(queries);
        csvs_close(ds);
    }

    cJSON_Delete(cases);
}

/* ── Test: init.json ──────────────────────────────────────────────── */

static void test_init(void)
{
    printf("init...\n");
    cJSON *cases = csvs_test_load_testcase("init");
    if (!cases) return;

    cJSON *tc;
    cJSON_ArrayForEach(tc, cases) {
        const char *name = cJSON_GetObjectItemCaseSensitive(tc, "name")->valuestring;
        const char *expected = cJSON_GetObjectItemCaseSensitive(tc, "expected")->valuestring;

        csvs_dataset *ds = csvs_open(csvs_test_dataset_path(expected));
        int ok = ds != NULL;
        if (!ok)
            fprintf(stderr, "    init %s: cannot open expected dataset %s\n",
                    name, expected);
        RUN_TEST(name, ok);
        csvs_close(ds);
    }

    cJSON_Delete(cases);
}

/* ── Main ─────────────────────────────────────────────────────────── */

int main(void)
{
    printf("csvs-test Phase 1-3\n");
    printf("test dir: %s\n\n", CSVS_TEST_DIR);

    test_entry();
    test_grain();
    test_to_schema();
    test_schema();
    test_get_nesting_level();
    test_sort("sort_ascending", 1);
    test_sort("sort_descending", 0);
    test_mow();
    test_sow();
    test_select();
    test_init();

    printf("\n%d tests: %d passed, %d failed\n",
           tests_run, tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
