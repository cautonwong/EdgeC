/**
 * @file edge_app_host.h
 * @brief Host side of the app ABI v1: framed JSON over an inherited socketpair.
 *
 * The app (SDK client) sends {"id":N,"fn":"point_resolve","args":{...}} as a
 * 4-byte big-endian length plus UTF-8 JSON. The host answers
 * {"id":N,"ok":true,"result":{...}} or {"id":N,"ok":false,"code":"..."}.
 * Values, quality and error codes follow sdk/wit/app.v1.wit.
 */
#ifndef EDGE_APP_HOST_H
#define EDGE_APP_HOST_H

#include <stddef.h>
#include <stdint.h>

typedef struct uv_loop_s uv_loop_t;

#define APP_HOST_REASON_SIZE 64
#define APP_HOST_TEXT_SIZE 64
#define APP_HOST_MAX_FRAME (1024u * 1024u)

typedef enum {
    APP_HOST_VALUE_FLOAT = 0,
    APP_HOST_VALUE_INTEGER,
    APP_HOST_VALUE_BOOLEAN,
    APP_HOST_VALUE_TEXT,
} app_host_value_kind_t;

typedef enum {
    APP_HOST_ACCESS_READ = 0,
    APP_HOST_ACCESS_PUBLISH,
} app_host_access_t;

typedef enum {
    APP_HOST_QUALITY_GOOD = 0,
    APP_HOST_QUALITY_BAD,
    APP_HOST_QUALITY_UNCERTAIN,
} app_host_quality_t;

typedef struct {
    uint8_t kind; /**< app_host_value_kind_t */
    uint8_t quality;
    double number;
    int64_t integer;
    int boolean;
    char text[APP_HOST_TEXT_SIZE];
    uint64_t source_ts;
} app_host_sample_t;

/**
 * Host callbacks. Return 0 on success. On failure return -1 and put a WIT
 * error code in reason: denied_not_granted, denied_scope, denied_expired,
 * denied_revoked, denied_policy, denied_quota, denied_package_mismatch,
 * denied_abi, invalid_manifest, not_found, stale, busy, invalid, internal.
 */
typedef struct {
    void *ctx;
    int (*resolve)(void *ctx, const char *name, uint32_t *handle, uint8_t *access, char *reason,
                   size_t reason_size);
    int (*read)(void *ctx, uint32_t handle, app_host_sample_t *out, char *reason,
                size_t reason_size);
    int (*publish)(void *ctx, uint32_t handle, const app_host_sample_t *sample, char *reason,
                   size_t reason_size);
    void (*log)(void *ctx, int level, const char *message);
    uint64_t (*now_ms)(void *ctx);
} app_host_ports_t;

typedef struct app_host_server_s app_host_server_t;

/** Runs once the server has closed and freed itself; the ctx must outlive it. */
typedef void (*app_host_closed_cb_t)(void *ctx);

/**
 * Take ownership of fd and serve host calls on the loop. Returns NULL when
 * allocation or uv_pipe_open fails. on_closed runs after the server's pipe is
 * closed and the server is freed, so owners can drop their pointer without
 * guessing the close timing.
 */
app_host_server_t *app_host_server_create_ex(uv_loop_t *loop, int fd,
                                             const app_host_ports_t *ports,
                                             app_host_closed_cb_t on_closed, void *closed_ctx);

/** create_ex without a close notification. */
app_host_server_t *app_host_server_create(uv_loop_t *loop, int fd, const app_host_ports_t *ports);

/** Stop reading, close fd and free the server once the loop runs. */
void app_host_server_destroy(app_host_server_t *srv);

#endif /* EDGE_APP_HOST_H */
