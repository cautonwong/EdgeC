#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

#include "contracts.h"
#include "dcu_inventory.h"
#include "driver_manager.h"
#include "edge_params.h"
#include "edgeos.h"
#include "northbound_mgr.h"

extern int modbus_driver_register(driver_manager_t *mgr);
extern void stream_engine_register_sim_pipeline(stream_engine_t *engine, datahub_t *hub,
                                                uint32_t trigger_tag, uint32_t target_tag);

static service_t *g_running_svc = NULL;
static datahub_t *g_hub_ptr = NULL;
static bus_orchestrator_t *g_orch_ptr = NULL;
static stream_engine_t *g_se_ptr = NULL;
static int g_last_adc_fd = -1;
static io_handle_t *g_mqtt_h = NULL;

#define TEST_BUS_ID 0

// --- 1. 信号处理 (Graceful Shutdown) ---
static void on_sigterm_uv(uv_signal_t *handle, int sig) {
    (void)handle;
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

// --- 2. 物理层：SDD 仿真连接逻辑 ---
static int _one_shot_capture_fd(int sock_fd) {
    char control[CMSG_SPACE(sizeof(int))];
    char buf[1];
    struct iovec iov = {.iov_base = buf, .iov_len = 1};
    struct msghdr msg = {.msg_iov = &iov,
                         .msg_iovlen = 1,
                         .msg_control = control,
                         .msg_controllen = sizeof(control)};
    if (recvmsg(sock_fd, &msg, 0) > 0) {
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_type == SCM_RIGHTS) {
            int received_fd = *((int *)CMSG_DATA(cmsg));
            uint8_t ack = 0x06;
            send(sock_fd, &ack, 1, 0);
            return received_fd;
        }
    }
    return -1;
}

