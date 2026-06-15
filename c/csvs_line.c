/*
 * csvs - CSV line I/O: escape/unescape, parse with libcsv, write
 */

#include "csvs_internal.h"
#include "lib/libcsv/csv.h"

/* ── Newline escape/unescape ─────────────────────────────────────── */

char *csvs_escape_newline(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    /* worst case: every char is \n → doubles length */
    char *out = malloc(len * 2 + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\n') {
            out[j++] = '\\';
            out[j++] = 'n';
        } else {
            out[j++] = s[i];
        }
    }
    out[j] = '\0';
    return out;
}

char *csvs_unescape_newline(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    char *out = malloc(len + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] == '\\' && i + 1 < len && s[i + 1] == 'n') {
            out[j++] = '\n';
            i++;
        } else {
            out[j++] = s[i];
        }
    }
    out[j] = '\0';
    return out;
}

/* ── Parse a single CSV line using libcsv ────────────────────────── */

/* Callback state for libcsv parser */
typedef struct {
    char *fields[2];
    int field_idx;
} parse_ctx;

static void cb_field(void *data, size_t len, void *ctx_)
{
    parse_ctx *ctx = ctx_;
    if (ctx->field_idx < 2) {
        ctx->fields[ctx->field_idx] = csvs_strndup(data, len);
    }
    ctx->field_idx++;
}

static void cb_row(int c __attribute__((unused)), void *ctx_)
{
    (void)ctx_;
}

int csvs_parse_line(const char *line, size_t len,
                    char **key_out, char **val_out)
{
    struct csv_parser p;
    if (csv_init(&p, CSV_STRICT) != 0) {
        csvs_set_error("csv_init failed");
        return -1;
    }

    parse_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));

    csv_parse(&p, line, len, cb_field, cb_row, &ctx);
    csv_fini(&p, cb_field, cb_row, &ctx);
    csv_free(&p);

    *key_out = ctx.fields[0] ? ctx.fields[0] : csvs_strdup("");
    *val_out = ctx.fields[1] ? ctx.fields[1] : csvs_strdup("");
    return 0;
}

/* ── Write a CSV line ────────────────────────────────────────────── */

int csvs_write_line(FILE *fp, const char *key, const char *value)
{
    /* Escape newlines first */
    char *ek = csvs_escape_newline(key);
    char *ev = csvs_escape_newline(value);

    /* Check if field needs quoting (contains comma, quote, or newline) */
    int kq = ek && (strchr(ek, ',') || strchr(ek, '"') || strchr(ek, '\n'));
    int vq = ev && (strchr(ev, ',') || strchr(ev, '"') || strchr(ev, '\n'));

    if (kq) {
        fputc('"', fp);
        for (const char *p = ek; *p; p++) {
            if (*p == '"') fputc('"', fp);
            fputc(*p, fp);
        }
        fputc('"', fp);
    } else {
        fputs(ek ? ek : "", fp);
    }

    fputc(',', fp);

    if (vq) {
        fputc('"', fp);
        for (const char *p = ev; *p; p++) {
            if (*p == '"') fputc('"', fp);
            fputc(*p, fp);
        }
        fputc('"', fp);
    } else {
        fputs(ev ? ev : "", fp);
    }

    fputc('\n', fp);

    free(ek);
    free(ev);
    return 0;
}

/* ── File helpers ────────────────────────────────────────────────── */

int csvs_file_is_empty(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz == 0;
}

/* ── Key groups: read a sorted CSV tablet ────────────────────────── */

static void csvs_key_group_free(csvs_key_group *g)
{
    free(g->key);
    for (size_t i = 0; i < g->nvalues; i++) free(g->values[i]);
    free(g->values);
}

void csvs_groups_free(csvs_groups *gs)
{
    for (size_t i = 0; i < gs->ngroups; i++)
        csvs_key_group_free(&gs->groups[i]);
    free(gs->groups);
    gs->groups = NULL;
    gs->ngroups = gs->cap = 0;
}

csvs_groups csvs_read_groups(const char *filepath)
{
    csvs_groups gs;
    memset(&gs, 0, sizeof(gs));

    FILE *f = fopen(filepath, "r");
    if (!f) return gs;

    char *line_buf = NULL;
    size_t line_cap = 0;
    ssize_t line_len;

    char *current_key = NULL;
    char **current_values = NULL;
    size_t nvalues = 0, values_cap = 0;

    while ((line_len = getline(&line_buf, &line_cap, f)) > 0) {
        /* strip trailing newline */
        while (line_len > 0 && (line_buf[line_len - 1] == '\n' ||
                                line_buf[line_len - 1] == '\r'))
            line_len--;

        if (line_len == 0) continue;

        char *key_raw = NULL, *val_raw = NULL;
        if (csvs_parse_line(line_buf, line_len, &key_raw, &val_raw) != 0) {
            free(key_raw); free(val_raw);
            continue;
        }

        /* unescape newlines */
        char *key = csvs_unescape_newline(key_raw);
        char *val = csvs_unescape_newline(val_raw);
        free(key_raw); free(val_raw);

        if (current_key && strcmp(current_key, key) != 0) {
            /* flush group */
            csvs_key_group grp;
            grp.key = current_key;
            grp.values = current_values;
            grp.nvalues = nvalues;
            grp.cap = values_cap;
            VEC_PUSH(gs.groups, gs.ngroups, gs.cap, grp);

            current_key = key;
            current_values = NULL;
            nvalues = 0;
            values_cap = 0;
            VEC_PUSH(current_values, nvalues, values_cap, val);
        } else if (!current_key) {
            current_key = key;
            VEC_PUSH(current_values, nvalues, values_cap, val);
        } else {
            free(key);
            VEC_PUSH(current_values, nvalues, values_cap, val);
        }
    }

    /* flush last group */
    if (current_key) {
        csvs_key_group grp;
        grp.key = current_key;
        grp.values = current_values;
        grp.nvalues = nvalues;
        grp.cap = values_cap;
        VEC_PUSH(gs.groups, gs.ngroups, gs.cap, grp);
    }

    free(line_buf);
    fclose(f);
    return gs;
}
