/**
 * @file edge_telemetry_store.h
 * @brief Unified Telemetry Storage, WAL Crash-Consistency & SnF Repository (ARCH-DEEP-03)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#ifndef EDGE_TELEMETRY_STORE_H
#define EDGE_TELEMETRY_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "contract_data_types.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct edge_telemetry_store_s edge_telemetry_store_t;

/**
 * @brief 北向数据转发回调函数 (用于 SnF 在线实时/回放转发)
 * @param msgs 待发送的测点消息数组
 * @param count 消息条数
 * @param user_ctx 用户上下文
 * @return 0 成功，非 0 失败
 */
typedef int (*telemetry_forward_fn)(const pipe_msg_t *msgs, size_t count, void *user_ctx);

/**
 * @brief 时序仓储配置
 */
typedef struct {
    const char *db_path;             /**< 主数据库/持久化文件路径 (可为 NULL) */
    const char *wal_path;            /**< 二进制 WAL 路径 (NULL 则在 db_path.wal 同目录) */
    uint32_t flush_interval_ms;      /**< 异步批量刷盘周期 (默认 1000ms) */
    uint32_t batch_size;             /**< 批量刷盘最大条数 (默认 500) */
    uint32_t max_retention_days;     /**< 历史数据最大保存天数 */
    uint32_t max_db_size_mb;         /**< 数据库最大磁盘配额 (MB) */
    bool enable_gorilla_compression; /**< 是否启用 Gorilla 时序压缩 */

    // --- Store-and-Forward (SnF) 选项 ---
    telemetry_forward_fn forward_cb; /**< 北向转发回调 */
    void *forward_ctx;
    size_t snf_max_offline_records;  /**< 离线缓存最大记录数 (默认 10000) */
} edge_telemetry_store_cfg_t;

/**
 * @brief 仓储运行状态与统计指标
 */
typedef struct {
    uint64_t total_appended;         /**< 总追加记录数 */
    uint64_t total_wal_flushed;      /**< 已刷盘 WAL 记录数 */
    uint64_t total_snf_forwarded;    /**< 已转发北向记录数 */
    uint64_t current_snf_pending;    /**< 当前离线暂存记录数 */
    uint64_t wal_file_size_bytes;    /**< WAL 文件字节大小 */
    uint64_t compression_bytes_in;   /**< 压缩前原始字节 */
    uint64_t compression_bytes_out;  /**< 压缩后压缩字节 */
    bool is_online;                  /**< 当前北向链路在线状态 */
} edge_telemetry_store_stats_t;

/**
 * @brief 创建并启动统一时序仓储
 * @param loop Libuv 事件循环句柄 (可为 NULL，在无循环单测模式下同步运行)
 * @param cfg 仓储配置
 * @return 仓储实例指针，失败返回 NULL
 */
edge_telemetry_store_t *edge_telemetry_store_create(uv_loop_t *loop,
                                                    const edge_telemetry_store_cfg_t *cfg);

/**
 * @brief 追加单条遥测数据 (非阻塞，快速路径)
 */
int edge_telemetry_store_append(edge_telemetry_store_t *store, const pipe_msg_t *msg);

/**
 * @brief 追加批量遥测数据
 */
int edge_telemetry_store_append_batch(edge_telemetry_store_t *store,
                                      const pipe_msg_t *msgs, size_t count);

/**
 * @brief 强制同步刷盘 (用于关机/断电保护)
 */
int edge_telemetry_store_flush_sync(edge_telemetry_store_t *store);

/**
 * @brief 查询指定 Tag 历史时序数据
 */
typedef int (*telemetry_query_cb)(void *user_ctx, const pipe_msg_t *msg);
int edge_telemetry_store_query(edge_telemetry_store_t *store, uint32_t tag_id,
                               uint64_t start_ts, uint64_t end_ts,
                               telemetry_query_cb cb, void *user_ctx);

/**
 * @brief 更新网络在线状态 (触发 SnF 队列自动回放)
 */
void edge_telemetry_store_set_online(edge_telemetry_store_t *store, bool is_online);

/**
 * @brief 获取仓储运行统计指标
 */
int edge_telemetry_store_get_stats(edge_telemetry_store_t *store, edge_telemetry_store_stats_t *stats);

/**
 * @brief 销毁仓储并安全落盘
 */
void edge_telemetry_store_destroy(edge_telemetry_store_t *store);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_TELEMETRY_STORE_H */
