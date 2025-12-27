#pragma once
#include <uv.h>

typedef void (*sql_cb_t)(int rc, void *user);

int sqlite_exec_async(uv_loop_t *loop, const char *sql, sql_cb_t cb, void *user);