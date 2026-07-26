#include "edgeos.h"

/**
 * @file voltage_guard.c
 * @brief EdgeOS WASM 智能越限守护算法示例
 * 
 * 物理契约:
 * 1. 订阅输入 Tag 1001 (VOLTAGE_PHASE_A)
 * 2. 判定逻辑: 若 A 相电压 > 240.0V，触发高压越限告警 (Tag 9999 输出 1.0)
 * 3. 正常逻辑: 若 A 相电压 <= 240.0V，取消告警 (Tag 9999 输出 0.0)
 */

__attribute__((export_name("on_event"))) void on_event() {
    double voltage = 0.0;
    
    // 读 DataHub 1001 点位
    if (ext_datahub_read(1001, &voltage) == 0) {
        if (voltage > 240.0) {
            ext_log(LOG_WARN, "[WASM-GUARD] Overvoltage detected on Tag 1001!");
            ext_datahub_publish(9999, 1.0);
        } else {
            ext_log(LOG_INFO, "[WASM-GUARD] Voltage normal on Tag 1001.");
            ext_datahub_publish(9999, 0.0);
        }
    } else {
        ext_log(LOG_ERROR, "[WASM-GUARD] Failed to read Tag 1001!");
    }
}
