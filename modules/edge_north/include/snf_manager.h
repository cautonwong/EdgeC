/**
 * @file snf_manager.h
 * @brief Store-and-Forward (SnF) 管理器接口
 */

#ifndef SNF_MANAGER_H
#define SNF_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
typedef struct uv_loop_s uv_loop_t;

#include "pipe_msg.h"

typedef struct snf_manager_s snf_manager_t;

// Aligned storage placeholder for caller allocation
typedef struct {
    alignas(max_align_t) uint8_t raw[4096];
} snf_manager_storage_t;

/**
 * @brief SnF 事件回调 (用于上层同步数据)
 */
typedef int (*snf_publish_cb)(void *user_data, const PipeMsg *msg);

/**
 * @brief 在途消息数回调。
 *
 * `publish_fn` 返回 0 只代表**入队**，不代表对端收到。SnF 用这个回调判定整批是否
 * 真的送达：返回 0 才把记录标成 synced。为 NULL 时按"无在途"处理。
 *
 * 语义是 at-least-once：没确认的批次下一轮会重发，允许重复，不允许丢。
 */
typedef uint32_t (*snf_pending_cb)(void *user_data);

/** 单批最多确认多少条（受 snf_manager_storage_t 的定长缓冲约束） */
#define SNF_MAX_BATCH 64

typedef struct {
    uint64_t ts;
    uint32_t tag_id;
    double value;
    uint16_t quality;
} snf_record_t;

/**
 * @brief SnF 持久化端口。由 product/bootstrap 注入，edge_north 不直接依赖 SQLite。
 */
typedef struct snf_store_port_s {
    void *ctx;
    int (*ensure_schema)(void *ctx);
    int (*count_unsynced)(void *ctx, int *out_count);
    int (*read_unsynced)(void *ctx, uint32_t limit, snf_record_t *out, uint32_t cap,
                         uint32_t *out_count);
    int (*mark_synced)(void *ctx, const snf_record_t *records, uint32_t count);
    int (*insert)(void *ctx, const PipeMsg *msg);
} snf_store_port_t;

typedef struct {
    const snf_store_port_t *store;
    uv_loop_t *loop;
    snf_publish_cb publish_fn;
    snf_pending_cb pending_fn; /**< 可空 */
    void *user_data;
    uint32_t sync_batch_size;
    uint32_t sync_interval_ms;
} snf_config_t;

/**
 * @brief 创建 SnF 管理器 (基于调用方分配存储)
 */
snf_manager_t *snf_manager_create_in(snf_manager_storage_t *storage, const snf_config_t *cfg);

/**
 * @brief 创建 SnF 管理器
 */
snf_manager_t *snf_manager_create(const snf_config_t *cfg);

/**
 * @brief 更新连接状态 (触发或暂停转发)
 */
void snf_manager_set_online(snf_manager_t *mgr, bool online);

/**
 * @brief 处理离线缓存 (当 MQTT 断开时，直接写入 DB)
 */
int snf_manager_buffer_data(snf_manager_t *mgr, const PipeMsg *msg);

/**
 * @brief 销毁管理器
 */
void snf_manager_destroy(snf_manager_t *mgr);

#endif // SNF_MANAGER_H
