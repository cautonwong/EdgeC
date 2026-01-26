/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <errno.h>

#undef errno
extern int errno;

__WEAK int _close(int fd)
{
    errno = EBADF;
    return -1;
}
