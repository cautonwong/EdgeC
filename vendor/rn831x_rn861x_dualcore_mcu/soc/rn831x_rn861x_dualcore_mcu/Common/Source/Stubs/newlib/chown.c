/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <errno.h>
#include <sys/types.h>

#undef errno
extern int errno;

__WEAK int _chown(const char *path, uid_t owner, gid_t group)
{
    errno = ENOSYS;
    return -1;
}
