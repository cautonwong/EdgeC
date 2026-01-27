/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <errno.h>
#include <time.h>

/* Set CLOCK to value TP. */
__WEAK int clock_settime(clockid_t clock_id, const struct timespec* tp)
{
    return -1;
}
