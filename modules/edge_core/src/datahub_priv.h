#ifndef DATAHUB_PRIV_H
#define DATAHUB_PRIV_H

#include "datahub.h"
#include "datahub_shm.h"

struct datahub_s {
    datahub_backend_type_t backend_type;
    const datahub_ops_t *ops;  /**< Active VTable implementation */
    void *backend_ctx;         /**< Backend-specific private context pointer */

    uv_loop_t *loop;
    shm_ring_t ring;
    struct net_buf_pool *io_pool;  // [Phase 4] 注入的 IO 内存池引用

    const edge_tag_meta_t *metadata_table;
    uint32_t metadata_count;

    pipe_msg_t *shadow_table;
    uint64_t *shadow_seq;
    datahub_shm_header_t *shadow_header;
    int shadow_fd;
    size_t shadow_size;
    uint32_t shadow_capacity;
    bool shadow_owner;
    char shadow_shm_name[256];

    // --- [Dynamic Chunking & Hot Schema] ---
    atomic_uintptr_t active_schema; /**< 原子动态契约指针 (schema_version_t*) */
    datahub_chunk_t chunks[16];      /**< 动态分块内存映射表 */
    uint32_t chunk_count;

    datahub_tag_map_cb map_fn;
    datahub_tag_map_cb_v2 map_fn_v2; // [Nexus-Pro] 支持实例感知的寻址
    datahub_wakeup_filter_cb wakeup_fn;  // [注入] 唤醒过滤器

    // --- [Batch Wakeup] 优化 ---
    uv_async_t *listeners[16];
    atomic_int listener_count;
    uv_check_t batch_check;     /**< [新增] 用于在本轮 Loop 结束前合并通知 */
    atomic_bool batch_pending;  /**< [新增] 标记是否有待处理的批次 */

    datahub_ref_t refs[32];  // [新增] 引用注册表，支持32个并发大块数据流
    atomic_int ref_count;    // [优化] 改为原子类型

    datahub_on_data_cb legacy_cb;
    void *legacy_user_data;
    uv_async_t legacy_async;

    uv_mutex_t shadow_lock;

    // --- [WAL] Write-Ahead Log ---
    int wal_fd;
    char wal_path[256];
    
    // --- [Compaction] Snapshot Compaction ---
    uv_timer_t compaction_timer;
    char snapshot_path[256];
    int compaction_active;

    // --- [ADR-0001] Metrics ---
    atomic_uint_fast64_t metrics_shadow_writes;
    uv_timer_t metrics_timer;
    int metrics_active;

    // --- [ARCH-DEEP-01] Fine-Grained Subscription Engine ---
    datahub_subscriber_entry_t subscribers[DATAHUB_MAX_SUBSCRIBERS];
    uv_mutex_t sub_lock;
    int next_sub_id;

    // --- [ADR-0005] Backend Stats Counters ---
    atomic_uint_fast64_t total_publishes;
    atomic_uint_fast64_t total_shadow_reads;
    atomic_uint_fast64_t network_packets_tx;
    atomic_uint_fast64_t network_packets_rx;
    atomic_uint_fast64_t backend_drops;
};

#endif /* DATAHUB_PRIV_H */
