/**
 * 分时电价调度策略 (Time-of-Use Dispatch)
 *
 * 逻辑：
 * - 00:00 - 06:00: 谷时 (Low price) -> 充电 (Charge @ 2000W)
 * - 10:00 - 14:00: 峰时 (High price) -> 放电 (Discharge @ 3000W)
 * - 18:00 - 22:00: 峰时 (High price) -> 放电 (Discharge @ 3000W)
 * - 其他时间: 待机 (Standby)
 */

function getTargetPower() {
    const now = new Date();
    const hour = now.getHours();

    if (hour >= 0 && hour < 6) {
        return 2000.0; // Charge
    } else if ((hour >= 10 && hour < 14) || (hour >= 18 && hour < 22)) {
        return -3000.0; // Discharge
    }

    return 0.0; // Standby
}

// 在脚本末尾直接返回结果对象，避免使用全局 const 冲突
({
    target_pwr: getTargetPower(),
    mode: "TOU_AUTO",
    timestamp: Date.now()
});
