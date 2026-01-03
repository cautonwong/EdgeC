#include "edge_version.h"

#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Default macros to prevent compilation errors if CMake doesn't inject them
#ifndef EDGE_VER_MAJOR
#define EDGE_VER_MAJOR 0
#endif
#ifndef EDGE_VER_MINOR
#define EDGE_VER_MINOR 0
#endif
#ifndef EDGE_VER_PATCH
#define EDGE_VER_PATCH 0
#endif
#ifndef EDGE_VER_STR
#define EDGE_VER_STR "0.0.0-dev"
#endif
#ifndef EDGE_GIT_HASH
#define EDGE_GIT_HASH "unknown"
#endif
#ifndef EDGE_BUILD_TIME
#define EDGE_BUILD_TIME "unknown"
#endif

// Static instance to avoid dynamic allocation
static const edge_version_t g_edge_version = {.major = EDGE_VER_MAJOR,
                                              .minor = EDGE_VER_MINOR,
                                              .patch = EDGE_VER_PATCH,
                                              .version_str = EDGE_VER_STR,
                                              .git_hash = EDGE_GIT_HASH,
                                              .build_time = EDGE_BUILD_TIME};

const edge_version_t *edge_get_version(void) {
    return &g_edge_version;
}

int edge_get_app_root(char *out_path, size_t size) {
    char exe_path[512];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char *dir = dirname(exe_path);  // bin directory
        char *root = dirname(dir);      // app root directory
        strncpy(out_path, root, size - 1);
        return 0;
    }
    return -1;
}

int edge_add(int a, int b) {
    return a + b;
}
