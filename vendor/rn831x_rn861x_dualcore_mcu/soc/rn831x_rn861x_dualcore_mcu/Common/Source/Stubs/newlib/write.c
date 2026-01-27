/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#undef putchar

__WEAK ssize_t _write(int fd, const void* ptr, size_t len)
{
    if (!isatty(fd)) {
        return -1;
    }

    const uint8_t* writebuf = (const uint8_t*)ptr;
    for (size_t i = 0; i < len; i++) {
        putchar((int)writebuf[i]);
    }
    return len;
}
