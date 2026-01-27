/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <errno.h>
#include <sys/types.h>

#undef errno
extern int errno;

__WEAK int _symlink(const char *path1, const char *path2)
{
    errno = ENOSYS;
    return -1;
}
