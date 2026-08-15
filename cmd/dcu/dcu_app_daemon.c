/**
 * @file dcu_app_daemon.c
 * @brief Production SGCC/CSG Microservices Container Lifecycle & Resource Quota Daemon Engine Implementation
 */

#include "dcu_app_daemon.h"
#include <string.h>

void dcu_app_daemon_init(dcu_app_daemon_t* daemon, uint64_t now_ms) {
    if (!daemon) return;
    memset(daemon, 0, sizeof(dcu_app_daemon_t));
    daemon->daemon_start_ms = now_ms;
}

int dcu_app_daemon_install(dcu_app_daemon_t* daemon, const char* name, const char* ver,
                           uint32_t app_id, const dcu_app_quota_t* quota) {
    if (!daemon || !name) return -1;

    for (uint32_t i = 0; i < daemon->app_count; i++) {
        if (daemon->apps[i].app_id == app_id) {
            return -2; // Already exists
        }
    }

    if (daemon->app_count >= DCU_MAX_CONTAINER_APPS) {
        return -3; // Full
    }

    uint32_t idx = daemon->app_count++;
    dcu_app_container_t* app = &daemon->apps[idx];
    memset(app, 0, sizeof(dcu_app_container_t));
    strncpy(app->app_name, name, sizeof(app->app_name) - 1);
    if (ver) strncpy(app->app_version, ver, sizeof(app->app_version) - 1);
    app->app_id = app_id;
    app->state = APP_STATE_INSTALLED;

    if (quota) {
        app->quota = *quota;
    } else {
        // Standard SGCC default quota: CPU <= 20%, Memory <= 64MB, Flash <= 32MB
        app->quota.max_cpu_pct = 20;
        app->quota.max_mem_bytes = 64 * 1024 * 1024;
        app->quota.max_flash_bytes = 32 * 1024 * 1024;
    }

    return 0;
}

int dcu_app_daemon_start(dcu_app_daemon_t* daemon, uint32_t app_id, uint64_t now_ms) {
    if (!daemon) return -1;

    for (uint32_t i = 0; i < daemon->app_count; i++) {
        if (daemon->apps[i].app_id == app_id) {
            daemon->apps[i].state = APP_STATE_RUNNING;
            daemon->apps[i].last_heartbeat_ms = now_ms;
            daemon->apps[i].is_quota_exceeded = false;
            return 0;
        }
    }
    return -2; // Not found
}

int dcu_app_daemon_stop(dcu_app_daemon_t* daemon, uint32_t app_id) {
    if (!daemon) return -1;

    for (uint32_t i = 0; i < daemon->app_count; i++) {
        if (daemon->apps[i].app_id == app_id) {
            daemon->apps[i].state = APP_STATE_STOPPED;
            daemon->apps[i].current_cpu_pct = 0.0;
            daemon->apps[i].current_mem_bytes = 0;
            return 0;
        }
    }
    return -2;
}

int dcu_app_daemon_heartbeat(dcu_app_daemon_t* daemon, uint32_t app_id, double cpu_pct,
                             uint32_t mem_bytes, uint64_t now_ms) {
    if (!daemon) return -1;

    for (uint32_t i = 0; i < daemon->app_count; i++) {
        dcu_app_container_t* app = &daemon->apps[i];
        if (app->app_id == app_id) {
            if (app->state != APP_STATE_RUNNING) {
                return -3; // Not running
            }
            app->current_cpu_pct = cpu_pct;
            app->current_mem_bytes = mem_bytes;
            app->last_heartbeat_ms = now_ms;

            // Enforce quota limits
            if (cpu_pct > app->quota.max_cpu_pct || mem_bytes > app->quota.max_mem_bytes) {
                app->is_quota_exceeded = true;
            } else {
                app->is_quota_exceeded = false;
            }
            return 0;
        }
    }
    return -2;
}

int dcu_app_daemon_watchdog_poll(dcu_app_daemon_t* daemon, uint64_t timeout_threshold_ms, uint64_t now_ms) {
    if (!daemon) return -1;

    int recovered_count = 0;
    for (uint32_t i = 0; i < daemon->app_count; i++) {
        dcu_app_container_t* app = &daemon->apps[i];
        if (app->state == APP_STATE_RUNNING) {
            uint64_t elapsed = (now_ms >= app->last_heartbeat_ms) ? (now_ms - app->last_heartbeat_ms) : 0;
            // If heartbeat timeout or severely exceeded quota, restart sandbox
            if (elapsed > timeout_threshold_ms || app->is_quota_exceeded) {
                app->state = APP_STATE_FAULT;
                app->restart_count++;
                // Auto-recover/restart
                app->state = APP_STATE_RUNNING;
                app->last_heartbeat_ms = now_ms;
                app->is_quota_exceeded = false;
                recovered_count++;
            }
        }
    }
    return recovered_count;
}
