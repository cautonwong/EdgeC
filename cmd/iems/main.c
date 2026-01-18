#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
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

extern int modbus_driver_register(driver_manager_t *mgr);
extern void stream_engine_register_sim_pipeline(stream_engine_t *engine, datahub_t *hub,
                                                uint32_t trigger_tag, uint32_t target_tag);

static service_t *g_running_svc = NULL;
static datahub_t *g_hub_ptr = NULL;
static bus_orchestrator_t *g_orch_ptr = NULL;
static driver_contract_injection_t g_modbus_inject;
static stream_engine_t *g_se_ptr = NULL;
static int g_last_adc_fd = -1;

#define TEST_BUS_ID 0

static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

static void on_adc_receiver_cb(io_handle_t *h, io_event_t evt, void *data, void *user_data) {
    (void)h;
    (void)user_data;
    if (evt == IO_EVENT_RX && data) {
        // FIXME: ADC 逻辑需要适配统一 IO 接口
        // 原本逻辑期望收到 pipe_msg_t，但 RX 目前返回 net_buf_t
        net_buf_unref((net_buf_t *)data);
    }
}

static void on_adc_connect(uv_connect_t *req, int status) {
    if (status < 0) {
        uv_close((uv_handle_t *)req->handle, (uv_close_cb)free);
        free(req);
        return;
    }
    int fd;
    uv_fileno((const uv_handle_t *)req->handle, &fd);
    io_handle_t *ah = io_executor_create_from_fd(req->handle->loop, fd, g_hub_ptr->io_pool);
    if (ah) {
        ah->cb = on_adc_receiver_cb;
        ah->ops->enable_read(ah, true);
    }
    free(req);
}

static void _try_adc_connect(uv_timer_t *handle) {
    if (access("/tmp/v_adc.sock", F_OK) != 0) return;
    uv_pipe_t *pipe = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(handle->loop, pipe, 0);
    uv_connect_t *req = malloc(sizeof(uv_connect_t));
    uv_pipe_connect(req, pipe, "/tmp/v_adc.sock", on_adc_connect);
}

static void on_vbus_connect(uv_connect_t *req, int status) {
    uv_timer_t *reconnect_timer = (uv_timer_t *)req->data;
    if (status < 0) {
        uv_close((uv_handle_t *)req->handle, (uv_close_cb)free);
        free(req);
        return;
    }
    uv_timer_stop(reconnect_timer);
    uv_pipe_t *pipe = (uv_pipe_t *)req->handle;
    int fd;
    uv_fileno((const uv_handle_t *)pipe, &fd);
    io_handle_t *h = io_executor_create_from_fd(pipe->loop, fd, g_hub_ptr->io_pool);
    bus_orchestrator_attach_io(g_orch_ptr, TEST_BUS_ID, h);
    h->ops->enable_read(h, true);
    free(req);
}

static void _try_vbus_connect(uv_timer_t *handle) {
    char *vbus_path = getenv("OMNIS_VBUS_PATH");
    if (!vbus_path) vbus_path = "/tmp/vbus0.sock";
    if (access(vbus_path, F_OK) != 0) return;
    uv_pipe_t *vbus_client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(handle->loop, vbus_client, 0);
    uv_connect_t *conn_req = malloc(sizeof(uv_connect_t));
    conn_req->data = handle;
    uv_pipe_connect(conn_req, vbus_client, vbus_path, on_vbus_connect);
}

// [Sched Callback] 分钟冻结
static void on_minute_freeze(uv_periodic_t *handle) {
    (void)handle;
    wall_time_t now = sched_wall_now();
    LOG_INFO(LOG_CAT_SCHED, "❄️ Minute Freeze Triggered at %lu (Aligned)", now);
    // 实际业务：触发数据快照、存储
}

