#include <time.h>

#include "sched_time.h"

/**
 * @file sched_port.c
 * @brief 时间源物理实现 (支持 S10 挂钟对齐)
 * 使用 weak 符号方便测试 Mock
 */

__attribute__((weak)) wall_time_t sched_wall_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (wall_time_t)ts.tv_sec * 1000 + (wall_time_t)ts.tv_nsec / 1000000;
}

__attribute__((weak)) mono_time_t sched_mono_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (mono_time_t)ts.tv_sec * 1000 + (mono_time_t)ts.tv_nsec / 1000000;
}
