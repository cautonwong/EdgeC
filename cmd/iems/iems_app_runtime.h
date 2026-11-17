/**
 * @file iems_app_runtime.h
 * @brief Per-app app-host processes, their Host ABI servers and supervision.
 *
 * One runtime lives in the product loop. start spawns app-host with an
 * inherited socketpair, serves the Host ABI on the parent end, and stops by
 * signalling the process and closing the socket.
 */
#ifndef IEMS_APP_RUNTIME_H
#define IEMS_APP_RUNTIME_H

#include <stdint.h>

#include "contract_data_types.h"
#include "datahub.h"
#include "sqlite_service.h"

typedef struct uv_loop_s uv_loop_t;

#define IEMS_APP_RUNTIME_MAX 8

typedef struct {
    datahub_t *hub;
    sqlite_svc_t *db;
    const edge_tag_meta_t *tags;
    uint32_t tag_count;
    uint32_t app_base;
    const char *app_host_path; /**< NULL: EDGEC_APP_HOST_PATH, else the installed path */
    /** Unexpected app-host exit. Called on the loop before the runtime forgets the app. */
    void (*on_fault)(void *ctx, const char *app_id, int64_t status, int term_signal);
    void *on_fault_ctx;
    /** A scheduled restart spawned the app again. */
    void (*on_restarted)(void *ctx, const char *app_id);
    void *on_restarted_ctx;
} iems_app_runtime_config_t;

#define IEMS_APP_RESTART_CAP 5
/** Bounded wait for the app to ack app_will_stop before the stop signals it. */
#define IEMS_APP_STOP_GRACE_MS 2000u

typedef struct iems_app_runtime_s iems_app_runtime_t;

iems_app_runtime_t *iems_app_runtime_create(uv_loop_t *loop, const iems_app_runtime_config_t *cfg);

/** Stop every app and free the runtime after the loop drains exits. */
void iems_app_runtime_destroy(iems_app_runtime_t *rt);

int iems_app_runtime_start(iems_app_runtime_t *rt, const char *app_id, const char *wasm_path,
                           uint32_t heap_kb, uint32_t stack_kb);
/** Start with the manifest resource limits. 0 disables a limit; tick_ms 0 disables ticks. */
int iems_app_runtime_start_ex(iems_app_runtime_t *rt, const char *app_id, const char *wasm_path,
                              uint32_t heap_kb, uint32_t stack_kb, uint32_t pids_max,
                              uint32_t cpu_ms_per_sec, uint32_t calls_per_sec,
                              uint32_t publish_per_sec, uint32_t tick_ms);
/**
 * Operator stop: queue app_will_stop {"reason":"operator","deadline_ms":N},
 * then wait up to IEMS_APP_STOP_GRACE_MS for the app's ack or exit before
 * signalling it. A second stop while waiting joins the first.
 */
void iems_app_runtime_stop(iems_app_runtime_t *rt, const char *app_id);
/**
 * Stop immediately without app_will_stop. For fault, replacement and product
 * shutdown paths, where the app must not run another flush step.
 */
void iems_app_runtime_stop_now(iems_app_runtime_t *rt, const char *app_id);
/** Stop every app immediately: the product loop is going away, so there is no grace wait. */
void iems_app_runtime_stop_all(iems_app_runtime_t *rt);
int iems_app_runtime_is_running(iems_app_runtime_t *rt, const char *app_id);

/** app_will_stop events queued, acked, and expired at the deadline. */
void iems_app_runtime_stop_counters(const iems_app_runtime_t *rt, uint64_t *sent,
                                    uint64_t *acked, uint64_t *expired);

/** app-host pid for a running app, or -1. */
int iems_app_runtime_pid(iems_app_runtime_t *rt, const char *app_id);

/** Platform app-host binary; the installer links it into each app directory. */
const char *iems_app_runtime_app_host_path(const iems_app_runtime_t *rt);

/**
 * Start the app again after delay_ms. Idempotent per app while a restart is
 * pending. Returns 0 when scheduled.
 */
int iems_app_runtime_restart_later(iems_app_runtime_t *rt, const char *app_id,
                                   const char *wasm_path, uint32_t delay_ms, uint32_t heap_kb,
                                   uint32_t stack_kb);
int iems_app_runtime_restart_later_ex(iems_app_runtime_t *rt, const char *app_id,
                                      const char *wasm_path, uint32_t delay_ms, uint32_t heap_kb,
                                      uint32_t stack_kb, uint32_t pids_max,
                                      uint32_t cpu_ms_per_sec, uint32_t calls_per_sec,
                                      uint32_t publish_per_sec, uint32_t tick_ms);

/** Tick events queued and periods skipped for a running app; counters are 0 when absent. */
int iems_app_runtime_tick_counters(iems_app_runtime_t *rt, const char *app_id,
                                   uint64_t *enqueued, uint64_t *skipped);

/**
 * Tell a running app that an operator wrote setting `key`. Delivers
 * settings_changed only when the app called setting_watch on its current
 * connection. Missing or stopped apps return -1 without an event.
 */
int iems_app_runtime_notify_settings_changed(iems_app_runtime_t *rt, const char *app_id,
                                             const char *key);

#endif /* IEMS_APP_RUNTIME_H */
