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

typedef struct iems_app_runtime_s iems_app_runtime_t;

iems_app_runtime_t *iems_app_runtime_create(uv_loop_t *loop, const iems_app_runtime_config_t *cfg);

/** Stop every app and free the runtime after the loop drains exits. */
void iems_app_runtime_destroy(iems_app_runtime_t *rt);

int iems_app_runtime_start(iems_app_runtime_t *rt, const char *app_id, const char *wasm_path,
                           uint32_t heap_kb, uint32_t stack_kb);
void iems_app_runtime_stop(iems_app_runtime_t *rt, const char *app_id);
void iems_app_runtime_stop_all(iems_app_runtime_t *rt);
int iems_app_runtime_is_running(iems_app_runtime_t *rt, const char *app_id);

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

#endif /* IEMS_APP_RUNTIME_H */