static void on_adc_receiver_cb(io_handle_t *h, io_event_t evt, void *data, void *user_data) {
    (void)h;
    (void)user_data;
    if (evt == IO_EVENT_RX && data) {
        net_buf_t *nb = (net_buf_t *)data;
        if (nb->b.len >= 1 && nb->b.data[0] == 0x01 && g_se_ptr && g_last_adc_fd != -1) {
            pipe_msg_t trigger = {.type = PIPEMSG_TYPE_REF,
                                  .tag_id = 12001,
                                  .payload.ref.fd = g_last_adc_fd,
                                  .payload.ref.len = 1600};
            trigger.ts = uv_hrtime();
            stream_engine_push(g_se_ptr, &trigger);
        }
        net_buf_unref(nb);
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
    g_last_adc_fd = _one_shot_capture_fd(fd);
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
    int fd;
    uv_fileno((const uv_handle_t *)req->handle, &fd);
    io_handle_t *h = io_executor_create_from_fd(req->handle->loop, fd, g_hub_ptr->io_pool);
    bus_orchestrator_attach_io(g_orch_ptr, TEST_BUS_ID, h);
    h->ops->enable_read(h, true);
    free(req);
}

static void _try_vbus_connect(uv_timer_t *handle) {
    char *vbus_path = getenv("OMNIS_VBUS_PATH") ?: "/tmp/vbus0.sock";
    if (access(vbus_path, F_OK) != 0) return;
    uv_pipe_t *vbus_client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(handle->loop, vbus_client, 0);
    uv_connect_t *conn_req = malloc(sizeof(uv_connect_t));
    conn_req->data = handle;
    uv_pipe_connect(conn_req, vbus_client, vbus_path, on_vbus_connect);
}

// --- 3. 业务调度：Libev 风格任务 ---

// 3.1 分钟对齐冻结
static void on_minute_freeze(uv_periodic_t *handle) {
    (void)handle;
    LOG_INFO(LOG_CAT_SCHED, "❄️ Clock Aligned Minute Freeze triggered.");
}

// 3.2 周期性抄读 (驱动 Inventory)
static void on_periodic_poll(uv_periodic_t *handle) {
    (void)handle;
    if (g_running_svc) dcu_inventory_load_and_run(g_running_svc);
}

// 3.3 HTTP 14小时周期上报
static void on_http_report_trigger(uv_periodic_t *handle) {
    (void)handle;
    LOG_INFO(LOG_CAT_CORE, "🌐 [14H Cycle] Sending report to httpbin.org...");
    io_executor_http_request(uv_default_loop(), "https://httpbin.org/get", "GET");
}

// 3.4 MQTT 10秒周期同步
static void on_mqtt_sync_trigger(uv_periodic_t *handle) {
    (void)handle;
    if (!g_mqtt_h) return;
    LOG_INFO(LOG_CAT_CORE, "📡 [MQTT] Periodic Heartbeat Publication.");
    char payload[] = "{\"status\": \"online\", \"ver\": \"4.5\"}";
    net_buf_t *nb = net_buf_alloc(bus_orchestrator_get_pool(g_orch_ptr), 0);
    if (nb) {
        net_buf_append_bytes(nb, (uint8_t *)payload, sizeof(payload));
        g_mqtt_h->ops->write_async(g_mqtt_h, nb);
    }
}

// --- 4. 引导程序 (Staged Boot) ---
static void on_boot_delayed(uv_timer_t *handle) {
    service_t *svc = (service_t *)handle->data;
    uv_loop_t *loop = handle->loop;

    // 组装所有周期性任务
    static uv_periodic_t poll_h;
    uv_periodic_init(loop, &poll_h);
    uv_periodic_set(&poll_h, 0, 5000.0, NULL);
    uv_periodic_start(&poll_h, on_periodic_poll);

    static uv_periodic_t align_h;
    uv_periodic_init(loop, &align_h);
    uv_periodic_set(&align_h, 0, 0, sched_policy_at_minute);
    uv_periodic_start(&align_h, on_minute_freeze);

    static uv_periodic_t report_h;
    uv_periodic_init(loop, &report_h);
    extern double sched_policy_14h_from_000030(uv_periodic_t * h, double now);
    uv_periodic_set(&report_h, 0, 0, sched_policy_14h_from_000030);
    uv_periodic_start(&report_h, on_http_report_trigger);

    static uv_periodic_t mqtt_h;
    uv_periodic_init(loop, &mqtt_h);
    uv_periodic_set(&mqtt_h, 0, 10000.0, NULL);
    uv_periodic_start(&mqtt_h, on_mqtt_sync_trigger);

    arbiter_resume(service_get_arbiter(svc));
    LOG_INFO(LOG_CAT_CORE, "✅ IEMS Orchestration Complete. System Online.");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uv_loop_t *loop = uv_default_loop();
    printf("\n🚀 IEMS STARTUP (Sprint 4 Omnis)\n");

    // 1. 初始化基础核心
    arbiter_t *arbiter = NULL;
    arbiter_init(&arbiter, loop);
    driver_manager_t *drv_mgr = driver_manager_create();
    modbus_driver_register(drv_mgr);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    datahub_init(hub, loop, "iems", 1024, 1024, g_tag_metadata, g_tag_metadata_len, NULL,
                 edge_tag_get_compact_index, edge_router_needs_wakeup);
    g_hub_ptr = hub;

    bus_orchestrator_t *io_orch = bus_orchestrator_create(hub, arbiter, drv_mgr, 8, 32, 2048);
    g_orch_ptr = io_orch;
    hub->io_pool = bus_orchestrator_get_pool(io_orch);

    // 2. 初始化持久化与算法引擎
    sqlite_config_t db_conf = {.db_path = "iems.db", .strategy = STRATEGY_LITE};
    sqlite_svc_t *db_service = sqlite_svc_create(loop, &db_conf);
    stream_engine_t *se = stream_engine_create(hub, NULL);
    g_se_ptr = se;
    pipeline_loader_ctx_t pl_ctx = {.db = db_service, .orch = io_orch};
    stream_engine_load_static_metadata(se, g_pipeline_metadata, g_pipeline_metadata_len, &pl_ctx);
    stream_engine_register_sim_pipeline(se, hub, 12001, 1001);

    // 3. 初始化参数管理与北向服务
    edge_params_t *param_mgr = edge_params_create(db_service, hub);
    edge_params_register_metadata(param_mgr, g_param_metadata, g_param_metadata_len);

    northbound_mgr_t *north_mgr = calloc(1, sizeof(northbound_mgr_t));
    northbound_mgr_init(north_mgr, loop, &hub->ring, "iems", db_service);
    north_mgr->rest_port = 9091;

    // 4. 创建总线容器服务
    service_t *svc = service_new(loop, hub, arbiter, db_service, se, param_mgr, north_mgr);
    g_running_svc = svc;
    if (north_mgr) north_mgr->user_data = svc;

    // 5. 创建仿真与演示 IO 句柄
    uv_timer_t *at = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, at);
    uv_timer_start(at, _try_adc_connect, 100, 1000);
    uv_timer_t *vt = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, vt);
    uv_timer_start(vt, _try_vbus_connect, 100, 1000);

    io_config_t mqtt_cfg = {.type = IO_TYPE_MQTT};
    strncpy(mqtt_cfg.params.mqtt.broker, "tcp://broker.hivemq.com:1883", 127);
    g_mqtt_h = io_executor_create(loop, &mqtt_cfg, bus_orchestrator_get_pool(io_orch));

    // 6. 启动引导
    static uv_timer_t boot_timer;
    uv_timer_init(loop, &boot_timer);
    boot_timer.data = svc;
    uv_timer_start(&boot_timer, on_boot_delayed, 1500, 0);

    // 7. 信号监听
    static uv_signal_t sig_term;
    uv_signal_init(loop, &sig_term);
    uv_signal_start(&sig_term, on_sigterm_uv, SIGTERM);

    return service_run(svc);
}
