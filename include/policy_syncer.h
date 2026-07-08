#ifndef POLICY_SYNCER_H
#define POLICY_SYNCER_H

#include "bus_orchestrator.h"
#include "sqlite_service.h"

// 💡 [Consumer-Driven] 策略 IO 抽象：允许注入 MQTT/HTTP 客户端
typedef struct {
    void* ctx;
    int (*publish)(void* ctx, const char* topic, const uint8_t* data, size_t len);
    int (*request)(void* ctx, const char* url, const char* method, const uint8_t* body, size_t body_len);
} policy_io_ops_t;

/**
 * @brief 从数据库加载采集策略并注入编排器
 * 符合 v3.1 架构中 "SQLite 驱动调度" 的原则
 */
int policy_syncer_run(sqlite_svc_t* db, bus_orchestrator_t* orch);

/**
 * @brief [Aetheris] 动态应用北向配置增量 (JSON/YAML)
 */
int policy_syncer_apply_delta(bus_orchestrator_t* orch, const char* payload, size_t len);

/**
 * @brief [Consumer-Driven] 设置策略 IO 接口（替代直接依赖 MQTT/HTTP）
 */
void policy_syncer_set_io(policy_io_ops_t ops);

#endif