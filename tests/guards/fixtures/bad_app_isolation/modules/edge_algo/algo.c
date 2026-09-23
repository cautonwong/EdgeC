#include <stdint.h>
// VIOLATION: Algorithm directly includes private RTOS kernel header
#include <zephyr/kernel_internal.h>

void algo_compute(void) {
    int v = 42;
    (void)v;
}
