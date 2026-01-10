/**
 * @file main.c
 * @brief IEMS Controller - The Ultimate Composition Root
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contracts.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "sched_time.h"
#include "sqlite_service.h"
#include "stream_engine.h"

// 驱动注册接口
extern int dlt645_driver_register(driver_manager_t *mgr);
extern int dlms_driver_register(driver_manager_t *mgr);
extern int iec104_driver_register(driver_manager_t *mgr);

static service_t *g_running_svc = NULL;

static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

/* --- [Sprint 2] Phase 4: 指挥链与配置驱动轮询 --- */

typedef struct {
    bus_orchestrator_t *orch;
    int bus_id;
    uint32_t tag_id;
    char protocol[16];
} calendar_poll_ctx_t;

/**
 * @brief 指令完成回调：实现任务状态回环 (Task 3)
 */
static void on_poll_complete(cmd_msg_t *msg, int status) {
    if (status == CMD_STATUS_SUCCESS) {
        LOG_INFO(LOG_CAT_SCHED, "Calendar: Poll Tag %u SUCCESS", msg->tag_id);
    } else {
        LOG_WARN(LOG_CAT_SCHED, "Calendar: Poll Tag %u FAILED (Status: %d)", msg->tag_id, status);
    }
}

static void on_calendar_poll_fire(calendar_task_t *task) {
    calendar_poll_ctx_t *ctx = (calendar_poll_ctx_t *)task->user_data;
    if (!ctx) return;

    cmd_msg_t cmd = {0};
    cmd.action = CMD_ACTION_POLL;
    cmd.tag_id = ctx->tag_id;
    cmd.priority = ARBITER_PRIO_NORMAL;
    cmd.on_complete = on_poll_complete;  // [Task 3] 注册反馈回调
    strncpy(cmd.protocol, ctx->protocol, 15);
    snprintf(cmd.req_id, sizeof(cmd.req_id), "CAL_P_%u", ctx->tag_id);

    LOG_INFO(LOG_CAT_SCHED, "Calendar: Triggering poll for Tag %u on Bus %d [%s]", ctx->tag_id,
             ctx->bus_id, ctx->protocol);

    bus_orchestrator_submit_cmd(ctx->orch, ctx->bus_id, &cmd);
}
static void add_periodic_poll(uv_loop_t *loop, bus_orchestrator_t *orch, int bus_id,
                              const char *proto, uint32_t tag_id, uint32_t interval_ms) {
    calendar_task_t *task = calloc(1, sizeof(calendar_task_t));
    calendar_poll_ctx_t *ctx = calloc(1, sizeof(calendar_poll_ctx_t));

    ctx->orch = orch;
    ctx->bus_id = bus_id;
    ctx->tag_id = tag_id;
    strncpy(ctx->protocol, proto, 15);

    task->loop = loop;
    task->type = CAL_TASK_PERIODIC_READ;
    task->period_sec = interval_ms / 1000;
    if (task->period_sec == 0) task->period_sec = 1;  // 最小 1s
    task->on_fire = on_calendar_poll_fire;
    task->user_data = ctx;

    calendar_schedule(task);
}

/**
 * @brief [Task 2] 根据契约自动加载轮询任务
 */
