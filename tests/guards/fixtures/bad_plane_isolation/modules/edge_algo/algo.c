// VIOLATION: Algorithm directly includes private Southbound driver header
#include <stdint.h>

#include "drivers/driver_private.h"

void algo_step(void) {
    uint32_t x = 0;
    (void)x;
}
