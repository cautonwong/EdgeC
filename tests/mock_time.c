#include "sched_time.h"

// Static mock state
static wall_time_t g_mock_wall = 0;
static mono_time_t g_mock_mono = 0;

void mock_set_time(wall_time_t w, mono_time_t m) {
    g_mock_wall = w;
    g_mock_mono = m;
}

// Override the weak symbols or just link this instead of sched_time_source.c
wall_time_t sched_wall_now(void) {
    return g_mock_wall;
}

mono_time_t sched_mono_now(void) {
    return g_mock_mono;
}
