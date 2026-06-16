/*
 * csvs - error reporting
 */

#include "csvs_internal.h"
#include <stdarg.h>

static _Thread_local char csvs_errbuf[CSVS_ERRBUF_SIZE] = "";

void csvs_set_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)vsnprintf(csvs_errbuf, sizeof(csvs_errbuf), fmt, ap);
    va_end(ap);
}

const char *csvs_errmsg(void)
{
    return csvs_errbuf;
}
