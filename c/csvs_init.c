/*
 * csvs - Dataset creation (init)
 */

#include "csvs_internal.h"
#include <sys/stat.h>

csvs_dataset *csvs_create(const char *dir, int bare)
{
    if (bare) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/.csvs.csv", dir);

        FILE *fp = fopen(path, "w");
        if (!fp) {
            csvs_set_error("cannot create %s", path);
            return NULL;
        }
        fprintf(fp, "csvs,0.0.4\n");
        fclose(fp);

        csvs_dataset *ds = calloc(1, sizeof(csvs_dataset));
        ds->dir = csvs_strdup(dir);
        return ds;
    } else {
        char nested[1024];
        snprintf(nested, sizeof(nested), "%s/csvs", dir);

        struct stat st;
        if (stat(nested, &st) == 0) {
            csvs_set_error("dataset exists");
            return NULL;
        }

        if (mkdir(nested, 0755) != 0) {
            csvs_set_error("cannot create directory %s", nested);
            return NULL;
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/.csvs.csv", nested);
        FILE *fp = fopen(path, "w");
        if (!fp) {
            csvs_set_error("cannot create %s", path);
            return NULL;
        }
        fprintf(fp, "csvs,0.0.4\n");
        fclose(fp);

        csvs_dataset *ds = calloc(1, sizeof(csvs_dataset));
        ds->dir = csvs_strdup(nested);
        return ds;
    }
}
