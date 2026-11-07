#ifndef STREAM_SQLITE_PORT_H
#define STREAM_SQLITE_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *ctx;
    int (*exec)(void *ctx, const char *sql);
} stream_sqlite_port_t;

static inline int stream_sqlite_exec(const stream_sqlite_port_t *db, const char *sql) {
    return (db && db->exec) ? db->exec(db->ctx, sql) : -1;
}

#ifdef __cplusplus
}
#endif

#endif /* STREAM_SQLITE_PORT_H */
