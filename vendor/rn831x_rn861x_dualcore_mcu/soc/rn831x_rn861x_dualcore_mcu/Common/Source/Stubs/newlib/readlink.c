/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <errno.h>
#include <sys/types.h>

#undef errno
extern int errno;

__WEAK int _readlink(const char *path, char *buf, size_t bufsize)
{
    errno = ENOSYS;
    return -1;
}
