/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#undef getchar

__WEAK ssize_t _read(int fd, void* ptr, size_t len)
{
    if (fd != STDIN_FILENO) {
        return -1;
    }
    ssize_t cnt = 0;
    uint8_t* readbuf = (uint8_t*)ptr;
    for (cnt = 0; cnt < len; cnt ++) {
        readbuf[cnt] = getchar();
        /* Return partial buffer if we get EOL */
        if (readbuf[cnt] == '\n') {
            return cnt;
        }
    }

    return cnt;
}
