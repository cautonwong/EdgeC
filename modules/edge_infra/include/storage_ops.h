/**
 * @file storage_ops.h
 * @brief EdgeOS Storage Subsystem Abstract Interface (Port / VTable)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 * 
 * [AEP-201] Clean Architecture / Anti-Corruption Layer:
 * Defines pure consumer interfaces for all storage operations.
 * Concrete backends (SQLite, TSDB, Flash, Mock) must implement storage_ops_t.
 */

#ifndef EDGE_INFRA_STORAGE_OPS_H
#define EDGE_INFRA_STORAGE_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "contract_data_types.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef PipeMsg pipe_msg_t;

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
    void *extra_opts;
} storage_config_t;

/**
 * @brief 查询结果行回调
 * @param user_data 上下文指针
 * @param msg 解析后的遥测管道消息
 * @return 0 继续遍历，非0 中止遍历
 */
typedef int (*storage_query_cb)(void *user_data, const pipe_msg_t *msg);

/**
 * @brief SQL/原生命令执行回调
 */
typedef int (*storage_exec_cb)(void *user_data, int col_count, char **col_values, char **col_names);

/**
 * @brief 存储引擎抽象虚表 (Storage Operations VTable)
 */
typedef struct storage_ops_s {
    /**
     * @brief 初始化存储驱动
     */
    int (*init)(storage_driver_t *drv, uv_loop_t *loop, const storage_config_t *cfg);

    /**
     * @brief 写入单条测点消息
     */
    int (*insert)(storage_driver_t *drv, const pipe_msg_t *msg);

    /**
     * @brief 按时间区间查询测点历史
     */
    int (*query)(storage_driver_t *drv, uint64_t start_ts, uint64_t end_ts, storage_query_cb cb, void *user_data);

    /**
     * @brief 开启事务
     */
    int (*begin_trans)(storage_driver_t *drv);

    /**
     * @brief 提交事务
     */
    int (*commit_trans)(storage_driver_t *drv);

    /**
     * @brief 回滚事务
     */
    int (*rollback_trans)(storage_driver_t *drv);

    /**
     * @brief 执行原生查询/DDL
     */
    int (*exec)(storage_driver_t *drv, const char *stmt, storage_exec_cb cb, void *user_data);

    /**
     * @brief 持久化参数
     */
    int (*set_param)(storage_driver_t *drv, const char *product, uint32_t tag_id, const void *val, size_t val_len);

    /**
     * @brief 读取持久化参数
     */
    int (*get_param)(storage_driver_t *drv, const char *product, uint32_t tag_id, void *buf, size_t buf_len);

    /**
     * @brief 保存应用注册元数据
     */
    int (*save_app)(storage_driver_t *drv, const void *manifest_ptr);

    /**
     * @brief 获取应用注册元数据
     */
    int (*get_app)(storage_driver_t *drv, const char *app_id, void *out_manifest_ptr);

    /**
     * @brief 删除应用注册
     */
    int (*delete_app)(storage_driver_t *drv, const char *app_id);

    /**
     * @brief 列举所有已注册应用
     */
    int (*list_apps)(storage_driver_t *drv, void *manifests_array_ptr, int max_count);

    /**
     * @brief 获取底层原生句柄 (仅供受控插件调用)
     */
    void *(*get_native_handle)(storage_driver_t *drv);

    /**
     * @brief 销毁存储驱动实例
     */
    void (*destroy)(storage_driver_t *drv);
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
