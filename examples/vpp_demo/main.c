#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "battery_strategy.h"
#include "datahub.h"
#include "edge_params.h"
#include "edge_service.h"
#include "js_engine.h"
#include "script_loader.h"
#include "shmring.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static uv_loop_t *loop;
static service_t *svc;
static sqlite_svc_t *db_svc;
static datahub_t data_hub;
static stream_engine_t *stream_engine;

// --- 模拟南向数据上报 (10Hz 高频模拟) ---
static void simulator_tick(uv_timer_t *handle) {
    (void)handle;
    uint64_t now = (uint64_t)time(NULL) * 1000;

    // 模拟光伏功率 (Tag 1002)
    PipeMsg m1;
    pipe_msg_init_scalar(&m1, 1002, 1500.0 + (rand() % 1000), now);
    datahub_publish(&data_hub, &m1);

    // 模拟 SoC (Tag 2001)
    static float soc = 14.8f;
    soc += 0.01f;
    PipeMsg m2;
    pipe_msg_init_scalar(&m2, 2001, (double)soc, now);
    datahub_publish(&data_hub, &m2);
}

static void on_signal(uv_signal_t *handle, int signum) {
    (void)handle;
    (void)signum;
    printf("\n[VPP] Shutdown signal received.\n");
    uv_stop(loop);
}

int main(void) {
    srand((unsigned int)time(NULL));
    printf("====================================================\n");
    printf("   EdgeOS SDK Ultimate Example: Full-Scenario VPP   \n");
    printf("   Testing StreamEngine & DataPlane Isolation       \n");
    printf("====================================================\n");

    loop = uv_default_loop();

    // 1. 初始化核心组件
    datahub_init(&data_hub, loop, "vpp_bus", 1024, NULL, NULL);

    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = "vpp_db.sqlite"};
    db_svc = sqlite_svc_create(loop, &db_cfg);

    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 128, 512, NULL, NULL);

    arbiter_t *arb;
    arbiter_init(&arb, loop);

    // 2. 初始化持久化参数
    edge_param_init(db_svc);
    edge_param_set_float("ess_min_soc", 15.0);

    // 3. 初始化流处理引擎 (Data Plane)
    stream_engine = stream_engine_create(&data_hub, NULL);

    // 创建一个 Pipeline：分钟聚合流水线
    // trigger_tag = 1002 (PV Power)
    // pipeline_t *p1 = stream_engine_add_pipeline(stream_engine, "pv_agg", 1002);
    // TODO: 注册具体 Stage

    stream_engine_start(stream_engine);

    // 4. 初始化容器与服务
    service_options_t opts = {.mode = "VPP_ULTIMATE", .web_port = 8080, .rest_port = 8081};
    svc = service_new(&opts);

    // 5. 定时模拟任务
    uv_timer_t t_sim;
    uv_timer_init(loop, &t_sim);
    uv_timer_start(&t_sim, simulator_tick, 0, 100);  // 100ms 一次，高频模拟

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, on_signal, SIGINT);

    // 6. 运行
    printf("[VPP] System Online. DataHub <-> StreamEngine link established.\n");
    service_run(svc, loop, &pool, arb, &data_hub, NULL, NULL);

    // 7. 清理
    printf("[VPP] Releasing resources...\n");
    stream_engine_destroy(stream_engine);
    sqlite_svc_destroy(db_svc);
    datahub_destroy(&data_hub);
    service_delete(svc);

    return 0;
}
