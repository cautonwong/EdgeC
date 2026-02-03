#include "edgeos.h"

/**
 * @brief 简单的 WASM 算法示例：滑动平均 (Moving Average)
 *
 * 此算子会读取一个输入点位，进行简单的逻辑判断，然后发布结果。
 */

__attribute__((export_name("on_event"))) void on_event() {
    double val = 0;
    // 假设我们关心 1001 号点位 (VOLTAGE_PHASE_A)
    if (ext_datahub_read(1001, &val) == 0) {
        ext_log(LOG_INFO, "WASM: Read Tag 1001 successfully");

        // 逻辑：如果电压超过 240V，发布一个警告值到 9999 号点位
        if (val > 240.0) {
            ext_log(LOG_WARN, "WASM: High voltage detected!");
            ext_datahub_publish(9999, 1.0);
        } else {
            ext_datahub_publish(9999, 0.0);
        }
    } else {
        ext_log(LOG_ERROR, "WASM: Failed to read Tag 1001");
    }
}
