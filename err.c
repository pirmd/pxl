#include "err.h"
#include <stddef.h>

static pxl_log_fn g_log_fn = NULL;

void
pxl_set_log_fn(pxl_log_fn fn) {
    g_log_fn = fn;
}

void
pxl_log(const char *msg) {
    if (g_log_fn) g_log_fn(msg);
}
