/*
 * csvs-test — shared test fixture loader for C consumers
 *
 * Provides fixture loading (records, test cases, datasets)
 * and JSON comparison. No csvs dependency.
 *
 * Parallel to lib.rs / index.js in this directory.
 *
 * Usage:
 *   #define CSVS_TEST_DIR "/path/to/csvs/test"
 *   #include "csvs_test.h"
 *
 * Depends on: cJSON.h
 */

#ifndef CSVS_TEST_H
#define CSVS_TEST_H

#include "cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CSVS_TEST_DIR
#error "Define CSVS_TEST_DIR before including csvs_test.h"
#endif

/* ── File loading ────────────────────────────────────────────────── */

static char *csvs_test_read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (buf) { fread(buf, 1, len, f); buf[len] = '\0'; }
    fclose(f);
    return buf;
}

static cJSON *csvs_test_load_json(const char *subdir, const char *name,
                                   const char *ext)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s/%s%s", CSVS_TEST_DIR, subdir, name, ext);
    char *text = csvs_test_read_file(path);
    if (!text) return NULL;
    cJSON *j = cJSON_Parse(text);
    free(text);
    if (!j) fprintf(stderr, "JSON parse error in %s\n", path);
    return j;
}

static cJSON *csvs_test_load_record(const char *name)
{
    return csvs_test_load_json("records", name, ".json");
}

static cJSON *csvs_test_load_testcase(const char *name)
{
    return csvs_test_load_json("cases", name, ".json");
}

static char *csvs_test_dataset_path(const char *name)
{
    static char path[1024];
    snprintf(path, sizeof(path), "%s/datasets/%s", CSVS_TEST_DIR, name);
    return path;
}

/* ── JSON comparison ─────────────────────────────────────────────── */

static int csvs_test_json_equal(const cJSON *a, const cJSON *b)
{
    return cJSON_Compare(a, b, 1);
}

#endif /* CSVS_TEST_H */
