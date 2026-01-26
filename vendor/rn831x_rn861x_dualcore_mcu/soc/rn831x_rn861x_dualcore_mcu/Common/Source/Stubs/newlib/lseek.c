/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <errno.h>

#undef errno
extern int errno;

__WEAK int _lseek(int file, int offset, int whence)
{
	UNUSED(errno);
    return 0;
}