// [Sched Callback] 周期轮询
static void on_periodic_poll(uv_periodic_t *handle) {
    (void)handle;
    if (!g_orch_ptr) return;

    // 简化逻辑：每次触发都扫描一遍需要轮询的点
    // 生产环境应按频率分组，避免无效遍历
    for (uint32_t i = 0; i < g_tag_metadata_len && i < 1024; i++) {
        const edge_tag_meta_t *meta = &g_tag_metadata[i];
        if (meta->id == 1001) continue;  // Skip voltage (example)
        if (meta->poll_interval_ms > 0) {
            cmd_msg_t cmd = {0};
            cmd_msg_init_read(&cmd, meta->id, ARBITER_PRIO_NORMAL);
            strncpy(cmd.protocol, "ModbusRTU", 15);
            bus_orchestrator_submit_cmd(g_orch_ptr, TEST_BUS_ID, &cmd);
        }
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uv_loop_t *loop = uv_default_loop();
    printf("\n🚀 IEMS: UNIFIED PRODUCTION & SIMULATION\n");

    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    modbus_driver_register(drv_mgr);

    g_modbus_inject.lookup_by_key = (void *)edge_get_map_modbus;
    g_modbus_inject.lookup_by_tag = (void *)edge_get_map_modbus_by_tag;

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    datahub_init(hub, loop, "iems", 1024, 1024, g_tag_metadata, g_tag_metadata_len, NULL,
                 edge_tag_get_compact_index, edge_router_needs_wakeup);
    g_hub_ptr = hub;

    bus_orchestrator_t *io_orch = bus_orchestrator_create(hub, arbiter, drv_mgr, 8, 32, 2048);
    hub->io_pool = bus_orchestrator_get_pool(io_orch);
    g_orch_ptr = io_orch;
    bus_orchestrator_setup_master(io_orch, TEST_BUS_ID, "ModbusRTU", TEST_BUS_ID, &g_modbus_inject);

    sqlite_config_t db_conf = {.db_path = "iems.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *se = stream_engine_create(hub, NULL);
    g_se_ptr = se;

    pipeline_loader_ctx_t pl_ctx = {.db = db_service, .orch = io_orch};
    stream_engine_load_static_metadata(se, g_pipeline_metadata, g_pipeline_metadata_len, &pl_ctx);
    stream_engine_register_sim_pipeline(se, hub, 12001, 1001);

    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub->ring, "iems", db_service);
    const char *env_port = getenv("EDGE_REST_PORT");
    north_mgr->rest_port = env_port ? (uint16_t)atoi(env_port) : 9091;

    edge_params_t *param_mgr = edge_params_create(db_service, hub);
    edge_params_register_metadata(param_mgr, g_param_metadata, g_param_metadata_len);

    service_t *svc = service_new(loop, hub, arbiter, db_service, se, param_mgr, north_mgr);
    g_running_svc = svc;

    uv_timer_t *at = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, at);
    uv_timer_start(at, _try_adc_connect, 100, 1000);
    uv_timer_t *vt = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, vt);
    uv_timer_start(vt, _try_vbus_connect, 100, 1000);

    // [Sched] 1. 分钟冻结任务 (00秒对齐)
    static uv_periodic_t freeze_handle = {0};
    freeze_handle.loop = loop;
    freeze_handle.reschedule_cb = sched_policy_at_minute;
    freeze_handle.offset_ms = 0;  // 0秒偏移
    uv_periodic_start(&freeze_handle, on_minute_freeze);

    // [Sched] 2. 周期轮询任务 (每5秒对齐)
    static uv_periodic_t poll_handle = {0};
    poll_handle.loop = loop;
    poll_handle.interval_ms = 5000;
    poll_handle.offset_ms = 0;
    uv_periodic_start(&poll_handle, on_periodic_poll);

    // [Sched] 启动时间跳变监控
    extern void scheduler_init(uv_loop_t * loop);
    scheduler_init(loop);

    signal(SIGTERM, on_sigterm);
    return service_run(svc);
}
