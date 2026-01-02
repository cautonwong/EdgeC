#ifndef POLICY_SYNCER_H
#define POLICY_SYNCER_H

#include "sqlite_service.h"
#include "bus_orchestrator.h"

/**
 * @brief 从数据库加载采集策略并注入编排器
 * 符合 v3.1 架构中 "SQLite 驱动调度" 的原则
 */
int policy_syncer_run(sqlite_svc_t *db, bus_orchestrator_t *orch);

#endif