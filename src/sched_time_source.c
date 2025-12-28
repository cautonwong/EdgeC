#include <time.h>

#include "sched_time.h"

// Weak linkage allows overriding for tests without recompiling everything if using static libs
// correctly, but for simple testing, we might just link a different object file. For now, standard
// implementation.

wall_time_t sched_wall_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (wall_time_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

mono_time_t sched_mono_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (mono_time_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
