/**
 * @file iems_internal_bus_adapter.h
 * @brief IEMS 内部总线适配器（ADC 采集线程）
 */

#ifndef IEMS_INTERNAL_BUS_ADAPTER_H
#define IEMS_INTERNAL_BUS_ADAPTER_H

#include <stdbool.h>
#include <uv.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uv_loop_t *loop;
    datahub_t *hub;
    uv_timer_t timer;
    uv_thread_t tid;
    bool timer_initialized;
    bool thread_started;
} iems_internal_bus_t;

/**
 * @brief 启动内部总线探测/ADC 线程（每 500ms 探测一次）。
 */
void iems_internal_bus_start(iems_internal_bus_t *ibus, uv_loop_t *loop, datahub_t *hub);

/**
 * @brief 停止探测定时器（线程需在事件循环跑完后再 join）。
 */
void iems_internal_bus_stop(iems_internal_bus_t *ibus);

/**
 * @brief 等待 ADC 线程退出。
 */
void iems_internal_bus_join(iems_internal_bus_t *ibus);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_INTERNAL_BUS_ADAPTER_H
