/**
 * @file storage_ops.h
 * @brief EdgeOS Storage Subsystem Abstract Interface (Port / VTable)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 * 
 * [AEP-201 / AEP-202] Clean Architecture / Anti-Corruption Layer:
 * Defines pure consumer interfaces for all storage operations.
 * Concrete backends (SQLite, TSDB, Flash, Mock) implement storage_ops_t.
 */

#ifndef EDGE_INFRA_STORAGE_OPS_H
#define EDGE_INFRA_STORAGE_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct uv_loop_s uv_loop_t;

#include "contract_data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct storage_driver_s storage_driver_t;

/**
 * @brief 通用存储配置
 */
typedef struct {
    char db_path[256];
    uint32_t flush_interval_ms;
    uint32_t batch_size;
    bool enable_wal;
    bool enable_snf;
    uint32_t max_snf_records;
    uint32_t max_db_size_mb;
    void *extra_opts;
} storage_config_t;

/**
 * @brief 查询结果行回调
 * @param user_data 上下文指针
 * @param msg_ptr 遥测数据行指针
 * @return 0 继续遍历，非0 中止遍历
 */
typedef int (*storage_query_cb)(void *user_data, const void *msg_ptr);

/**
 * @brief SQL/原生命令执行回调
 */
typedef int (*storage_exec_cb)(void *user_data, int col_count, char **col_values, char **col_names);

/**
 * @brief 存储引擎抽象虚表 (Storage Operations VTable)
 */
typedef struct storage_ops_s {
    /* 1. 生命周期 (Lifecycle) */
    int (*init)(storage_driver_t *drv, uv_loop_t *loop, const storage_config_t *cfg);
    void (*destroy)(storage_driver_t *drv);

    /* 2. 测点数据 CRUD (Telemetry Data Ops) */
    int (*insert)(storage_driver_t *drv, const void *msg_ptr);
    int (*query)(storage_driver_t *drv, uint64_t start_ts, uint64_t end_ts, storage_query_cb cb, void *user_data);

    /* 3. 事务控制 (Transactions) */
    int (*begin_trans)(storage_driver_t *drv);
    int (*commit_trans)(storage_driver_t *drv);
    int (*rollback_trans)(storage_driver_t *drv);

    /* 4. 原生查询与 DDL (Raw Execution) */
    int (*exec)(storage_driver_t *drv, const char *stmt, storage_exec_cb cb, void *user_data);

    /* 5. WAL 预写日志与压实管理 (WAL & Compaction - notes 3) */
    int (*checkpoint)(storage_driver_t *drv, int mode);
    int (*flush)(storage_driver_t *drv);
    int (*compact)(storage_driver_t *drv);
    int (*get_wal_size)(storage_driver_t *drv, uint64_t *out_bytes);

    /* 6. 分区生命周期与过期清理 (Partition & Retention GC - notes 3) */
    int (*partition_gc)(storage_driver_t *drv, uint64_t cutoff_ts);
    /**
     * @brief 基于容量/条数预算清理最旧历史数据并压实
     * @param max_size_mb 目标容量上限(MB)或保留阈值。在当前行数模型下，>0 表示触发最旧半数记录淘汰；在字节模型下按容量阈值淘汰。
     */
    int (*purge_old_data)(storage_driver_t *drv, uint32_t max_size_mb);

    /* 7. 断线重发与韧性缓存 (Store-and-Forward / SnF - notes 3) */
    int (*snf_enqueue)(storage_driver_t *drv, const void *msg_ptr);
    int (*snf_dequeue)(storage_driver_t *drv, void *out_msg_ptr);
    int (*snf_get_pending_count)(storage_driver_t *drv, uint32_t *out_count);
    int (*snf_flush)(storage_driver_t *drv);

    /* 8. 参数键值存取 (Key-Value Params) */
    int (*set_param)(storage_driver_t *drv, const char *product, uint32_t tag_id, const void *val, size_t val_len);
    int (*get_param)(storage_driver_t *drv, const char *product, uint32_t tag_id, void *buf, size_t buf_len);

    /* 9. 应用注册表管理 (App Registry CRUD) */
    int (*save_app)(storage_driver_t *drv, const void *manifest_ptr);
    int (*get_app)(storage_driver_t *drv, const char *app_id, void *out_manifest_ptr);
    int (*delete_app)(storage_driver_t *drv, const char *app_id);
    int (*list_apps)(storage_driver_t *drv, void *manifests_array_ptr, int max_count);

    /* 10. 原生句柄逃逸门 (Native Handle) */
    void *(*get_native_handle)(storage_driver_t *drv);
} storage_ops_t;

/**
 * @brief 存储驱动实例外壳 (Port Driver Container)
 */
struct storage_driver_s {
    const storage_ops_t *ops;  /**< 绑定的操作虚表 */
    void *priv_ctx;            /**< 后端私有上下文 (如 sqlite_svc_t, rocksdb_ctx_t 等) */
    char name[32];             /**< 驱动标识名称 (如 "sqlite-wal", "tsdb-snf") */
};

#ifdef __cplusplus
}
#endif

#endif /* EDGE_INFRA_STORAGE_OPS_H */
