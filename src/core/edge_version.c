#include "edge_version.h"

#include <libgen.h>
#include <string.h>
#include <unistd.h>

const char *edge_get_version(void) {
    return "EdgeOS SDK v4.5-Stable";
}

int edge_get_app_root(char *out_path, size_t size) {
    char exe_path[512];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char *dir = dirname(exe_path);  // bin 目录
        char *root = dirname(dir);      // 应用根目录
        strncpy(out_path, root, size - 1);
        return 0;
    }
    return -1;
}

int edge_add(int a, int b) {
    return a + b;
}
