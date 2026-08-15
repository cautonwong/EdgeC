#include <time.h>
#include <uv.h>

#include "edge_common.h"
#include "sched_time.h"

// 默认弱实现：通过 clock_gettime 获取挂钟和单调时间
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