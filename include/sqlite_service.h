#ifndef SQLITE_SERVICE_H
#define SQLITE_SERVICE_H

#include <stdint.h>
#include <uv.h>

#include "shmring.h"  // For PipeMsg

// 五大产品类型定义 (从文档对齐)
typedef enum {
    PROD_HEMS = 1,       // 家庭: 轻量, 覆盖写
    PROD_IEMS = 2,       // 工业: 高并发, 异步批量
    PROD_SG_EL2025 = 3,  // 融合: 混合, BLOB支持
    PROD_EVSE = 4,       // 充电: 强一致性, 事务
    PROD_DCU = 5         // 集中: 海量数据, 分表
} product_type_t;

// 存储服务上下文 (不透明句柄)
typedef struct sqlite_svc_s sqlite_svc_t;

// 错误回调
typedef void (*sqlite_err_cb)(int code, const char *msg);

// 配置参数
typedef struct {
    product_type_t type;
    const char *db_path;
    int cache_size_kb;      // Page Cache 大小
    int batch_size;         // 异步批量提交阈值 (仅 IEMS/SG/DCU 有效)
    int flush_interval_ms;  // 强制刷盘间隔
} sqlite_config_t;

// --- API ---

// 创建并初始化服务 (包含自动迁移、Pragma 调优)
sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg);

// 插入数据 (内部根据产品类型自动路由到 Sync 或 Async 通道)
int sqlite_svc_insert(sqlite_svc_t *svc, const PipeMsg *msg);

// 事务控制 (EVSE 专用)
int sqlite_svc_begin_trans(sqlite_svc_t *svc);
int sqlite_svc_commit_trans(sqlite_svc_t *svc);

// 销毁
void sqlite_svc_destroy(sqlite_svc_t *svc);

#endif
