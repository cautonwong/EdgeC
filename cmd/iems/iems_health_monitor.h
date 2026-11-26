/**
 * @file iems_health_monitor.h
 * @brief 硬件健康阈值门控：指标 → 5 个系统级告警点（EMS.*）。
 *
 * 设计：一张门控表是唯一真相（tag/threshold/hold + 运行时 over_since/alarm），
 * 纯函数按单调时钟求值并返回变化位图；采样源是端口，测试注入假指标，
 * 组合根装配 Linux 源（/proc/stat、/proc/meminfo、statvfs）。
 */

#ifndef IEMS_HEALTH_MONITOR_H
#define IEMS_HEALTH_MONITOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 门控表顺序不承载语义：每条 gate 用 metric 选择指标，tag_id 决定写哪个点。
 * 表仍是唯一真相。 */
#define IEMS_HEALTH_GATE_COUNT 5

#define IEMS_HEALTH_HOLD_CPU_MEM_NS (300ULL * 1000000000ULL)       /* 300s */
#define IEMS_HEALTH_HOLD_DISK_NS (24ULL * 3600ULL * 1000000000ULL) /* 24h */

typedef enum {
    IEMS_HEALTH_METRIC_POWER = 1,
    IEMS_HEALTH_METRIC_CPU,
    IEMS_HEALTH_METRIC_MEM,
    IEMS_HEALTH_METRIC_EMMC,
    IEMS_HEALTH_METRIC_SSD,
} iems_health_metric_t;

typedef struct {
    bool power_ok;   /**< false = 掉电 */
    double cpu_pct;  /**< 0..100 */
    double mem_pct;  /**< 0..100 */
    double emmc_pct; /**< 根文件系统使用率 0..100 */
    double ssd_pct;  /**< /mnt/ssd 使用率；ssd_present=false 时忽略 */
    bool ssd_present;
} iems_health_metrics_t;

typedef struct {
    uint32_t tag_id;        /**< 告警点位（9001..9005） */
    iems_health_metric_t metric; /**< 该门判定的指标；决定语义，与数组位置无关 */
    double threshold_pct;   /**< 严格大于才置位；电源用 power_ok 判据 */
    uint64_t hold_ns;       /**< 0 = 立即；300s/24h */
    uint64_t over_since_ns; /**< 运行时：连续超阈起点；0 = 未超 */
    bool alarm;             /**< 运行时：当前是否告警 */
} iems_health_gate_t;

/**
 * @brief 纯函数求值：无 IO、无时钟，now_ns 由调用方给（uv_hrtime 单调 ns）。
 * @return 变化位图：bit i = gate i 的 alarm 发生变化（需要写点）。
 */
uint32_t iems_health_evaluate(iems_health_gate_t *gates, size_t n,
                              const iems_health_metrics_t *m, uint64_t now_ns);

/* 采样源端口：0 成功；-1 本拍跳过（保留门控状态）。 */
typedef struct {
    int (*read)(void *ctx, iems_health_metrics_t *out);
    void *ctx;
} iems_health_source_t;

/* 采样与门控的运行时外壳：1s 定时器读源、求值、只在变化时 publish。 */
typedef struct {
    datahub_t *hub;
    iems_health_source_t source;
    iems_health_gate_t gates[IEMS_HEALTH_GATE_COUNT];
    uv_timer_t timer;
    bool timer_initialized;
} iems_health_monitor_t;

/** 装配门控表（静态定义）与源；运行时状态清零。 */
void iems_health_monitor_init(iems_health_monitor_t *mon, datahub_t *hub,
                              iems_health_source_t source);

/** 启动 1s 周期定时器；重复调用无副作用。 */
int iems_health_monitor_start(iems_health_monitor_t *mon, uv_loop_t *loop, uint64_t period_ms);

/** 停止并关闭定时器；未启动时无副作用。 */
void iems_health_monitor_stop(iems_health_monitor_t *mon);

/** 单拍：读源 → 求值 → changed 位图对应的点各 publish 一次。 */
int iems_health_monitor_step(iems_health_monitor_t *mon, uint64_t now_ns);

/* ---- Linux 采样源（/proc/stat、/proc/meminfo、statvfs） ---- */

typedef struct {
    uint64_t total;
    uint64_t idle;
} iems_health_cpu_sample_t;

typedef struct {
    uint64_t prev_total;
    uint64_t prev_idle;
    bool have_prev;
} iems_health_linux_source_t;

void iems_health_linux_source_init(iems_health_linux_source_t *src);
iems_health_source_t iems_health_linux_source_port(iems_health_linux_source_t *src);

/* 纯解析/计算函数：合成文本可直接测。 */
int iems_health_parse_proc_stat(const char *text, iems_health_cpu_sample_t *out);
int iems_health_parse_meminfo(const char *text, double *out_pct);
double iems_health_cpu_pct(const iems_health_cpu_sample_t *prev,
                           const iems_health_cpu_sample_t *cur);
double iems_health_disk_pct(uint64_t total_bytes, uint64_t avail_bytes);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_HEALTH_MONITOR_H */
