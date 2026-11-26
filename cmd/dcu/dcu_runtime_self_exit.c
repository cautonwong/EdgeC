#include "dcu_runtime_self_exit.h"

#include <errno.h>
#include <stdlib.h>

#include <uv.h>

uint64_t dcu_runtime_self_exit_ms(const char *value) {
    if (!value || value[0] < '0' || value[0] > '9') return 0;
    errno = 0;
    char *end = NULL;
    unsigned long long seconds = strtoull(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || seconds == 0) return 0;
    if (seconds > UINT64_MAX / 1000u) return 0;
    return (uint64_t)seconds * 1000u;
}

static uv_timer_t g_self_exit_timer;

void dcu_self_exit_start(uv_loop_t *loop, uv_timer_cb cb) {
    uint64_t ms = dcu_runtime_self_exit_ms(getenv("DCU_SELF_EXIT_SECONDS"));
    if (!loop || !cb || ms == 0) return;
    uv_timer_init(loop, &g_self_exit_timer);
    uv_timer_start(&g_self_exit_timer, cb, ms, 0);
}
