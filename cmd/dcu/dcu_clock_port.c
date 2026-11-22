#include "dcu_clock_port.h"

#include <errno.h>
#include <time.h>

static int system_now(void *ctx, struct tm *out) {
    (void)ctx;
    time_t now = time(NULL);
    if (now == (time_t)-1) return -1;
    return localtime_r(&now, out) ? 0 : -1;
}

static int system_set(void *ctx, const struct tm *in) {
    (void)ctx;
    struct tm value = *in;
    value.tm_isdst = -1;
    time_t secs = mktime(&value);
    if (secs == (time_t)-1) return -EINVAL;
    struct timespec ts = {.tv_sec = secs, .tv_nsec = 0};
    return clock_settime(CLOCK_REALTIME, &ts) == 0 ? 0 : -errno;
}

static const dcu_clock_port_t k_system_clock = {
    .now = system_now, .set = system_set, .ctx = NULL};

const dcu_clock_port_t *dcu_clock_port_system(void) {
    return &k_system_clock;
}
