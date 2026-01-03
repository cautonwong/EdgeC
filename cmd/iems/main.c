/**
 * @file main.c
 * @brief IEMS Controller Entry Point (Container Orchestration Mode)
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contracts.h"
#include "datahub.h"
#include "edge_log.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "policy_syncer.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static service_t *g_svc = NULL;

// --- 模拟数据生成器 (确保 Web UI 有数可显) ---
static void sim_data_tick(uv_timer_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    uint64_t now = (uint64_t)time(NULL) * 1000;
    pipe_msg_t msg;

    // 1. 模拟电压 (220V 左右波动)
    pipe_msg_init_scalar(&msg, TAG_VOLTAGE_PHASE_A, 220.0 + (rand() % 100) / 10.0, now);
    datahub_publish(hub, &msg);

    // 2. 模拟电网功率
    pipe_msg_init_scalar(&msg, TAG_GRID_POWER, 1500.0 + (rand() % 500), now);
    datahub_publish(hub, &msg);

    // 3. 模拟累积电量
    static double energy = 1234.56;
    energy += 0.01;
    pipe_msg_init_scalar(&msg, TAG_ENERGY_ACTIVE_IMPORT, energy, now);
    datahub_publish(hub, &msg);

    // 4. 模拟电池 SOC
    pipe_msg_init_scalar(&msg, TAG_BATTERY_SOC, 65.0 + (rand() % 5), now);
    datahub_publish(hub, &msg);

    // 5. 模拟系统频率
    pipe_msg_init_scalar(&msg, TAG_GRID_FREQUENCY, 49.98 + (rand() % 4) * 0.01, now);
    datahub_publish(hub, &msg);

    // 6. 模拟电池实时功率
    pipe_msg_init_scalar(&msg, TAG_BATTERY_POWER, (double)((rand() % 2000) - 1000), now);
    datahub_publish(hub, &msg);
}

static void l2_thread_entry(void *arg) {
    datahub_t *hub = (datahub_t *)arg;
    uv_loop_t l2_loop;
    uv_loop_init(&l2_loop);

    LOG_INFO(LOG_CAT_SCHED, "L2 Stream Thread starting...");
    stream_engine_t *engine = stream_engine_create_ex(hub, &l2_loop, NULL);
    uv_run(&l2_loop, UV_RUN_DEFAULT);
    stream_engine_destroy(engine);
    uv_loop_close(&l2_loop);
}

static void handle_sigpwr(int sig) {
    (void)sig;
    if (g_svc) {
        LOG_INFO(LOG_CAT_CORE, "Power Signal Detected. Stopping Service.");
        service_stop(g_svc);
    }
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    srand((unsigned int)time(NULL));
    edge_log_init(NULL, false);

    // 0. 契约一致性自检
    if (edge_contract_self_test() != 0) {
        LOG_ERROR(LOG_CAT_CORE, "Contract Self-test FAILED! Aborting.");
        return EXIT_FAILURE;
    }

    LOG_INFO(LOG_CAT_SCHED, "Initializing IEMS Controller [Service Container Mode]...");

    // 1. 利用容器自组装核心平面 (Foundation, Control, Northbound)
    service_options_t opts = {.product_name = "iems_v1",
                              .db_path = "iems_data.db",
                              .shm_slots = 1024,
                              .web_port = 8080,
                              .rest_port = 8081,
                              .mode = "PRODUCTION"};

    g_svc = service_new(&opts);
    uv_loop_t *loop = service_get_loop(g_svc);
    datahub_t *hub = service_get_datahub(g_svc);
    arbiter_t *arb = service_get_arbiter(g_svc);
    sqlite_svc_t *db = service_get_storage(g_svc);

    // [全场景补强] 初始化 IEMS 产品特定参数 (基于 TagID + BLOB)

    double max_soc = 95.0, min_soc = 15.0, grid_limit = 5000.0;

    sqlite_svc_set_param(db, "IEMS", 5001, &max_soc, sizeof(max_soc));

    sqlite_svc_set_param(db, "IEMS", 5002, &min_soc, sizeof(min_soc));

    sqlite_svc_set_param(db, "IEMS", 5003, &grid_limit, sizeof(grid_limit));

    LOG_INFO(LOG_CAT_SCHED, "IEMS: Global parameters synchronized to BLOB persistent storage.");

    // 2. 启动数据模拟定时器 (1秒一次)
    uv_timer_t sim_timer;
    uv_timer_init(loop, &sim_timer);
    sim_timer.data = hub;
    uv_timer_start(&sim_timer, sim_data_tick, 1000, 1000);

    // 3. 执行面 (Execution Plane)
    extern edge_driver_t dlt645_driver;
    bus_orchestrator_t *orch = bus_orchestrator_create(&dlt645_driver, hub, arb);

    // 4. 业务逻辑初始化 (Scenario S1: SQLite Driven)
    policy_syncer_run(service_get_storage(g_svc), orch);

    // 自适应路径探测 (Relocatable Logic)
    char app_root[512] = {0};
    edge_get_app_root(app_root, sizeof(app_root));

    char r_web[1024], r_contract[1024];
    snprintf(r_web, sizeof(r_web), "%s/www/iems", app_root);
    snprintf(r_contract, sizeof(r_contract), "%s/contracts/iems/contracts.yaml", app_root);

    const char *contract_candidates[] = {
        r_contract, "./build/products/iems/generated/contracts.yaml", "../generated/contracts.yaml",
        "./generated/contracts.yaml"};

    // 探测 Web
    // 注意：g_north 是在 edge_service 内部由 service_new 创建的，
    // 这里我们直接通过 service 接口设置属性 (如果 service 提供了接口)
    // 实际上我们在 edge_service.c 已经加了探测。
    // 为了双重保险，我们保留这里的日志输出。

    for (int i = 0; i < 4; i++) {
        if (access(contract_candidates[i], F_OK) == 0) {
            service_set_contract_path(g_svc, contract_candidates[i]);
            LOG_INFO(LOG_CAT_CORE, "Contract Probe: SUCCESS at %s", contract_candidates[i]);
            break;
        }
    }

    // 6. 启动 L2 计算平面线程 (Compute Plane Isolation)
    uv_thread_t l2_thread;
    uv_thread_create(&l2_thread, l2_thread_entry, hub);

    // 7. 注册信号监听
    signal(SIGPWR, handle_sigpwr);
    signal(SIGTERM, handle_sigpwr);

    // 8. 启动系统 (L1 Loop)
    LOG_INFO(LOG_CAT_SCHED, "IEMS LIVE. Entering Loop...");
    bus_orchestrator_run(orch);
    int ret = service_run(g_svc);

    // 9. 物理清理 (由容器接管核心组件释放)
    uv_timer_stop(&sim_timer);
    uv_thread_join(&l2_thread);
    bus_orchestrator_destroy(orch);
    service_delete(g_svc);
    edge_log_fini();

    return ret;
}
