/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <stddef.h>

__WEAK void* __dso_handle = NULL;

__WEAK void _exit(int fd)
{
    while (1) {
        __WFI();
    }
}
