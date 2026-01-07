#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "contracts.h"
#include "edge_log.h"

static int g_running = 1;
static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    edge_log_t *logger = edge_log_create(NULL, true);

    signal(SIGINT, on_signal);
    edge_log_out(logger, LOG_CAT_CORE, 0, "Device Simulator Running...");

    while (g_running) {
        uv_run(uv_default_loop(), UV_RUN_NOWAIT);
        usleep(100000);
    }

    edge_log_destroy(logger);
    return 0;
}