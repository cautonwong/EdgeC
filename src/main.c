#include <stdio.h>
#include <unistd.h>
#include <uv.h>
#include "edge_version.h"
#include "core_mem.h"
#include "core_sched.h"

// Sample cron job callback
void my_cron_job(void *arg) {
    const char *message = (const char *)arg;
    printf("Cron job triggered! Message: %s Timestamp: %ld\n", message, time(NULL));
}

int main(void) {
    printf("Starting %s...\n", edge_get_version());

    // Initialize the main event loop
    uv_loop_t *loop = uv_default_loop();

    // --- Phase 1: Initialize Core Services ---
    if (core_mem_init() != 0) {
        fprintf(stderr, "Failed to initialize memory manager\n");
        return 1;
    }
    if (core_sched_init(loop, 10) != 0) {
        fprintf(stderr, "Failed to initialize scheduler\n");
        return 1;
    }

    // --- Test the services ---
    printf("Core services initialized.\n");
    
    // Test memory manager
    net_buf_t *buf = core_net_buf_alloc(0);
    if (buf) {
        printf("Allocated a small net_buf from pool %d\n", buf->pool_id);
        core_net_buf_unref(buf);
    }

    // Test scheduler by adding a job to run every 10 seconds
    core_sched_add_cron("*/10 * * * * *", my_cron_job, "Hello from Scheduler!");
    printf("Added a cron job to run every 10 seconds.\n");
    
    printf("Running event loop for 30 seconds...\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    // Loop will exit when there are no more active handles.
    // Let's run it for a fixed duration for this demo.
    uv_loop_close(loop);
    core_sched_destroy();
    
    printf("EdgeOS Seed finished.\n");
    return 0;
}

