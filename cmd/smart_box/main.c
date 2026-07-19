#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include "edge_log.h"

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    LOG_INFO(LOG_CAT_CORE, "=== Starting Smart Box Controller ===");
    uv_loop_t *loop = uv_default_loop();
    LOG_INFO(LOG_CAT_CORE, "Smart Box Loop initialized successfully");
    return uv_run(loop, UV_RUN_DEFAULT);
}
