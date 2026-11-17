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
 * Change-stream sink. Invoked from the publisher's thread while the data source
 * holds its subscription lock. It copies *sample and returns; it must not call
 * back into the port, the data source, or the socket. `event_ctx` is borrowed
 * and stays valid until unsubscribe returns.
 */
typedef void (*app_host_event_cb_t)(void *event_ctx, uint32_t handle,
                                    const app_host_sample_t *sample);

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

    /**
     * Re-check read access for a handle before a queued event is written.
     * Called on the server's loop thread. Return 0 when delivery may continue;
     * a failure drops the event and releases the subscription. NULL means the
     * product has no revocation check.
     */
    int (*allow)(void *ctx, uint32_t handle, char *reason, size_t reason_size);

    /**
     * Register a change subscription for handle. Called on the server's loop
     * thread. The implementation validates the handle, authorizes read access,
     * registers with its data source, and arranges for
     * on_event(event_ctx, handle, &sample) on every change. *out_sub_id receives
     * an opaque token (> 0) for unsubscribe. The implementation must not invoke
     * on_event after unsubscribe returns.
     */
    int (*subscribe)(void *ctx, uint32_t handle, app_host_event_cb_t on_event, void *event_ctx,
                     int32_t *out_sub_id, char *reason, size_t reason_size);

    /** Release tokens from subscribe. Called on the loop thread. */
    int (*unsubscribe)(void *ctx, int32_t sub_id);

    /**
     * Read one setting of the calling app. Writes the value as NUL-terminated
     * JSON text into out. Return 0 on success, otherwise -1 with a code in
     * reason: not_found, not_set, too_large, internal. Called on the loop thread.
     */
    int (*setting_get)(void *ctx, const char *key, char *out, size_t cap, char *reason,
                       size_t reason_size);

    void (*log)(void *ctx, int level, const char *message);
    uint64_t (*now_ms)(void *ctx);
    /** Per-second host-call and publish budgets; 0 = unlimited. */
    uint32_t calls_per_sec;
    uint32_t publish_per_sec;
    /** Manifest tick period; 0 disables the periodic tick event. */
    uint32_t tick_ms;
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

/**
 * Observe platform-event acks. cb runs on the loop thread with the kind of the
 * event the app just acknowledged. NULL clears. Only platform events are
 * reported; point events keep their own keepalive/ack accounting.
 */
typedef void (*app_host_event_ack_cb_t)(void *ctx, const char *kind);
void app_host_server_set_event_ack_cb(app_host_server_t *srv, app_host_event_ack_cb_t cb,
                                      void *ctx);

/**
 * Queue one platform event (kind plus a JSON text payload) for the app. The
 * queue is mutex-protected, so any thread may call this; delivery itself stays
 * on the loop thread and waits for the previous event's ack, like point
 * events. Returns 0 when queued (the oldest event is dropped if the queue is
 * full), -1 when the server is closing or kind/payload_json does not fit the
 * bounded sizes. kind is [A-Za-z0-9_.-] and is sent as-is; payload_json must
 * be valid JSON text and is escaped into a JSON string.
 */
int app_host_server_post_platform_event(app_host_server_t *srv, const char *kind,
                                        const char *payload_json);

/**
 * Queue a settings_changed platform event when the app called setting_watch on
 * this connection. The payload is {"key":"<key>"}; the value is never sent.
 * Returns 0 when queued, -1 when the app never watched, the server is closing,
 * or key/payload does not fit the bounded sizes. Safe from any thread.
 */
int app_host_server_post_settings_changed(app_host_server_t *srv, const char *key);

/**
 * Tick counters for telemetry and tests. enqueued counts tick events queued;
 * skipped counts periods where a tick was already queued or in flight, or the
 * queue was full. Point events are never displaced to make room for a tick.
 */
void app_host_server_tick_counters(const app_host_server_t *srv, uint64_t *enqueued,
                                   uint64_t *skipped);

#endif /* EDGE_APP_HOST_H */
