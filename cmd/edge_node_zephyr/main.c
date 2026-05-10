#include <string.h>

#include "uv_rtos.h"

/* --- 电表核心上下文 --- */
typedef struct {
    uint64_t total_energy_pulse;
    uint32_t active_power;
    uv_async_t pulse_async;
    uv_check_t safety_check;
} metrology_t;

static metrology_t g_meter;

/* --- 1. 模拟计量脉冲 (由中断 ISR 触发) --- */
void on_energy_pulse_async(uv_async_t* handle) {
    metrology_t* m = edge_container_of(handle, metrology_t, pulse_async);
    m->total_energy_pulse++;
    edge_os_log("[MainLoop] Energy pulse received! Total: %llu\n", m->total_energy_pulse);
}

/* --- 2. 安全自检 (每一轮循环执行一次) --- */
void on_safety_check(uv_check_t* handle) {
    static uint64_t last_pulse = 0;
    metrology_t* m = edge_container_of(handle, metrology_t, safety_check);

    // 逻辑：如果电量脉冲增长过快（如一秒内增长 10000），怀疑电流过载或硬件故障
    if (m->total_energy_pulse - last_pulse > 1000) {
        edge_os_log("[SAFETY] ALERT: Power overload or metrology error detected!\n");
    }
    last_pulse = m->total_energy_pulse;
}

/* --- 3. 模拟硬件中断的后台线程 --- */
void metrology_hw_sim_thread(void* arg) {
    while (1) {
        // 模拟 100ms 产生一个电能脉冲 (来自硬件中断)
        edge_os_sleep(100);
        uv_async_send(&g_meter.pulse_async);
    }
}

/* --- 4. 定时显示数据 --- */
void display_timer_cb(uv_timer_t* handle) {
    edge_os_log("\n--- METER STATUS ---\n");
    edge_os_log("Energy: %.4f kWh\n", g_meter.total_energy_pulse / 1000.0);
    edge_os_log("--------------------\n");
}

int main(void) {
    static uv_loop_t loop;
    static uv_timer_t disp_timer;

    edge_os_log("--- EdgeNode Smart Meter Simulator ---\n");

    uv_loop_init(&loop);

    /* 初始化电能脉冲处理器 (Async 是电表异步化的关键) */
    uv_async_init(&loop, &g_meter.pulse_async, on_energy_pulse_async);

    /* 初始化安全巡检钩子 (Check 保证每轮业务后的数据一致性) */
    uv_check_init(&loop, &g_meter.safety_check);
    uv_check_start(&g_meter.safety_check, on_safety_check);

    /* 启动显示定时器 */
    uv_timer_init(&loop, &disp_timer);
    uv_timer_start(&disp_timer, display_timer_cb, 2000, 2000);

    /* 启动模拟硬件脉冲的后台线程 */
    edge_thread_create("metro_hw", metrology_hw_sim_thread, NULL, 5, 1024);

    uv_run(&loop);
    return 0;
}