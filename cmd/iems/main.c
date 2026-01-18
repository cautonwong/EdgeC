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
static driver_contract_injection_t g_modbus_inject = {0};
static stream_engine_t *g_se_ptr = NULL;
static int g_last_adc_fd = -1;

#define TEST_BUS_ID 0

static void on_sigterm(int sig) {
    (void)sig;
    if (g_running_svc) service_stop(g_running_svc);
}

// 物理级：处理通过 SCM_RIGHTS 传递的 memfd
static int _one_shot_capture_fd(int sock_fd) {
    char control[CMSG_SPACE(sizeof(int))];
    char buf[1];
    struct iovec iov = {.iov_base = buf, .iov_len = 1};
    struct msghdr msg = {.msg_iov = &iov,
                         .msg_iovlen = 1,
                         .msg_control = control,
                         .msg_controllen = sizeof(control)};

    ssize_t n = recvmsg(sock_fd, &msg, 0);  // 阻塞读取，直到 Python 发来第一个包
    if (n > 0) {
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_type == SCM_RIGHTS) {
            int received_fd = *((int *)CMSG_DATA(cmsg));
            // 回 ACK
            uint8_t ack = 0x06;
            send(sock_fd, &ack, 1, 0);
            return received_fd;
        }
    }
    return -1;
}

// [物理连接] ADC 模拟器回调
static void on_adc_receiver_cb(io_handle_t *h, io_event_t evt, void *data, void *user_data) {
    (void)h;
    (void)user_data;
    if (evt == IO_EVENT_RX && data) {
        net_buf_t *nb = (net_buf_t *)data;
        // 如果收到 \x01 (触发脉冲) 且 FD 已就绪
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

    // 1. 同步抢夺 FD (此时 Libuv 还没接管读取)
    g_last_adc_fd = _one_shot_capture_fd(fd);
    if (g_last_adc_fd != -1) {
        LOG_INFO(LOG_CAT_CORE, "✅ Evidence: Handshake Success. FD=%d", g_last_adc_fd);
    }

    // 2. 移交给 HAL 层进行异步触发脉冲监听
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

// [物理连接] VBus
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

static void on_boot_delayed(uv_timer_t *handle) {
    service_t *svc = (service_t *)handle->data;
    arbiter_resume(service_get_arbiter(svc));
    LOG_INFO(LOG_CAT_CORE, "✅ Arbiter Resumed.");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    uv_loop_t *loop = uv_default_loop();
    printf("\n🚀 IEMS: HARNESS STARTUP\n");

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
    if (north_mgr) north_mgr->user_data = svc;

    uv_timer_t *at = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, at);
    uv_timer_start(at, _try_adc_connect, 100, 1000);
    uv_timer_t *vt = malloc(sizeof(uv_timer_t));
    uv_timer_init(loop, vt);
    uv_timer_start(vt, _try_vbus_connect, 100, 1000);

    static uv_timer_t boot_timer;
    uv_timer_init(loop, &boot_timer);
    boot_timer.data = svc;
    uv_timer_start(&boot_timer, on_boot_delayed, 1500, 0);

    signal(SIGTERM, on_sigterm);
    return service_run(svc);
}