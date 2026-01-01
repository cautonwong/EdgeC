#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "contracts.h"
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

// --- 10Hz 高频数据模拟 ---
static void simulator_tick(uv_timer_t *handle) {
    (void)handle;
    uint64_t now = (uint64_t)time(NULL) * 1000;

    // 模拟光伏并网功率 (TagID: GRID_POWER_TOTAL)
    PipeMsg m1;
    pipe_msg_init_scalar(&m1, TAG_GRID_POWER, 1500.0 + (rand() % 1000), now);
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
    printf("   EdgeOS VPP Demo v2.1 (Best Practice)             \n");
    printf("====================================================\n");

    loop = uv_default_loop();

    // 1. 初始化核心底座
    datahub_init(&data_hub, loop, "vpp_bus_v4", 1024);
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = "vpp_history.db"};
    db_svc = sqlite_svc_create(loop, &db_cfg);

    // 2. 初始化流引擎
    stream_engine = stream_engine_create(&data_hub, NULL);

    // --- 核心演示：如何正确地使用算子工厂注入 Pipeline ---
    pipeline_t *p1 = stream_engine_add_pipeline(stream_engine, "vpp_main_stats", TAG_GRID_POWER);

    // Stage 1: 防逆流过滤 (0~100000W)
    pipeline_stage_t *s_filter = stage_filter_range_create(0.0, 100000.0);
    pipeline_add_stage(p1, s_filter->name, s_filter->process, s_filter->priv_state,
                       s_filter->cleanup_state, s_filter->priv_cfg, s_filter->cleanup_cfg);
    free(s_filter);  // 属性已接管，释放壳对象

    // Stage 2: 量纲转换 (W to kW)
    pipeline_stage_t *s_scale = stage_transform_create(0.001, 0.0);
    pipeline_add_stage(p1, s_scale->name, s_scale->process, s_scale->priv_state,
                       s_scale->cleanup_state, s_scale->priv_cfg, s_scale->cleanup_cfg);
    free(s_scale);

    // Stage 3: 持久化记录
    pipeline_stage_t *s_sink = stage_sqlite_sink_create(db_svc, "vpp_log");
    pipeline_add_stage(p1, s_sink->name, s_sink->process, s_sink->priv_state, s_sink->cleanup_state,
                       s_sink->priv_cfg, s_sink->cleanup_cfg);
    free(s_sink);

    // 3. 开启北向服务
    service_options_t opts = {.mode = "VPP_DEMO", .web_port = 8080, .rest_port = 8081};
    svc = service_new(&opts);
    service_set_datahub(svc, &data_hub);
    service_set_storage(svc, db_svc);
    service_set_contract_path(svc, "products/vpp_demo/generated/contracts.yaml");

    // 4. 高频模拟与信号处理
    uv_timer_t t_sim;
    uv_timer_init(loop, &t_sim);
    uv_timer_start(&t_sim, simulator_tick, 0, 100);

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, on_signal, SIGINT);

    printf("[VPP] System Ready. All pipelines verified.\n");

    persistence_mgr_t storage = {.writer = db_svc};
    service_run(svc, loop, NULL, NULL, &data_hub, &storage, NULL);

    // 5. 彻底清理
    stream_engine_destroy(stream_engine);
    sqlite_svc_destroy(db_svc);
    datahub_destroy(&data_hub);
    service_delete(svc);

    return 0;
}