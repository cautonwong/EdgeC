/* See LICENSE of license details. */
#include "rn831x_rn861x_mcu.h"
#include <sys/stat.h>

__WEAK int _stat(char* file, struct stat* st)
{
    st->st_mode = S_IFCHR;
    return 0;
}
