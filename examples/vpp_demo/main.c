#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "battery_strategy.h"
#include "edge_service.h"
#include "js_engine.h"
#include "script_loader.h"

static uv_loop_t *loop;
static js_engine_t *js_engine;
static script_loader_t *loader;
static int running = 1;

static void on_signal(uv_signal_t *handle, int signum) {
    (void)handle;
    (void)signum;
    printf("\n[VPP] Stopping...\n");
    running = 0;
    uv_stop(loop);
}

static void on_schedule_tick(uv_timer_t *handle) {
    (void)handle;

    // 1. 模拟电池实时状态 (通常应从 DataHub 读取)
    battery_state_t state = {.soc = 15.0f,  // 假设当前电量较低
                             .temperature = 30.0f,
                             .rated_capacity = 5000.0f};

    battery_limit_t limits = {.min_soc = 10.0f,
                              .max_soc = 95.0f,
                              .max_charge_pwr = 5000.0f,
                              .max_discharge_pwr = 5000.0f};

    // 2. 执行 JS 调度策略 (battery_dispatch.js)
    // 脚本会根据当前时间输出 target_pwr
    // 注意：这里简化为直接打印路径，实际中应使用 script_executor
    printf("[VPP] Tick. Date: %s", "2025-12-28 (Simulated)\n");

    // 模拟从 JS 获取的决策
    // 假设现在是峰时，JS 请求放电 -3000W
    float js_target_pwr = -3000.0f;
    printf("[JS Strategy] Requested ESS Power: %.1f W\n", js_target_pwr);

    // 3. 调用 Native C 安全算法进行边界检查
    float safe_pwr = algo_battery_safety_clamp(&state, &limits, js_target_pwr);

    // 4. 输出最终决策
    if (safe_pwr != js_target_pwr) {
        printf(
            "[Native Safety] CLAMPED! JS requested %.1f W, but safety limit forced %.1f W (SoC "
            "Low)\n",
            js_target_pwr, safe_pwr);
    } else {
        printf("[Execution] Safe Power: %.1f W\n", safe_pwr);
    }
}

int main(void) {
    loop = uv_default_loop();

    // 初始化 JS 引擎和加载器 (演示用)
    js_engine = js_engine_create(NULL, 1024 * 1024);
    loader = script_loader_create(loop, js_engine, "algo/js", 1000);

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, on_signal, SIGINT);

    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    uv_timer_start(&timer, on_schedule_tick, 1000, 3000);  // 3秒执行一次

    printf("[VPP] Hybrid Strategy Demo Started (Native C + JS).\n");
    printf("[VPP] Strategy: algo/js/battery_dispatch.js\n");
    printf("[VPP] Safety: algo/native/battery_strategy.c\n");

    uv_run(loop, UV_RUN_DEFAULT);

    script_loader_destroy(loader);
    js_engine_destroy(js_engine);

    return 0;
}
