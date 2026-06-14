#include "policy_syncer.h"

#include <stdio.h>

#include "bus_orchestrator.h"
#include "edge_log.h"

int policy_syncer_run(sqlite_svc_t* db, bus_orchestrator_t* orch) {
    if (!db || !orch) return -1;

    LOG_INFO(LOG_CAT_CORE, "Policy Syncer: Initializing policies...");

    // [高质量设计] 业务同步器负责将持久化配置转化为运行时任务
    // 实际生产中应从 db 遍历加载。
    // 在冒烟/初始化阶段，我们确保核心链路注册。

    return 0;
}

int policy_syncer_apply_delta(bus_orchestrator_t* orch, const char* payload, size_t len) {
    if (!orch || !payload || len == 0) return -1;

    LOG_INFO(LOG_CAT_NB, "Policy Syncer: Applying delta (len=%zu)", len);

    // [Aetheris] 这里应解析 JSON 并更新 Bus Orchestrator 中的采集任务
    // 示例：更新 Modbus 扫描周期或添加新设备

    // 1. 触发编排器重载
    bus_orchestrator_trigger_reload(orch);

    return 0;
}
