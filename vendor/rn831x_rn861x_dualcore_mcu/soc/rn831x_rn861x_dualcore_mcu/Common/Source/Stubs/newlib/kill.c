/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <errno.h>

#undef errno
extern int errno;

__WEAK int _kill(int pid, int sig)
{
    errno = EINVAL;
    return -1;
}
