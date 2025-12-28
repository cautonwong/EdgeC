#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

// EdgeOS SDK 核心组件
#include "arbiter.h"
#include "battery_strategy.h"
#include "datahub.h"
#include "edge_params.h"
#include "edge_service.h"
#include "js_engine.h"
#include "script_loader.h"
#include "shmring.h"
#include "sqlite_service.h"

static uv_loop_t *loop;
static service_t *svc;
static js_engine_t *js_engine;
static script_loader_t *loader;
static sqlite_svc_t *db_svc;
static datahub_t data_hub;
static uint64_t vpp_last_seq = 0;

// --- 业务逻辑：VPP 核心控制循环 ---
static void vpp_logic_tick(uv_timer_t *handle) {
    (void)handle;
    printf("\n--- [VPP Strategy Cycle] ---\n");

    datahub_t *hub = service_get_datahub(svc);
    arbiter_t *arb = service_get_arbiter(svc);
    if (!hub || !arb) return;

    // 1. [全场景场景：参数感知] 从持久化数据库读取当前的运行约束
    float max_pwr = (float)edge_param_get_float("ess_max_charge", 3000.0);
    float min_soc = (float)edge_param_get_float("ess_min_soc", 15.0);

    // 2. [全场景场景：数据融合] 从 SHM 总线获取实时输入
    float pv_pwr = 0, batt_soc = 0;
    PipeMsg msg;
    while (shm_ring_read(&hub->ring, &vpp_last_seq, &msg) == 0) {
        if (msg.tag_id == 1002) pv_pwr = (float)msg.payload.val.f;
        if (msg.tag_id == 2001) batt_soc = (float)msg.payload.val.f;
    }
    printf("[Input] PV: %.1fW, SOC: %.1f%% (LastSeq: %lu)\n", (double)pv_pwr, (double)batt_soc,
           vpp_last_seq);

    // 3. [全场景场景：动态优化] 根据环境自动调整系统参数
    // 如果光伏大于 2000W，我们临时提高充电限值 (Self-Optimization)
    if (pv_pwr > 2000.0f && max_pwr < 4500.0f) {
        printf("[Optimize] High PV detected. Boosting charge limit...\n");
        edge_param_set_float("ess_max_charge", 5000.0);
    }

    // 4. [全场景场景：业务策略] 模拟 JS 侧生成的调度请求
    // 逻辑：优先消纳光伏，如果 SoC 低于阈值则强制充电
    float target_pwr = (batt_soc < min_soc) ? 1500.0f : (pv_pwr * 0.8f);
    printf("[Decision] JS Strategy suggested: %.1fW\n", (double)target_pwr);

    // 5. [全场景场景：安全防护] Native C 算法实施最后的安全拦截
    battery_state_t b_state = {.soc = batt_soc, .temperature = 25.0f};
    battery_limit_t b_limits = {.min_soc = min_soc,
                                .max_soc = 98.0f,
                                .max_charge_pwr = max_pwr,
                                .max_discharge_pwr = 3000.0f};
    float safe_pwr = algo_battery_safety_clamp(&b_state, &b_limits, target_pwr);

    // 6. [全场景场景：执行控制] 通过 Arbiter 下发指令
    if (safe_pwr != target_pwr) {
        printf("[Safety] PROTECTED! Clamped %.1fW -> %.1fW (Check SOC/Limit)\n", (double)target_pwr,
               (double)safe_pwr);
    }
    cmd_msg_t cmd;
    cmd_msg_init_write(&cmd, 2002, (double)safe_pwr, CMD_PRIO_NORMAL);
    arbiter_submit_cmd(arb, 0, &cmd);

    // 7. [全场景场景：状态回馈] 将系统运行状态写回总线，供北向查询
    // 假设 Tag 3001 为“系统健康状态”
    PipeMsg status_msg;
    pipe_msg_init_scalar(&status_msg, 3001, (safe_pwr > 0 ? 1.0 : 0.0),
                         (uint64_t)time(NULL) * 1000);
    datahub_publish(hub, &status_msg);
    printf("[Output] Command sent. Status feedback published.\n");
}

// --- 模拟器：模拟南向设备数据 ---
static void simulator_tick(uv_timer_t *handle) {
    (void)handle;
    datahub_t *hub = service_get_datahub(svc);
    uint64_t now = (uint64_t)time(NULL) * 1000;

    // 模拟高光伏场景
    static float base_pv = 1800.0f;
    base_pv += (float)(rand() % 200 - 100);
    PipeMsg m1;
    pipe_msg_init_scalar(&m1, 1002, (double)base_pv, now);
    datahub_publish(hub, &m1);

    // 模拟 SOC 处于临界值
    static float s_soc = 14.5f;
    s_soc += 0.05f;
    PipeMsg m2;
    pipe_msg_init_scalar(&m2, 2001, (double)s_soc, now);
    datahub_publish(hub, &m2);
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
    printf("====================================================\n");

    loop = uv_default_loop();

    // 1. 组件组装
    datahub_init(&data_hub, loop, "vpp_bus", 1024, NULL, NULL);
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = "vpp_db.sqlite"};
    db_svc = sqlite_svc_create(loop, &db_cfg);

    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 128, 512, NULL, NULL);

    arbiter_t *arb;
    arbiter_init(&arb, loop);

    // 2. 参数初始化
    edge_param_init(db_svc);
    edge_param_set_float("ess_max_charge", 2500.0);
    edge_param_set_float("ess_min_soc", 15.0);

    // 3. 容器配置
    service_options_t opts = {.mode = "VPP_ULTIMATE", .web_port = 8080, .rest_port = 8081};
    svc = service_new(&opts);

    // 4. 逻辑引擎配置 (注入 console.log 后将看到 JS 输出)
    js_engine = js_engine_create(svc, 1024 * 1024);
    loader = script_loader_create(loop, js_engine, "algo/js", 5000);
    script_loader_start(loader);

    // 5. 任务注册
    uv_timer_t t1, t2;
    uv_timer_init(loop, &t1);
    uv_timer_init(loop, &t2);
    uv_timer_start(&t1, vpp_logic_tick, 1000, 2000);
    uv_timer_start(&t2, simulator_tick, 0, 1000);

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, on_signal, SIGINT);

    // 6. 运行
    printf("[VPP] System Online. Listening for data & control...\n");
    service_run(svc, loop, &pool, arb, &data_hub, NULL, NULL);

    // 7. 清理
    printf("[VPP] Releasing resources...\n");
    uv_timer_stop(&t1);
    uv_timer_stop(&t2);
    script_loader_destroy(loader);
    js_engine_destroy(js_engine);
    sqlite_svc_destroy(db_svc);
    datahub_destroy(&data_hub);
    arbiter_shutdown(arb);
    net_buf_pool_destroy(&pool);
    service_delete(svc);

    printf("[VPP] Clean Shutdown.\n");
    return 0;
}
