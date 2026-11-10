#include "stream_sqlite_adapter.h"

#include "sqlite_service.h"

static int _exec(void *ctx, const char *sql) {
    return sqlite_svc_exec((sqlite_svc_t *)ctx, sql, NULL, NULL);
}

stream_sqlite_port_t stream_sqlite_port_from_sqlite(sqlite_svc_t *db) {
    stream_sqlite_port_t port = {.ctx = db, .exec = _exec};
    return port;
}
