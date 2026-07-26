#include "edgeos.h"

/**
 * @file moving_average.c
 * @brief EdgeOS WASM 滑动平均与越限预警算法示例
 */

static double s_history[4] = {0.0, 0.0, 0.0, 0.0};
static int s_count = 0;

__attribute__((export_name("on_event"))) void on_event() {
    double current_val = 0.0;

    // 读取输入点位 Tag 1001
    if (ext_datahub_read(1001, &current_val) == 0) {
        // 更新 4 点滑动窗口
        s_history[s_count % 4] = current_val;
        s_count++;

        double sum = 0.0;
        int num = s_count < 4 ? s_count : 4;
        for (int i = 0; i < num; i++) {
            sum += s_history[i];
        }
        double avg = sum / num;

        ext_log(LOG_INFO, "[WASM-AVG] Calculated moving average successfully.");

        // 将平滑后的均值输出到 Tag 9001
        ext_datahub_publish(9001, avg);
    } else {
        ext_log(LOG_ERROR, "[WASM-AVG] Failed to read Tag 1001.");
    }
}
