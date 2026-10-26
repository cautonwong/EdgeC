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
            uint8_t capture_mode;
            uint8_t ms_type;
            char ms_addrs[8][MAX_ADDR_LEN];
            uint8_t ms_count;
            uint32_t csd_list[MAX_SCHEME_OADS];
            uint16_t csd_count;
        } normal;
    } data;
} dcu_scheme_t;


typedef struct {
    uint32_t task_id;
    uint32_t interval_s;
    uint8_t scheme_type;
    uint32_t scheme_id;
    uint16_t cursor_meter_idx;
    uint64_t last_run_time;       // [Added] 上次执行时间戳 (保留向后兼容)
    edge_periodic_clock_t clock;  // [Issue #216] 统一抗漂移单调时钟
    dcu_task_monitor_t monitor;
} dcu_task_unit_t;

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

typedef struct {
    void* ctx;
    int (*read_value)(void* ctx, uint32_t meter_idx, uint32_t tag_id, double* out_val);
} poller_source_ops_t;

#include "driver_manager.h"


/* 表级轮询聚合：连败计数与熔断状态归 poller 这一个根，不归通道。
 * 以表地址为键（档案删除会重排数组下标，下标不是稳定身份），开放寻址。 */
typedef struct {
    char address[MAX_ADDR_LEN]; // 与 production_meter_t.address 同宽
    uint32_t consecutive_fails;
    uint32_t gen; // 档案存活代际，用于退休已删表的条目
    uint8_t channel_id;
    bool breaker_open;
    bool tracked;
    bool tombstone; // 已退休：探测链必须跳过它，不能当作空槽终止
} dcu_meter_poll_state_t;

typedef struct {
    dcu_task_unit_t tasks[MAX_TASKS];
    uint16_t task_count;
    dcu_scheme_t schemes[MAX_SCHEMES];
    uint16_t scheme_count;

    uv_timer_t timer;
    uv_mutex_t lock;

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
} dcu_poller_t;

int dcu_poller_init(dcu_poller_t* poller);
int dcu_poller_add_task(dcu_poller_t* poller, uint32_t task_id, uint32_t interval_s,
                        uint32_t scheme_id);
int dcu_poller_add_scheme(dcu_poller_t* poller, uint32_t scheme_id, uint8_t mode,
                          const uint32_t* csds, uint16_t csd_count);
dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t* poller, uint32_t scheme_id);
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
