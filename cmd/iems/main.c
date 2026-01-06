/**
 * @file main.c
 * @brief IEMS Controller Entry Point (Container Orchestration Mode)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contracts.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "policy_syncer.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static service_t *g_svc = NULL;

// [外部函数声明]
extern int io_channel_open(const char *path, int baud);
extern void bus_orchestrator_attach_fd(bus_orchestrator_t *ctx, int bus_id, int fd);
extern int edge_get_app_root(char *out_path, size_t size);

// --- 模拟数据生成器 ---
static void sim_data_tick(uv_timer_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    uint64_t now = (uint64_t)time(NULL) * 1000;
    pipe_msg_t msg;

    pipe_msg_init_scalar(&msg, TAG_VOLTAGE_PHASE_A, 220.0 + (rand() % 100) / 10.0, now);
    datahub_publish(hub, &msg);
}

static void l2_thread_entry(void *arg) {
    datahub_t *hub = (datahub_t *)arg;
    uv_loop_t l2_loop;
    uv_loop_init(&l2_loop);
    stream_engine_t *engine = stream_engine_create_ex(hub, &l2_loop, NULL);
    uv_run(&l2_loop, UV_RUN_DEFAULT);
    stream_engine_destroy(engine);
    uv_loop_close(&l2_loop);
}

static void handle_sigpwr(int sig) {
    (void)sig;
    if (g_svc) service_stop(g_svc);
}

static void on_smoke_timeout(uv_timer_t *handle) {
    LOG_INFO(LOG_CAT_CORE, "Smoke Test Timeout Triggered.");
    service_stop((service_t *)handle->data);
}

int main(int argc, char **argv) {
    srand((unsigned int)time(NULL));
    edge_log_init(NULL, false);

    const char *io_port = NULL;
    bool smoke_mode = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc)
            io_port = argv[++i];
        else if (strcmp(argv[i], "--smoke-test") == 0)
            smoke_mode = true;
    }

    if (edge_contract_self_test() != 0) return EXIT_FAILURE;

    service_options_t opts = {.product_name = "iems_v1",
                              .db_path = "iems_data.db",
                              .rest_port = 8081,
                              .mode = "PRODUCTION"};
    g_svc = service_new(&opts);
    if (smoke_mode) service_enable_smoke_test(g_svc);

    // [专家级补强] 注入运行期整定参数元数据
    edge_params_register_metadata(service_get_params(g_svc), g_param_metadata,
                                  g_param_metadata_len);

    uv_loop_t *loop = service_get_loop(g_svc);
    datahub_t *hub = service_get_datahub(g_svc);
    arbiter_t *arb = service_get_arbiter(g_svc);

    uv_timer_t sim_timer;
    if (!smoke_mode) {
        uv_timer_init(loop, &sim_timer);
        sim_timer.data = hub;
        uv_timer_start(&sim_timer, sim_data_tick, 1000, 1000);
    }

    extern int dlt645_driver_register(void);
    extern int dlms_driver_register(void);
    driver_manager_init();
    dlt645_driver_register();
    dlms_driver_register();
    bus_orchestrator_t *orch = bus_orchestrator_create(
        hub, arb, CONFIG_BUS_MAX_COUNT, CONFIG_NETBUF_POOL_SIZE, CONFIG_NETBUF_BLOCK_SIZE);

    if (io_port) {
        int fd = io_channel_open(io_port, 9600);
        if (fd >= 0) {
            bus_orchestrator_attach_fd(orch, 0, fd);
            // 冒烟测试：统一使用 DLMS 验证全链路
            bus_orchestrator_add_poll(orch, "DLMS", 1, 1001, 1000);
        }
    }

    policy_syncer_run(service_get_storage(g_svc), orch);

    uv_thread_t l2_thread;
    uv_thread_create(&l2_thread, l2_thread_entry, hub);

    uv_timer_t smoke_timer;
    if (smoke_mode) {
        uv_timer_init(loop, &smoke_timer);
        smoke_timer.data = g_svc;
        uv_timer_start(&smoke_timer, on_smoke_timeout, 5000, 0);
    }

    signal(SIGTERM, handle_sigpwr);
    bus_orchestrator_run(orch);
    int ret = service_run(g_svc);

    if (!smoke_mode) uv_timer_stop(&sim_timer);
    uv_thread_join(&l2_thread);
    bus_orchestrator_destroy(orch);
    service_delete(g_svc);
    edge_log_fini();
    return ret;
}