/**
 * @file iems_pressure_adapter.c
 * @brief Binds the pressure engine to /proc, the app registry, and the app manager.
 */

#include "iems_pressure_adapter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "edgeos/edge_app_mgr.h"

static int _read_level(void *ctx, pressure_level_t *out) {
    iems_pressure_ctx_t *pressure = (iems_pressure_ctx_t *)ctx;
    if (!pressure || !out) return -1;

    char psi[512] = "";
    char meminfo[8192] = "";
    FILE *file = fopen("/proc/pressure/memory", "r");
    if (file) {
        size_t n = fread(psi, 1, sizeof(psi) - 1, file);
        psi[n] = '\0';
        fclose(file);
    }
    file = fopen("/proc/meminfo", "r");
    if (file) {
        size_t n = fread(meminfo, 1, sizeof(meminfo) - 1, file);
        meminfo[n] = '\0';
        fclose(file);
    }
    return edge_pressure_level_from_sources(psi[0] ? psi : NULL, meminfo[0] ? meminfo : NULL,
                                            &pressure->parse_policy, out);
}

static int _list_candidates(void *ctx, app_reclaim_candidate_t *out, size_t max, size_t *count) {
    iems_pressure_ctx_t *pressure = (iems_pressure_ctx_t *)ctx;
    if (!pressure || !pressure->mgr || !out || !count) return -1;
    *count = 0;

    const edge_app_repo_t *repo = edge_app_mgr_get_repo(pressure->mgr);
    if (!repo || !repo->list_apps) return -1;

    app_manifest_t apps[64];
    int n = repo->list_apps(repo->ctx, apps, 64);
    for (int i = 0; i < n && *count < max; i++) {
        if (apps[i].state == APP_STATE_UNINSTALLED) continue;
        app_reclaim_candidate_t *candidate = &out[*count];
        memset(candidate, 0, sizeof(*candidate));
        snprintf(candidate->app_id, sizeof(candidate->app_id), "%s", apps[i].id);
        candidate->priority = apps[i].priority;
        candidate->keep_alive = apps[i].keep_alive;
        candidate->last_active_ms = apps[i].last_active_ms;
        candidate->rss_kb = 0; /* no per-app RSS source for in-process wasm apps yet */
        candidate->desired_running = apps[i].desired_state;
        candidate->suspended =
            apps[i].state == APP_STATE_STOPPED && apps[i].desired_state;
        (*count)++;
    }
    return 0;
}

static int _clear_cache(void *ctx) {
    (void)ctx;
    return -1; /* no cache subsystem to drop */
}

static int _reclaim_stop(void *ctx, const char *app_id, reclaim_reason_t reason) {
    (void)reason;
    iems_pressure_ctx_t *pressure = (iems_pressure_ctx_t *)ctx;
    if (!pressure || !pressure->mgr) return -1;
    if (pressure->runtime) iems_app_runtime_stop(pressure->runtime, app_id);
    return edge_app_mgr_reclaim_stop_async(pressure->mgr, app_id, NULL, NULL);
}

static int _suspend(void *ctx, const char *app_id) {
    return _reclaim_stop(ctx, app_id, RECLAIM_REASON_RECLAIMED);
}

static int _activate(void *ctx, const char *app_id) {
    iems_pressure_ctx_t *pressure = (iems_pressure_ctx_t *)ctx;
    if (!pressure) return -1;
    /* Activation must spawn app-host, not just flip the registry state. */
    if (pressure->control && pressure->control->start_app) {
        return pressure->control->start_app(pressure->control->ctx, app_id);
    }
    if (!pressure->mgr) return -1;
    return edge_app_mgr_start_async(pressure->mgr, app_id, NULL, NULL);
}

static void _emit(void *ctx, pressure_level_t level, const char *app_id,
                  reclaim_reason_t reason, const char *message) {
    (void)ctx;
    (void)reason;
    if (level >= PRESSURE_LEVEL_HIGH) {
        LOG_WARN(LOG_CAT_CORE, "pressure: level=%s app=%s %s", edge_pressure_level_name(level),
                 app_id && app_id[0] ? app_id : "-", message ? message : "");
    } else {
        LOG_INFO(LOG_CAT_CORE, "pressure: level=%s app=%s %s", edge_pressure_level_name(level),
                 app_id && app_id[0] ? app_id : "-", message ? message : "");
    }
}

void iems_pressure_init(app_pressure_t *pressure, app_pressure_port_t *port,
                        iems_pressure_ctx_t *ctx, edge_app_mgr_t *mgr,
                        iems_app_runtime_t *runtime, rest_control_port_t *control) {
    if (!pressure || !port || !ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->mgr = mgr;
    ctx->runtime = runtime;
    ctx->control = control;
    edge_pressure_policy_init(&ctx->parse_policy);

    app_pressure_policy_t policy;
    edge_pressure_policy_init(&policy);
    const char *flag = getenv("EDGEC_PRESSURE_RECLAIM");
    policy.reclaim_enabled = flag && strcmp(flag, "1") == 0;

    port->ctx = ctx;
    port->read_level = _read_level;
    port->list_candidates = _list_candidates;
    port->clear_cache = _clear_cache;
    port->suspend = _suspend;
    port->stop = _reclaim_stop;
    port->activate = _activate;
    port->emit = _emit;

    edge_pressure_init(pressure, port, &policy);
    LOG_INFO(LOG_CAT_CORE, "pressure: %s (EDGEC_PRESSURE_RECLAIM=%s)",
             policy.reclaim_enabled ? "reclaim enabled" : "monitor only",
             policy.reclaim_enabled ? "1" : "0");
}

void iems_pressure_tick(app_pressure_t *pressure, uint64_t now_ms) {
    if (pressure) edge_pressure_step(pressure, now_ms);
}
