#include <string.h>

static int dispatch(const char *fn) {
    if (strcmp(fn, "app_init") == 0) return 1;
    if (strcmp(fn, "app_on_event") == 0) return 1;
    if (strcmp(fn, "app_stop") == 0) return 1;
    return 0;
}
