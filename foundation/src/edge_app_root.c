#include "edge_common.h"
#include <unistd.h>
#include <string.h>

int edge_get_app_root(char* out_path, size_t size) {
    ssize_t len = readlink("/proc/self/exe", out_path, size - 1);
    if (len != -1) {
        out_path[len] = '\0';
        char* last_slash = strrchr(out_path, '/');
        if (last_slash) *last_slash = '\0';
        return 0;
    }
    return -1;
}
