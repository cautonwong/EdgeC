// alarm_policy.js
// 这是一个动态下发的报警策略

// 从 DataHub 读取实时电压 (模拟点位 1001)
const v = DataHub.read(1001); 

// 打印日志 (将显示在 HMI 和 控制台)
// Log.info(`Current Voltage: ${v.toFixed(2)}V`);

if (v > 245) {
    Log.info(`[ALARM] Overvoltage detected (${v.toFixed(2)}V)! Triggering emergency stop.`);
    
    // 通过 Arbiter 下发控制指令 (模拟点位 2001, 设置为 0 即停止)
    Arbiter.control(2001, 0); 
}
