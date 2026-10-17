#ifndef EDGE_DATAHUB_H
#define EDGE_DATAHUB_H

#include <stdatomic.h>
#include <uv.h>

#include "contract_data_types.h"
#include "shmring.h"
#include "datahub_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 测点映射回调 (支持实例寻址)
 */
typedef int (*datahub_tag_map_cb_v2)(uint32_t tag_id, uint32_t device_id);

/**
 * @brief 测点映射回调
 */
typedef int (*datahub_tag_map_cb)(uint32_t tag_id);

/**
 * @brief 唤醒过滤回调 (Phase 3 优化)
 */
typedef bool (*datahub_wakeup_filter_cb)(int index);

typedef struct {
    uint32_t tag_id;
    int fd;
    void *ptr;
    size_t len;
} datahub_ref_t;

typedef struct {
    uint32_t chunk_id;
    int fd;
    pipe_msg_t *ptr;
    size_t size;
    uint32_t capacity;
} datahub_chunk_t;

typedef struct {
    uint32_t version;
    uint32_t chunk_count;
    datahub_chunk_t chunks[16];
    uint32_t *tag_to_index_map;
    uint32_t tag_count;
} schema_version_t;

#define DATAHUB_MAX_SUBSCRIBERS 64
#define EDGE_DEVICE_ANY 0xFFFFFFFF

/**
 * @brief 细粒度数据更新回调 (Zero-Copy)
 */
typedef void (*datahub_event_cb)(datahub_t *hub, const pipe_msg_t *msg, void *user_ctx);

/**
 * @brief 订阅过滤条件
 */
typedef struct {
    uint32_t tag_id;            /**< 精确测点 ID (0 表示使用掩码或范围) */
    uint32_t tag_mask;          /**< 测点位掩码过滤 (如 0xFFFF0000) */
    uint32_t domain_mask;       /**< 域掩码 (如 DOMAIN_TELEMETRY | DOMAIN_ALARM) */
    uint32_t device_id;         /**< 目标设备 ID (EDGE_DEVICE_ANY = 0xFFFFFFFF 表示通配) */
    bool notify_on_change_only; /**< 仅在值发生物理改变时触发 (Deadband / RBE) */
    double deadband;            /**< 死区绝对变化阈值 */
} datahub_sub_filter_t;

typedef struct {
    int sub_id;
    bool active;
    datahub_sub_filter_t filter;
    datahub_event_cb callback;
    void *user_ctx;
    pipe_msg_t last_notified_msg;
    bool has_last_msg;
} datahub_subscriber_entry_t;

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
    atomic_uint *shadow_seq;  // [新增] 用于实现每槽位的 Sequence Lock
    int shadow_fd;
    size_t shadow_size;
    uint32_t shadow_capacity;

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

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 uint32_t shadow_capacity, const edge_tag_meta_t *metadata, uint32_t metadata_len,
                 struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                 datahub_wakeup_filter_cb wakeup_fn);

/**
 * @brief [ADR-0005] 初始化 DataHub (配置驱动)
 */
int datahub_init_with_config(datahub_t *hub, uv_loop_t *loop, const datahub_config_t *config,
                             const edge_tag_meta_t *metadata, uint32_t metadata_len,
                             struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                             datahub_wakeup_filter_cb wakeup_fn);

/**
 * @brief [Nexus] 从契约自动初始化 DataHub
 * @param hub DataHub 实例
 * @param loop libuv 事件循环
 * @param contract 契约句柄
 * @param io_pool IO 内存池引用
 */
int datahub_init_from_contract(datahub_t *hub, uv_loop_t *loop, const edge_contract_t *contract,
                               struct net_buf_pool *io_pool);

int datahub_init_from_fd(datahub_t *hub, uv_loop_t *loop, int ring_fd, size_t ring_size,
                         int shadow_fd, size_t shadow_size, uint32_t compact_capacity,
                         const edge_tag_meta_t *metadata, uint32_t metadata_len,
                         struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                         datahub_wakeup_filter_cb wakeup_fn);

void datahub_destroy(datahub_t *hub);
int datahub_publish(datahub_t *hub, const pipe_msg_t *msg);
int datahub_write_shadow(datahub_t *hub, const pipe_msg_t *msg);
int datahub_write_shadow_idx(datahub_t *hub, int idx, const pipe_msg_t *msg);
int datahub_register_ref(datahub_t *hub, uint32_t tag_id, int fd, size_t len);
int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out);
int datahub_read_shadow_idx(datahub_t *hub, uint32_t idx, pipe_msg_t *out);
void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data);

/**
 * @brief [ARCH-DEEP-01] 订阅指定测点精确更新 (Zero-Copy Callback)
 * @return 订阅句柄 sub_id (>= 0)，失败返回负数错误码
 */
int datahub_subscribe_tag(datahub_t *hub, uint32_t tag_id, datahub_event_cb cb, void *user_ctx);

/**
 * @brief [ARCH-DEEP-01] 订阅指定数据域更新
 */
int datahub_subscribe_domain(datahub_t *hub, uint32_t domain_mask, datahub_event_cb cb, void *user_ctx);

/**
 * @brief [ARCH-DEEP-01] 细粒度组合条件订阅 (过滤、死区、变位上送)
 */
