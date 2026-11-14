#ifndef EDGE_APP_SDK_H
#define EDGE_APP_SDK_H

/*
 * Hand-written C binding for the frozen EdgeC app host ABI v1.
 * Source of truth: sdk/wit/app.v1.wit. ci/scripts/check_sdk_wit.py fails when
 * the hash below no longer matches that file.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_APP_ABI_VERSION "1.0.0"
#define EDGE_APP_WIT_SHA256 "adfad063f9c47f564e22faf78ade88f8d9e99399879b85aaf6709c443c016976"

typedef enum {
    EDGE_APP_VALUE_FLOAT,
    EDGE_APP_VALUE_INTEGER,
    EDGE_APP_VALUE_BOOLEAN,
    EDGE_APP_VALUE_TEXT
} edge_app_value_kind_t;

typedef struct {
    edge_app_value_kind_t kind;
    double number;
    int64_t integer;
    bool boolean;
    char text[64];
} edge_app_value_t;

typedef struct {
    edge_app_value_t value;
    const char *quality;
    uint64_t source_ts;
} edge_app_sample_t;

typedef struct edge_app_sdk edge_app_sdk_t;

typedef void (*edge_app_init_fn)(void *ctx);
typedef void (*edge_app_event_fn)(void *ctx, const char *event_json);
typedef void (*edge_app_stop_fn)(void *ctx);

/* Takes ownership of fd. Returns NULL only when allocation fails. */
edge_app_sdk_t *edge_app_sdk_create(int fd);
void edge_app_sdk_destroy(edge_app_sdk_t *sdk);

/* Register the three exports. The future app-host calls dispatch() with the
 * decoded host request; this slice does not start a reader thread. */
void edge_app_sdk_set_exports(edge_app_sdk_t *sdk, edge_app_init_fn init,
                              edge_app_event_fn on_event, edge_app_stop_fn stop,
                              void *ctx);

/* Handle one host-to-app request (app_init / app_on_event / app_stop) and write
 * the response JSON. Returns 0 on success, -1 on malformed input or short buffer. */
int edge_app_sdk_dispatch(edge_app_sdk_t *sdk, const char *request_json,
                          char *response_json, size_t response_cap);

/* Raw host call. Returns NULL and fills result_json on success; otherwise returns
 * the host error code (or "protocol" / "transport"). The pointer stays valid until
 * the next SDK call. */
const char *edge_app_sdk_call(edge_app_sdk_t *sdk, const char *fn,
                              const char *args_json, char *result_json,
                              size_t result_cap);

const char *edge_app_point_resolve(edge_app_sdk_t *sdk, const char *name,
                                   uint32_t *handle, const char **access);
const char *edge_app_point_read(edge_app_sdk_t *sdk, uint32_t handle,
                                edge_app_sample_t *sample);
const char *edge_app_point_subscribe(edge_app_sdk_t *sdk, uint32_t handle,
                                     double deadband, uint32_t min_interval_ms,
                                     uint32_t max_interval_ms,
                                     uint32_t *subscription);
const char *edge_app_point_publish(edge_app_sdk_t *sdk, uint32_t handle,
                                   const edge_app_sample_t *sample);
const char *edge_app_log(edge_app_sdk_t *sdk, const char *level,
                         const char *message);
const char *edge_app_now(edge_app_sdk_t *sdk, uint64_t *now_ms);

#ifdef __cplusplus
}
#endif

#endif
