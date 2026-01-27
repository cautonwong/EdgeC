/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <unistd.h>

__WEAK int _isatty(int fd)
{
    return 1;
}
