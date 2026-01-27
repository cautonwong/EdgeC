/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <sys/stat.h>
#include <errno.h>

#undef errno
extern int errno;

__WEAK int _wait(int* status)
{
    errno = ECHILD;
    return -1;
}
