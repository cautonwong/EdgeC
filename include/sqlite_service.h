#ifndef SQLITE_SERVICE_H
#define SQLITE_SERVICE_H

#include <stdint.h>
#include <uv.h>

#include "shmring.h"  // For PipeMsg

// 存储策略定义
typedef enum {
    STRATEGY_LITE = 1,        // 覆盖写，省空间
    STRATEGY_SERIES = 2,      // 时序存储，异步批量
    STRATEGY_SHARDING = 3,    // 自动分表
    STRATEGY_TRANSACTION = 4  // 强一致性事务
} storage_strategy_t;

// 存储服务配置
typedef struct {
    storage_strategy_t strategy;
    const char *db_path;
    int cache_size_kb;
    int batch_size;
    int flush_interval_ms;
} sqlite_config_t;

// --- API ---

// 创建并初始化服务 (包含自动迁移、Pragma 调优)
sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg);

// 插入数据 (内部根据产品类型自动路由到 Sync 或 Async 通道)
int sqlite_svc_insert(sqlite_svc_t *svc, const PipeMsg *msg);

// 查询回调定义
typedef int (*sqlite_svc_query_cb)(void *user_data, PipeMsg *msg);

/**
 * @brief 查询历史数据
 * @param start_ts 起始时间戳 (ms)
 * @param end_ts 结束时间戳 (ms)
 * @param cb 每行数据的回调
 * @param user_data 回调透传参数
 */
int sqlite_svc_query(sqlite_svc_t *svc, uint64_t start_ts, uint64_t end_ts, sqlite_svc_query_cb cb,
                     void *user_data);

// 事务控制 (EVSE 专用)
int sqlite_svc_begin_trans(sqlite_svc_t *svc);
int sqlite_svc_commit_trans(sqlite_svc_t *svc);

// 销毁
void sqlite_svc_destroy(sqlite_svc_t *svc);

#endif
