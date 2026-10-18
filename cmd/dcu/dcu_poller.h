#ifndef DCU_POLLER_H
#define DCU_POLLER_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "dcu_inventory.h"
#include "edge_periodic_clock.h"
#include "queue.h"

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

void dcu_poller_stop(dcu_poller_t* poller);
void dcu_poller_destroy(dcu_poller_t* poller);

#endif