static void load_contract_polls(uv_loop_t *loop, bus_orchestrator_t *orch) {
    LOG_INFO(LOG_CAT_SCHED, "Calendar: Auto-loading poll tasks from contract...");
    for (uint32_t i = 0; i < g_tag_metadata_len; i++) {
        const edge_tag_meta_t *meta = &g_tag_metadata[i];
        if (meta->poll_interval_ms == 0) continue;

            // 策略：根据测点在不同规约中的绑定关系，决定分发到哪个总线
            // TODO: 建立一个统一的总线-规约映射配置，目前先硬编码总线 ID 对齐 main.c

#ifdef CONFIG_MODBUS_ENABLED
        if (edge_get_map_modbus_by_tag(meta->id)) {
            add_periodic_poll(loop, orch, 2, "ModbusRTU", meta->id, meta->poll_interval_ms);
            continue;
        }
#endif

#ifdef CONFIG_DLMS_ENABLED
        if (edge_get_map_dlms_by_tag(meta->id)) {
            add_periodic_poll(loop, orch, 1, "DLMS", meta->id, meta->poll_interval_ms);
            continue;
        }
#endif

#ifdef CONFIG_DLT645_ENABLED
        if (edge_get_map_dlt645_by_tag(meta->id)) {
            add_periodic_poll(loop, orch, 1, "DLT645-2007", meta->id, meta->poll_interval_ms);
            continue;
        }
#endif
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // 1.1 基础设施
    uv_loop_t *loop = uv_default_loop();
    edge_log_t *logger = edge_log_create(NULL, true);

    // 1.2 数据面
    datahub_t hub;
    datahub_init(&hub, loop, "iems_v4.5", 1024, g_tag_metadata_len, g_tag_metadata,
                 g_tag_metadata_len, edge_tag_get_compact_index, edge_router_needs_wakeup);

    // 1.3 驱动与控制面
    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);

    driver_manager_t *drv_mgr = driver_manager_create();
    dlms_driver_register(drv_mgr);
    dlt645_driver_register(drv_mgr);
    iec104_driver_register(drv_mgr);

    bus_orchestrator_t *io_orch = bus_orchestrator_create(&hub, arbiter, drv_mgr, 8, 32, 2048);

    // [Task 7] 注入监控点位
    bus_orch_monitor_tags_t mtags = {
        .tag_netbuf_total = TAG_SYS_NETBUF_TOTAL,
        .tag_netbuf_free = TAG_SYS_NETBUF_FREE,
        .tag_netbuf_peak = TAG_SYS_NETBUF_PEAK,
        .tag_netbuf_leak = TAG_SYS_NETBUF_LEAK_RECLAIM,
    };
    bus_orchestrator_set_monitor_tags(io_orch, &mtags);

    // --- 总线配置 ---
    driver_contract_injection_t iec_inject = {0};
#ifdef CONFIG_IEC104_ENABLED
    iec_inject.lookup_by_key = (const void *(*)(uint32_t))edge_get_map_iec104;
    iec_inject.lookup_by_tag = (const void *(*)(uint32_t))edge_get_map_iec104_by_tag;
    iec_inject.meta_list = (const void *const *)g_iec104_gi_list;
    iec_inject.meta_list_len = g_iec104_gi_len;
#endif
    bus_orchestrator_setup_slave(io_orch, 0, "IEC104", 1, &iec_inject);

    driver_contract_injection_t dlms_inject = {0};
#ifdef CONFIG_DLMS_ENABLED
    dlms_inject.lookup_by_tag = (const void *(*)(uint32_t))edge_get_map_dlms_by_tag;
#endif
    bus_orchestrator_setup_master(io_orch, 1, "DLMS", 3, &dlms_inject);

    driver_contract_injection_t mb_inject = {0};
#ifdef CONFIG_MODBUS_ENABLED
    mb_inject.lookup_by_key = (const void *(*)(uint32_t))edge_get_map_modbus;
#endif
    bus_orchestrator_setup_master(io_orch, 2, "ModbusRTU", 1, &mb_inject);

    // [Task 2] 配置驱动轮询加载
    load_contract_polls(loop, io_orch);

    // 1.4 其他服务
    sqlite_config_t db_conf = {
        .db_path = "iems.db", .strategy = STRATEGY_LITE, .cache_size_kb = 2048};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *stream_engine = stream_engine_create(&hub, NULL);
    edge_params_t *param_mgr = edge_params_create(db_service);
    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub.ring);

    service_t *svc =
        service_new(loop, &hub, arbiter, db_service, stream_engine, param_mgr, north_mgr);
    g_running_svc = svc;

    signal(SIGTERM, on_sigterm);
    bus_orchestrator_run(io_orch);
    int exit_code = service_run(svc);

    // 清理
    service_delete(svc);
    northbound_mgr_stop(north_mgr);
    free(north_mgr);
    bus_orchestrator_destroy(io_orch);
    stream_engine_destroy(stream_engine);
    driver_manager_destroy(drv_mgr);
    arbiter_destroy(arbiter);
    edge_params_destroy(param_mgr);
    sqlite_svc_destroy(db_service);
    datahub_destroy(&hub);
    edge_log_destroy(logger);

    return exit_code;
}