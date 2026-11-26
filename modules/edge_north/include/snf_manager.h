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
 * @brief SnF 发布回调 (用于上层同步数据)
 *
 * @param wall_ts_us 记录墙钟，epoch 微秒 (CLOCK_REALTIME)。离线缓冲在 buffer_data
 *                   入库时采样一次；实时直发路径在发送边界采样。北向驱动用它
 *                   作为报文时间戳，不能用 PipeMsg 里的单调 ts。
 */
typedef int (*snf_publish_cb)(void *user_data, const PipeMsg *msg, uint64_t wall_ts_us);

/**
 * @brief 在途消息数回调。
 *
 * `publish_fn` 返回 0 只代表**入队**，不代表对端收到。SnF 用这个回调判定整批是否
 * 真的送达：返回 0 且 failures_fn 也返回 0 才把记录标成 synced。
 *
 * 跨线程契约：传输层必须在递减 inflight **之前**递增失败计数（先 failures++ 再
 * inflight--）；管理器先采样 pending，为 0 再采样 failures。两者都遵守时，确认
 * 判据不会漏掉"已发生但尚未发布"的失败。
 *
 * at-least-once 只在 pending_fn 与 failures_fn 都提供时成立。pending_fn 非空却
 * 缺 failures_fn 属配置错误，snf_manager_create* 返回 NULL。两个都为空表示不跟踪
 * 在途的信任模式，该模式不提供 at-least-once。
 */
typedef uint32_t (*snf_pending_cb)(void *user_data);

/**
 * @brief 取走并清零传输层失败计数。返回 >0 时本轮不确认并重发该批。
 *
 * 与 pending_fn 成对使用。传输层实现必须先递增本计数、再递减 inflight，管理器
 * 才能用"pending 归零后读 failures"得到原子一致的判据。
 */
typedef uint32_t (*snf_failures_cb)(void *user_data);

/** 单批最多确认多少条（受 snf_manager_storage_t 的定长缓冲约束） */
#define SNF_MAX_BATCH 64

typedef struct {
    uint64_t wall_ts_us; /**< 记录墙钟 (epoch µs)；主键之一，也是补发排序键 */
    uint64_t ts;         /**< 单调时间 ns，仅用于排障日志 */
    uint32_t tag_id;
    double value;
    uint16_t quality;
} snf_record_t;

/**
 * @brief SnF 持久化端口。由 product/bootstrap 注入，edge_north 不直接依赖 SQLite。
 *
 * insert 的 wall_ts_us 是buffer_data 在持久化边界采样的墙钟；端口负责以
 * (wall_ts_us, tag_id) 为身份落库，read_unsynced/mark_synced 使用同一身份。
 */
typedef struct snf_store_port_s {
    void *ctx;
    int (*ensure_schema)(void *ctx);
    int (*count_unsynced)(void *ctx, int *out_count);
    int (*read_unsynced)(void *ctx, uint32_t limit, snf_record_t *out, uint32_t cap,
                         uint32_t *out_count);
    int (*mark_synced)(void *ctx, const snf_record_t *records, uint32_t count);
    int (*insert)(void *ctx, const PipeMsg *msg, uint64_t wall_ts_us);
} snf_store_port_t;

typedef struct {
    const snf_store_port_t *store;
    uv_loop_t *loop;
    snf_publish_cb publish_fn;
    snf_pending_cb pending_fn;   /**< 可空；非空时 failures_fn 必须非空 */
    snf_failures_cb failures_fn; /**< 可空；与 pending_fn 成对才提供 at-least-once */
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
 *
 * 在持久化边界采样一次墙钟 (epoch µs, CLOCK_REALTIME) 交给 store；同一微秒的
 * 突发按 +1 钳制，避免 (wall_ts_us, tag_id) 主键互相覆盖。PipeMsg 里的 ts
 * 保持单调时间，仅用于日志/排障。
 */
int snf_manager_buffer_data(snf_manager_t *mgr, const PipeMsg *msg);

/**
 * @brief 销毁管理器
 */
void snf_manager_destroy(snf_manager_t *mgr);

#endif // SNF_MANAGER_H
