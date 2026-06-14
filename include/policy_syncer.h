#ifndef POLICY_SYNCER_H
#define POLICY_SYNCER_H

#include "bus_orchestrator.h"
#include "sqlite_service.h"

/**
 * @brief 从数据库加载采集策略并注入编排器
 * 符合 v3.1 架构中 "SQLite 驱动调度" 的原则
 */
int policy_syncer_run(sqlite_svc_t* db, bus_orchestrator_t* orch);

/**
 * @brief [Aetheris] 动态应用北向配置增量 (JSON/YAML)
 */
int policy_syncer_apply_delta(bus_orchestrator_t* orch, const char* payload, size_t len);

#endif