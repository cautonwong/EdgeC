#include "edge_version.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

// 由 CMake 注入的宏定义，如果不存在则使用默认值
#ifndef EDGE_VER_MAJOR
#define EDGE_VER_MAJOR 4
#endif
#ifndef EDGE_VER_MINOR
#define EDGE_VER_MINOR 5
#endif
#ifndef EDGE_VER_PATCH
#define EDGE_VER_PATCH 0
#endif
#ifndef EDGE_VER_STR
#define EDGE_VER_STR "4.5.0"
#endif
#ifndef EDGE_GIT_HASH
#define EDGE_GIT_HASH "unknown"
#endif
#ifndef EDGE_BUILD_TIME
#define EDGE_BUILD_TIME "now"
#endif

static const edge_version_t g_version = {.major = EDGE_VER_MAJOR,
                                         .minor = EDGE_VER_MINOR,
                                         .patch = EDGE_VER_PATCH,
                                         .version_str = EDGE_VER_STR,
                                         .git_hash = EDGE_GIT_HASH,
                                         .build_time = EDGE_BUILD_TIME};

const edge_version_t* edge_get_version(void) {
    return &g_version;
}

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