int datahub_subscribe_filtered(datahub_t *hub, const datahub_sub_filter_t *filter,
                               datahub_event_cb cb, void *user_ctx);

/**
 * @brief [ARCH-DEEP-01] 注销细粒度订阅
 */
int datahub_unsubscribe(datahub_t *hub, int sub_id);

/**
 * @brief [ADR-0005] 获取 Backend 统计信息
 */
int datahub_get_stats(datahub_t *hub, datahub_backend_stats_t *stats);

/**
 * @brief [ADR-0001] 启动 DataHub Metrics 定时上报（100ms 高频环）
 */
int datahub_start_metrics(datahub_t *hub);

/**
 * @brief [ADR-0001] 停止 Metrics 上报
 */
void datahub_stop_metrics(datahub_t *hub);

/**
 * @brief [Aetheris] 持久化快照：将影子表存入磁盘
 */
int datahub_save_snapshot(datahub_t *hub, const char *path);

/**
 * @brief [Aetheris] 加载快照：从磁盘恢复影子表状态
 */
int datahub_load_snapshot(datahub_t *hub, const char *path);

/**
 * @brief [WAL] 开启 Write-Ahead Log 持久化
 */
int datahub_enable_wal(datahub_t *hub, const char *path);

/**
 * @brief [WAL] 重放 WAL 日志，恢复影子表状态
 */
int datahub_replay_wal(datahub_t *hub);

/**
 * @brief [Compaction] 开启定时压实：定期 Dump 快照并清空 WAL
 */
int datahub_start_compaction(datahub_t *hub, uint64_t interval_ms, const char *snapshot_path);

/**
 * @brief [Compaction] 手动执行一次快照压实
 */
int datahub_compact_now(datahub_t *hub);

/**
 * @brief [V2 Evolution] 动态分配扩展 Chunk 内存物理切片
 */
int datahub_allocate_chunk(datahub_t *hub, uint32_t additional_capacity, datahub_chunk_t *out_chunk);

/**
 * @brief [V2 Evolution] 执行契约 Schema CAS 无锁热切换
 */
int datahub_switch_schema(datahub_t *hub, schema_version_t *new_schema);

// 监听器管理 (由 StreamEngine 等使用)
int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle);
int datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle);

uint32_t datahub_get_metadata_count(datahub_t *hub);

static inline const edge_tag_meta_t *datahub_get_metadata_by_index(datahub_t *hub, uint32_t index) {
    if (hub && index < hub->metadata_count) return &hub->metadata_table[index];
    return NULL;
}

static inline const edge_tag_meta_t *datahub_get_metadata_by_tag(datahub_t *hub, uint32_t tag_id) {
    if (!hub) return NULL;
    int idx = -1;
    if (hub->map_fn_v2) {
        idx = hub->map_fn_v2(tag_id, 0);
    } else if (hub->map_fn) {
        idx = hub->map_fn(tag_id);
    }
    if (idx < 0) idx = (int)tag_id;
    if (idx < 0) return NULL;
    return datahub_get_metadata_by_index(hub, (uint32_t)idx);
}

/**
 * @brief [KubeEdge DeviceTwin Pattern] 设备影子状态对齐与冲突校验
 * @return 0=状态对齐 (In-Sync), 1=存在偏差 (State Mismatch)
 */
int datahub_devicetwin_reconcile(datahub_t *hub, uint32_t desired_tag_id, uint32_t reported_tag_id, double tolerance);

/**
 * @brief [Eclipse Zenoh-Pico Pattern] 瞬时 Tag 快照 Query-Reply 查询
 * @return 0=成功 (Query Success), -1=Tag不存在 (Tag Not Found)
 */
int datahub_query_snapshot(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg);

/**
 * @brief [DDD 聚合根安全封装] 从 DataHub 环形缓冲区读取下一条消息
 */
int datahub_read_ring(datahub_t *hub, uint64_t *inout_seq, pipe_msg_t *out_msg);

/**
 * @brief [DDD 聚合根安全封装] 向 DataHub 环形缓冲区写入一条消息
 */
int datahub_write_ring(datahub_t *hub, const pipe_msg_t *msg);

/**
 * @brief [DDD 聚合根安全封装] 获取 DataHub 环形缓冲区的文件描述符
 */
int datahub_get_ring_fd(datahub_t *hub);

/**
 * @brief [DDD 聚合根安全封装] 获取 DataHub 环形缓冲区写入总计数
 */
uint64_t datahub_get_ring_total_writes(const datahub_t *hub);

/**
 * @brief [DDD 聚合根安全封装] 获取 DataHub 环形缓冲区当前序列号
 */
uint64_t datahub_get_ring_seq(const datahub_t *hub);

/**
 * @brief [DDD 聚合根安全封装] 绑定网络/IO 缓冲池到 DataHub
 */
int datahub_bind_io_pool(datahub_t *hub, struct net_buf_pool *pool);

/**
 * @brief [DDD 聚合根安全封装] 获取 DataHub 绑定的 IO 缓冲池引用
 */
struct net_buf_pool *datahub_get_io_pool(const datahub_t *hub);

/**
 * @brief [DDD 聚合根安全封装] 获取当前注册的监听器数量
 */
uint32_t datahub_get_listener_count(const datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif
