#ifndef DCU_POLLER_H
#define DCU_POLLER_H

#include <stdbool.h>
#include <stdint.h>

/* read_value 返回此值: 请求已发出、下一轮再收, 既非成功也非失败 */
#define POLL_READ_PENDING 1
#include <uv.h>

#include "dcu_inventory.h"
#include "edge_periodic_clock.h"
#include "queue.h"

// 表级状态的开放寻址表容量取档案上限的 2 倍：载荷因子 ≤ 0.5，探针链不会退化成长链扫描。
// 墓碑只在探针路过的位置回收，容量不足时它们会把表占满。
#define METER_TABLE_CAP (MAX_METERS * 2)

// 前向声明驱动管理器，避免头文件循环依赖
struct driver_manager_s;
typedef struct driver_manager_s driver_manager_t;

#define MAX_TASKS 16
#define MAX_SCHEMES 16
#define MAX_SCHEME_OADS 32
#define MAX_QUEUE_ENTRIES 128
#define DCU_MAX_CHANNELS 32

/**
 * @brief DL/T 698.45 采集任务监控单元 (OI 6035)
 */

typedef struct {
    uint32_t task_id;
    uint8_t status;
    uint32_t total_meters;
    uint32_t success_meters;
    uint32_t total_packets;
} dcu_task_monitor_t;

/**
 * @brief DL/T 698.45 方案类型枚举
 */
typedef enum {
    SCHEME_TYPE_NORMAL = 1,
    SCHEME_TYPE_EVENT = 2,
    SCHEME_TYPE_TRANSPARENT = 3,
    SCHEME_TYPE_REPORT = 4,
    SCHEME_TYPE_SCRIPT = 5
} dcu_scheme_type_t;


typedef struct {
    uint32_t scheme_id;
    dcu_scheme_type_t type;
    union {
        struct {
            uint8_t capture_mode;      /* 6014 采集类型；只有 0 可执行 */
            uint8_t ms_type;           /* 1 = 全部用户地址 */
            uint16_t storage_depth;    /* 0 = 只采集当前数据 */
            uint8_t storage_time;      /* 0 = 未定义时标 */
            char ms_addrs[8][MAX_ADDR_LEN];
            uint8_t ms_count;
            uint32_t csd_list[MAX_SCHEME_OADS]; /* DataHub tag，不是标准 OAD */
            uint16_t csd_count;
        } normal;
    } data;
} dcu_scheme_t;

/* 6014 A2 单元（标准 A.7）。CSD 保留线上 OAD；store 在落库前翻成 DataHub tag。 */
typedef struct {
    uint8_t scheme_id;
    uint16_t storage_depth;
    uint8_t capture_type;
    uint8_t storage_time;
    uint8_t ms_choice;
    bool has_road;
    uint8_t csd_count;
    uint32_t csd_oads[MAX_SCHEME_OADS];
} dcu_scheme_cfg_t;

/* 6014 方法的解码结果。整帧先完整解码（DAR 7），再做字段策略（DAR 2/3/6）。 */
typedef struct {
    uint8_t method;
    uint8_t unit_count;
    uint8_t id_count;
    dcu_scheme_cfg_t units[MAX_SCHEMES];
    uint8_t ids[MAX_SCHEMES];
} dcu_scheme_op_t;


/* 任务槽状态。FREE 是删除后的占位：槽位不压缩，表级 task_mask 的位含义才不会漂。 */
#define DCU_TASK_STATUS_FREE 0
#define DCU_TASK_STATUS_NORMAL 1
#define DCU_TASK_STATUS_DISABLED 2

typedef struct {
    uint32_t task_id;
    uint32_t interval_s;
    uint8_t interval_unit;        /* TI 原样保留，供 A2 精确读回 */
    uint16_t interval_value;
    bool has_interval;            /* TI value == 0 表示无周期触发，不是"每轮" */
    uint8_t status;
    uint8_t period_type;          /* 空时段表也接受的类型，A2 读回 */
    uint8_t scheme_type;
    uint32_t scheme_id;
    /* 本任务要方案里的哪些 CSD：位 i = scheme->csd_list[i]。
     * 0 视为"全部"（兼容既有配置，见 add_task 的默认值）。 */
    uint32_t item_mask;
    uint16_t cursor_meter_idx;
    uint64_t last_run_time;       // [Added] 上次执行时间戳 (保留向后兼容)
    edge_periodic_clock_t clock;  // [Issue #216] 统一抗漂移单调时钟
    dcu_task_monitor_t monitor;
} dcu_task_unit_t;

/* 6013 采集任务配置单元（A.7）。字段宽度按线上 unsigned/enum 收窄。 */
typedef struct {
    uint8_t task_id;
    uint8_t interval_unit;
    uint16_t interval_value;
    uint8_t scheme_type;
    uint8_t scheme_id;
    uint8_t start_time[7];        /* date_time_s 值；全 0 = 无窗口 */
    uint8_t end_time[7];
    uint8_t delay_unit;
    uint16_t delay_value;
    uint8_t priority;
    uint8_t status;
    uint16_t pre_script;
    uint16_t post_script;
    uint8_t period_type;
    uint8_t period_count;         /* 仅用于形状校验；>0 在本产品被拒 */
} dcu_task_config_t;

/* 6012 方法的解码结果。整帧先完整解码（DAR 7），再做字段策略（DAR 2/3/6）。 */
typedef struct {
    uint8_t method;
    uint8_t unit_count;
    uint8_t id_count;
    dcu_task_config_t units[MAX_TASKS];
    uint8_t ids[MAX_TASKS];
} dcu_task_op_t;

// === [Consumer-Defined Interfaces] ===
// 1. 数据持久化接口

typedef struct {
    void* ctx;
    int (*record_telemetry)(void* ctx, const char* meter_addr, uint32_t meter_idx, uint32_t tag_id,
                            double val);
    int (*commit_batch)(void* ctx);
} poller_sink_ops_t;

// 2. 实时数据总线接口

typedef struct {
    void* ctx;
    int (*publish_realtime)(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                            uint64_t timestamp_ns);
    int (*push_event)(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                      uint64_t timestamp_ns);
} poller_bus_ops_t;

// 3. 采样数据源接口 (Source Adapter Seam)
//
// 链式提交：submit 只负责把一帧交给总线（arbiter 决定何时真正发出去），
// 结果通过 cb 回调。链**不判断通道是否空闲**——那是 arbiter 的职责
// （arbiter_bus_status_t.is_busy / queue_len）。链只保留一个自计数 outstanding，
// 表示"我提交了几个还没回来"，用来防止把 arbiter 的队列灌爆。
typedef void (*poller_read_done_cb)(void* ud, int status, double value);

typedef struct {
    void* ctx;
    /** @return 0=已受理（无论是否立即发出），非 0=提交失败，不会再有回调 */
    int (*submit)(void* ctx, uint32_t meter_idx, uint32_t tag_id, poller_read_done_cb cb, void* ud);
} poller_source_ops_t;


#include "driver_manager.h"


/* 表级轮询聚合：连败计数与熔断状态归 poller 这一个根，不归通道。
 * 以表地址为键（档案删除会重排数组下标，下标不是稳定身份），开放寻址。 */
typedef struct {
    char address[MAX_ADDR_LEN]; // 与 production_meter_t.address 同宽
    uint32_t consecutive_fails;
    uint32_t gen; // 档案存活代际，用于退休已删表的条目
    uint8_t channel_id;
    /* 这块表归哪些任务采集：位 i = poller->tasks[i]（任务注册顺序）。
     * task_mask_set=false 表示"未配置"，沿用旧行为（所有任务都抄全表）；
     * 一旦显式配置过就严格按位过滤，0 表示这块表不参与任何任务。
     * 之所以不放进 production_meter_t：那是档案聚合，调度归属是运行时状态，
     * 而且档案结构里还挂着待删除的 inject 指针，不该再往里塞东西。 */
    uint16_t task_mask;
    bool task_mask_set;
    /* 注：S3 曾在这里放 done_mask/done_step 做"同一步去重"。链式模型下不需要——
     * 游标每轮只访问每块表一次，在一次访问里把到期任务的项取并集，结构上就不会重复。 */
    bool breaker_open;
    bool tracked;
    bool tombstone; // 已退休：探测链必须跳过它，不能当作空槽终止
} dcu_meter_poll_state_t;

/**
 * @brief 一条物理通道的提交链。
 *
 * 调度状态只有游标和一个自计数，没有队列、没有 in-flight 表：
 *   - 游标扫一圈 = 一个窗口（round），窗口内每块表只被访问一次；
 *   - outstanding 只约束"我给 arbiter 塞了几个"，通道忙不忙由 arbiter 判；
 *   - pending_tags 是本次访问要抄的项（已按 tag_id 去重），逐个提交。
 */
typedef struct {
    void*    owner;                  // 回指 dcu_poller_t（回调里要用）
    uint8_t  channel_id;
    uint16_t cursor;                 // 下一块要看的表（档案下标）
    uint32_t round;                  // 本通道窗口号（扫完一圈 +1）
    uint8_t  outstanding;            // 已提交未回调的数量（自计数）
    uint8_t  depth;                  // 允许并发，串口半双工默认 1
    uint32_t meter_idx;              // 本次访问的表
    uint32_t inflight_tag;           // 在途那一项的 tag
    uint32_t pending_tags[MAX_SCHEME_OADS]; // 本次访问要抄的项（按 tag 去重后）
    uint8_t  pending_n;
    uint8_t  pending_i;
    uint16_t visit_ok;               // 本次访问成功项数
    uint16_t visit_fail;             // 本次访问失败项数
    uint16_t scanned_in_round;       // 本圈已看过多少块表（一圈=一步）
    bool     round_ended;            // 本步的这一圈已经扫完，不再继续
    /* 防重入：数据源若同步回调（测试 mock、或底层立即返回），
     * 回调里再进 pump 会一层层递归下去。有这个标志就退化成循环。 */
    bool     pumping;
} dcu_chain_t;

typedef struct {
    dcu_task_unit_t tasks[MAX_TASKS];
    uint16_t task_count;
    dcu_scheme_t schemes[MAX_SCHEMES];
    uint16_t scheme_count;

    uv_timer_t timer;

    /* 6012/6014 解码暂存：op 都超过 512 字节，放聚合里而不是调用栈上。
     * 只在 uv 循环线程使用，和 tasks[]/schemes[] 的所有权一致。 */
    dcu_task_op_t task_op;
    dcu_scheme_op_t scheme_op;

    // [New] 契约接口
    poller_sink_ops_t sink_ops;
    poller_bus_ops_t bus_ops;
    poller_source_ops_t source_ops;

    // [AEP-401 & AEP-399] 通道级背压、in-flight 约束与熔断可观测性
    edge_poll_stats_t channel_stats[DCU_MAX_CHANNELS];
    edge_poll_policy_t channel_policies[DCU_MAX_CHANNELS];
    // 通道级跟踪/熔断计数：由表级状态变更点维护，避免每次取统计都扫全表
    uint16_t ch_tracked[DCU_MAX_CHANNELS];
    uint16_t ch_broken[DCU_MAX_CHANNELS];

    dcu_meter_poll_state_t meter_poll_state[METER_TABLE_CAP];
    uint32_t state_gen;     // 档案变更代际
    uint32_t last_inv_rev;  // 上次 GC 时的档案版本号

    /* 每通道一条链：通道之间独立推进，慢通道不拖累快通道。
     * 这是"通道并行"的全部实现——没有线程，只有各自独立的游标与在途预算。 */
    dcu_chain_t chains[DCU_MAX_CHANNELS];
    /* 当前档案快照：完成回调要靠它推进游标（回调签名里没有 inv） */
    dcu_inventory_t* inv;
    /* 每步快照"哪些任务到期"。edge_periodic_clock_check 会推进时钟，
     * 必须每步只问一次——在链里对每块表各问一次的话，第二块表就被判成"未到期"。 */
    bool task_due[MAX_TASKS];
} dcu_poller_t;

int dcu_poller_init(dcu_poller_t* poller);
int dcu_poller_add_task(dcu_poller_t* poller, uint32_t task_id, uint32_t interval_s,
                        uint32_t scheme_id);

/**
 * @brief 注册任务并指定它要方案里的哪些数据项。
 *
 * @param item_mask 位 i = scheme->csd_list[i]；传 0 等价于 add_task（全部项）。
 *                  位宽 32 与 MAX_SCHEME_OADS 对齐。
 */
int dcu_poller_add_task_ex(dcu_poller_t* poller, uint32_t task_id, uint32_t interval_s,
                           uint32_t scheme_id, uint32_t item_mask);

/**
 * @brief 把 TI (单位 0..3) 折算成秒；单位 4/5 或溢出返回 -1。
 */
int dcu_task_interval_seconds(uint8_t unit, uint16_t value, uint32_t* out_seconds);

/** @brief 按 task_id 增/改一条任务；周期变化才重置时钟，其余运行态保留。 */
int dcu_poller_upsert_task(dcu_poller_t* poller, const dcu_task_unit_t* task);

/** @brief 删除任务：槽位置 FREE，不压缩，避免 task_mask 位改指别的任务。 */
int dcu_poller_remove_task(dcu_poller_t* poller, uint32_t task_id);

/** @brief 清空全部任务配置。 */
int dcu_poller_clear_tasks(dcu_poller_t* poller);

/** @brief 只改状态；DISABLED→NORMAL 重置时钟，恢复后不补发。 */
int dcu_poller_set_task_status(dcu_poller_t* poller, uint32_t task_id, uint8_t status);

/** @brief 按 task_id 找任务；FREE 槽不算命中。 */
const dcu_task_unit_t* dcu_poller_task_find(const dcu_poller_t* poller, uint32_t task_id);

/** @brief 取槽位；slot 越界返回 NULL。FREE 槽也返回，调用方需看 status。 */
const dcu_task_unit_t* dcu_poller_task_at(const dcu_poller_t* poller, uint16_t slot);

/**
 * @brief 声明某块表归哪些任务采集（位 i = 任务注册顺序）。
 *
 * 不调用时沿用旧行为：所有任务都抄全表（这样既有配置不需要迁移）。
 * 调用后该表严格按位过滤，传 0 表示这块表不参与任何任务。
 *
 * @note 表必须已在档案里，否则下一次档案变更 GC 会把这个状态条目退休。
 */
int dcu_poller_set_meter_tasks(dcu_poller_t* poller, const char* address, uint8_t channel_id,
                               uint16_t task_mask);
int dcu_poller_add_scheme(dcu_poller_t* poller, uint32_t scheme_id, uint8_t mode,
                          const uint32_t* csds, uint16_t csd_count);
dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t* poller, uint32_t scheme_id);

/** @brief 按 scheme_id 增/改一个方案；表满返回 -1。 */
int dcu_poller_upsert_scheme(dcu_poller_t* poller, const dcu_scheme_t* scheme);

/** @brief 删除方案并压缩数组（方案槽位不是外部身份，任务只认 scheme_id）。 */
int dcu_poller_remove_scheme(dcu_poller_t* poller, uint32_t scheme_id);

/** @brief 清空全部方案配置。 */
int dcu_poller_clear_schemes(dcu_poller_t* poller);

/** @brief 是否有活动任务引用该方案（FREE 槽不算）。 */
bool dcu_poller_scheme_referenced(const dcu_poller_t* poller, uint32_t scheme_id);

/** @brief 引用该方案的活动任务里是否有按位选列的（item_mask != 0）。 */
bool dcu_poller_scheme_masked(const dcu_poller_t* poller, uint32_t scheme_id);

int dcu_poller_start(dcu_poller_t* poller, poller_source_ops_t source_ops, poller_bus_ops_t bus_ops,
                     poller_sink_ops_t sink_ops);

void dcu_poller_pump(dcu_poller_t* poller, dcu_inventory_t* inv);

/**
 * @brief 异步分步推进轮询调度 (消除 libuv 事件循环长时间阻塞)
 */
int dcu_poller_step(dcu_poller_t* poller, dcu_inventory_t* inv, uint32_t max_meters_per_step);

/**
 * @brief 设置特定物理通道的调度与熔断策略 (AEP-401)
 */
int dcu_poller_set_channel_policy(dcu_poller_t* poller, uint32_t channel_id,
                                  const edge_poll_policy_t* policy);

/**
 * @brief 获取特定物理通道的实时运行与背压统计 (AEP-401)
 */
int dcu_poller_get_channel_stats(dcu_poller_t* poller, uint32_t channel_id,
                                 edge_poll_stats_t* out_stats);

/* 表级熔断可观测（按表地址查，不暴露聚合内部结构） */
bool dcu_poller_meter_breaker_open(const dcu_poller_t* poller, const char* address);

/**
 * @brief 重置通道熔断状态，恢复在线调度 (AEP-401)
 */
int dcu_poller_reset_circuit_breaker(dcu_poller_t* poller, uint32_t channel_id);

void dcu_poller_stop(dcu_poller_t* poller);
void dcu_poller_destroy(dcu_poller_t* poller);

#endif
