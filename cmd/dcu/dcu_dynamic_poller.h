#ifndef DCU_DYNAMIC_POLLER_H
#define DCU_DYNAMIC_POLLER_H

#include <uv.h>

#include "bus_orchestrator.h"
#include "sqlite_service.h"

typedef struct dcu_dynamic_poller_s dcu_dynamic_poller_t;

/**
 * @brief 创建 DCU 动态抄表管理器
 */
dcu_dynamic_poller_t* dcu_dynamic_poller_create(uv_loop_t* loop, bus_orchestrator_t* orch,
                                                sqlite_svc_t* db);

/**
 * @brief 启动动态抄表逻辑
 * 从 SQLite 加载表计列表并开始按周期轮询
 */
int dcu_dynamic_poller_start(dcu_dynamic_poller_t* poller);

/**
 * @brief 强制重新加载表计列表 (由数据库外部变更触发)
 */
int dcu_dynamic_poller_reload(dcu_dynamic_poller_t* poller);

void dcu_dynamic_poller_destroy(dcu_dynamic_poller_t* poller);

#endif
