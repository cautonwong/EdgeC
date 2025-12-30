#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "contracts.h"  // 引用 VPP 专用契约
#include "datahub.h"
#include "edge_params.h"
#include "edge_service.h"
#include "shmring.h"
#include "sqlite_service.h"
#include "stream_engine.h"

static uv_loop_t *loop;
static service_t *svc;
static sqlite_svc_t *db_svc;
static datahub_t data_hub;
static stream_engine_t *stream_engine;

// --- 10Hz 高频数据模拟 (对齐契约) ---
static void simulator_tick(uv_timer_t *handle) {
    (void)handle;
    uint64_t now = (uint64_t)time(NULL) * 1000;

    // 模拟光伏并网功率 (TagID: GRID_POWER_TOTAL)
    PipeMsg m1;
    pipe_msg_init_scalar(&m1, TAG_GRID_POWER_TOTAL, 1500.0 + (rand() % 1000), now);
    datahub_publish(&data_hub, &m1);

    // 模拟电池 SoC (TagID: BATTERY_SOC)
    static double soc = 45.5;
    soc += (rand() % 10) / 100.0;
    if (soc > 100.0) soc = 45.0;

    PipeMsg m2;
    pipe_msg_init_scalar(&m2, TAG_BATTERY_SOC, soc, now);
    datahub_publish(&data_hub, &m2);
}

static void on_signal(uv_signal_t *handle, int signum) {
    (void)handle;
    (void)signum;
    printf("\n[VPP] Graceful shutdown initiated...\n");
    uv_stop(loop);
}

int main(void) {
    printf("====================================================\n");
    printf("   EdgeOS VPP Demo v2.0 (Contract-Aware Version)    \n");
    printf("   Scenario: PV Aggregation & Safety Guarding       \n");
    printf("====================================================\n");

    loop = uv_default_loop();

    // 1. 底座初始化
    datahub_init(&data_hub, loop, "vpp_bus_v3", 1024, NULL, NULL);
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = "vpp_history.db"};
    db_svc = sqlite_svc_create(loop, &db_cfg);

    // 2. 流引擎初始化 (Data Plane)
    stream_engine = stream_engine_create(&data_hub, NULL);

    // --- 核心演示：动态构建计算流水线 (对齐 v2.0 销毁接口) ---
    // 目标：[防逆流过滤] -> [量纲缩放 W to kW] -> [数据库落库]
    pipeline_t *p1 =
        stream_engine_add_pipeline(stream_engine, "pv_main_stats", TAG_GRID_POWER_TOTAL);

    // Stage 1: 防逆流 (0 ~ 100000W 允许通过)
    pipeline_add_stage(p1, "safety_guard", stage_filter_range_create(0.0, 100000.0), NULL, NULL,
                       NULL, free);

    // Stage 2: 量纲转换
    pipeline_add_stage(p1, "scaler", stage_transform_create(0.001, 0.0), NULL, NULL, NULL, free);

    // Stage 3: 持久化
    pipeline_add_stage(p1, "db_logger", stage_sqlite_sink_create(db_svc, "vpp_log"), NULL, NULL,
                       NULL, free);

    stream_engine_start(stream_engine);

    // 3. 服务容器化 (Control Plane)
    service_options_t opts = {.mode = "VPP_DEMO", .web_port = 8080, .rest_port = 8081};
    svc = service_new(&opts);
    service_set_datahub(svc, &data_hub);
    service_set_storage(svc, db_svc);
    service_set_contract_path(svc, "products/vpp_demo/generated/contracts.yaml");

    // 4. 定时器与信号
    uv_timer_t t_sim;
    uv_timer_init(loop, &t_sim);
    uv_timer_start(&t_sim, simulator_tick, 0, 100);

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, on_signal, SIGINT);

    printf("[VPP] System Ready.\n");
    printf("[VPP] API Portal: http://localhost:8081/api/v1/contracts\n");

    persistence_mgr_t storage = {.writer = db_svc};
    service_run(svc, loop, NULL, NULL, &data_hub, &storage, NULL);

    // 5. 销毁逻辑
    stream_engine_destroy(stream_engine);
    sqlite_svc_destroy(db_svc);
    datahub_destroy(&data_hub);
    service_delete(svc);

    return 0;
}
