/**
 * @file rest_system.c
 * @brief Health and stats routes for the management plane.
 */

#include <stdlib.h>

#include <cjson/cJSON.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_version.h"
#include "rest_control_port.h"
#include "rest_internal.h"

/* uv_now() 是 CLOCK_MONOTONIC,Linux 上等于机器开机时长。进程运行时长必须
 * 以 REST 服务启动时刻为基准;否则换机器/重启产品都看不出区别。 */
static uint64_t g_rest_process_start_ms;

void rest_system_mark_start(uv_loop_t *loop) {
    if (loop && g_rest_process_start_ms == 0) g_rest_process_start_ms = uv_now(loop);
}

static double _rest_uptime_sec(service_t *svc) {
    uv_loop_t *loop = service_get_loop(svc);
    if (!loop || g_rest_process_start_ms == 0) return 0.0;
    uint64_t now = uv_now(loop);
    return now > g_rest_process_start_ms ? (double)(now - g_rest_process_start_ms) / 1000.0
                                         : 0.0;
}

char *rest_system_health(service_t *svc) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddNumberToObject(root, "uptime", _rest_uptime_sec(svc));
    cJSON_AddStringToObject(root, "version", edge_get_version()->version_str);

    datahub_t *hub = service_get_datahub(svc);
    struct net_buf_pool *io_pool = datahub_get_io_pool(hub);
    if (io_pool) {
        net_buf_pool_stats_t stats;
        /* health 只是采样统计,不能把毫秒级在飞事务报成 Long-lived;真正长期占用仍会在 >60s 时报。 */
        net_buf_pool_audit(io_pool, 60 * 1000, &stats);
        cJSON *pool = cJSON_AddObjectToObject(root, "net_buf_pool");
        cJSON_AddNumberToObject(pool, "total", stats.total_count);
        cJSON_AddNumberToObject(pool, "free", stats.free_count);
        cJSON_AddNumberToObject(pool, "peak", stats.peak_usage);
    }
    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}

char *rest_system_stats(service_t *svc) {
    cJSON *root = cJSON_CreateObject();

    // 注入无法伪造的生命证明：高精度 Uptime

    double uptime = _rest_uptime_sec(svc);

    cJSON_AddNumberToObject(root, "uptime_sec_proof", uptime);

    cJSON_AddStringToObject(root, "engine_status", "ALIVE_AND_KICKING");

    // 1. DataHub

    datahub_t *hub = service_get_datahub(svc);
    if (hub) {
        cJSON *dh = cJSON_AddObjectToObject(root, "datahub");
        cJSON_AddNumberToObject(dh, "total_writes", (double)datahub_get_ring_total_writes(hub));
        cJSON_AddNumberToObject(dh, "listener_count", (double)datahub_get_listener_count(hub));
    }

    // 2. Arbiter
    arbiter_t *arb = service_get_arbiter(svc);
    if (arb) {
        arbiter_bus_status_t bus_stats[32];  // 限制上报点位数量，防止栈溢出
        int count = arbiter_get_statuses(arb, bus_stats, 32);
        cJSON *bus_arr = cJSON_AddArrayToObject(root, "buses");
        for (int i = 0; i < count; i++) {
            cJSON *b = cJSON_CreateObject();
            cJSON_AddNumberToObject(b, "id", bus_stats[i].bus_id);
            cJSON_AddBoolToObject(b, "busy", bus_stats[i].is_busy);
            cJSON_AddNumberToObject(b, "queue", bus_stats[i].queue_len);
            cJSON_AddItemToArray(bus_arr, b);
        }
    }

    // 3. Pipeline (Stage 11.2)
    const rest_control_port_t *control =
        (const rest_control_port_t *)service_get_control_port(svc);
    if (control && control->stream_topology_json) {
        char *engine_json = control->stream_topology_json(control->ctx);
        if (engine_json) {
            cJSON *ej = cJSON_Parse(engine_json);
            if (ej) cJSON_AddItemToObject(root, "engine", ej);
            free(engine_json);
        }
    }

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return out;
}
