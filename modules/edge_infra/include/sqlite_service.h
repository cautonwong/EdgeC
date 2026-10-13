#ifndef SQLITE_SERVICE_H
#define SQLITE_SERVICE_H

#include <stdint.h>
typedef struct uv_loop_s uv_loop_t;

#include "shmring.h"  // For PipeMsg
#include "contract_data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// 存储策略定义
typedef enum {
    STRATEGY_LITE = 1,         // 覆盖写，省空间
    STRATEGY_SERIES = 2,       // 时序存储，异步批量
    STRATEGY_SHARDING = 3,     // 自动分表
    STRATEGY_TRANSACTION = 4,  // 强一致性事务
} storage_strategy_t;

// 存储服务上下文 (不透明句柄)
typedef struct sqlite_svc_s sqlite_svc_t;

// 错误回调
typedef void (*sqlite_err_cb)(int code, const char *msg);

// 存储服务配置
typedef struct {
    storage_strategy_t strategy;
    const char *db_path;
    int batch_size;
    int flush_interval_ms;
    int cache_size_kb;
    uint32_t max_db_size_mb; /**< [Phase 9] 数据库最大容量 (MB)，超过则触发自动清理 */
} sqlite_config_t;

// --- API ---

// 创建并初始化服务 (包含自动迁移、Pragma 调优)
sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg);

/**
 * @brief [Nexus] 从契约初始化数据库 (自动建表、注入初始配置)
 * @param svc 数据库服务实例
 * @param contract 契约句柄
 */
int sqlite_svc_init_from_contract(sqlite_svc_t *svc, const edge_contract_t *contract);

// 插入数据 (内部根据策略自动路由)
int sqlite_svc_insert(sqlite_svc_t *svc, const PipeMsg *msg);

// 同步刷盘（将内存批处理缓冲区立即写入 SQLite 数据库）
int sqlite_svc_flush_sync(sqlite_svc_t *svc);

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

// 同步冲刷内存缓冲至数据库
int sqlite_svc_flush(sqlite_svc_t *svc);

// 事务控制 (事务策略专用)
int sqlite_svc_begin_trans(sqlite_svc_t *svc);
int sqlite_svc_commit_trans(sqlite_svc_t *svc);

// 通用 SQL 执行回调 (用于非时序数据，如配置参数)
typedef void (*sqlite_exec_cb)(void *user_data, int status, char **errmsg);

/**
 * @brief 异步执行通用 SQL (DDL/DML)
 * @param sql SQL 语句
 * @param cb 执行完成回调
 */
int sqlite_svc_exec(sqlite_svc_t *svc, const char *sql, sqlite_exec_cb cb, void *user_data);

// 销毁
void sqlite_svc_destroy(sqlite_svc_t *svc);

/**
 * @brief 设置产品/全局参数 (支持 BLOB)
 * @param product 产品名 (如 "IEMS")
 * @param tag_id 契约中定义的参数 TagID
 * @param val 参数值指针 (可以是 double*, int*, string 等)
 * @param len 数据长度
 */
int sqlite_svc_set_param(sqlite_svc_t *svc, const char *product, uint32_t tag_id, const void *val,
                         size_t len);

/**
 * @brief 获取产品/全局参数
 * @param buf 接收缓冲区
 * @param buflen 缓冲区大小
 * @return 实际读取的字节数，失败返回 -1
 */
int sqlite_svc_get_param(sqlite_svc_t *svc, const char *product, uint32_t tag_id, void *buf,
                         size_t buflen);

/**
 * @brief 获取原始 SQLite 句柄 (用于高级配置加载)
 */
void *sqlite_svc_get_db_handle(sqlite_svc_t *svc);

// --- App Registry CRUD ---
struct app_manifest_s;
typedef struct app_manifest_s app_manifest_t_opaque;

int sqlite_svc_save_app(sqlite_svc_t *svc, const void *manifest_ptr);
int sqlite_svc_get_app(sqlite_svc_t *svc, const char *app_id, void *out_manifest_ptr);
int sqlite_svc_delete_app(sqlite_svc_t *svc, const char *app_id);
int sqlite_svc_list_apps(sqlite_svc_t *svc, void *manifests_array_ptr, int max_count);

#ifdef __cplusplus
}
#endif

#endif  // SQLITE_SERVICE_H
