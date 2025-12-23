#include <stdio.h>
#include <unistd.h>
#include <uv.h>
#include "edge_version.h"
#include "core_mem.h"
#include "core_sched.h"
#include "shm_ring.h"
#include "db_engine.h"

// Sample cron job callback that now writes to the database
void my_cron_job(void *arg) {
    db_engine_ctx_t *db_ctx = (db_engine_ctx_t *)arg;
    
    printf("Cron job triggered! Timestamp: %ld\n", time(NULL));

    // Create a sample datapoint
    db_datapoint_t dp = {
        .ts = (uint64_t)time(NULL) * 1000,
        .tag_id = 1,
        .value = (double)(rand() % 100),
        .quality = 0,
    };

    // Insert into the database
    if (!db_engine_insert(&dp)) {
        fprintf(stderr, "Failed to insert datapoint into DB!\n");
    }
}

int main(void) {
    printf("Starting %s...\n", edge_get_version());

    uv_loop_t *loop = uv_default_loop();

    // --- Initialize Core Services ---
    if (core_mem_init() != 0) {
        fprintf(stderr, "Failed to initialize memory manager\n");
        return 1;
    }
    if (core_sched_init(loop, 10) != 0) {
        fprintf(stderr, "Failed to initialize scheduler\n");
        return 1;
    }
    if (db_engine_init(loop, "edge_data.db", 100, 5000) != 0) {
        fprintf(stderr, "Failed to initialize database engine\n");
        return 1;
    }

    // Initialize shared memory ring as a producer
    shm_ring_t *ring = shm_ring_init("/my_data_hub", 1024, sizeof(db_datapoint_t), true);
    if (!ring) {
        fprintf(stderr, "Failed to initialize shared memory ring\n");
        return 1;
    }


    printf("Core services initialized.\n");
    
    // Add a cron job to run every 5 seconds and write to the DB
    core_sched_add_cron("*/5 * * * * *", my_cron_job, NULL);
    printf("Added a cron job to run every 5 seconds.\n");
    
    printf("Running event loop for 20 seconds...\n");
    
    // Run loop for a duration for this demo
    uv_timer_t demo_timer;
    uv_timer_init(loop, &demo_timer);
    uv_timer_start(&demo_timer, (uv_timer_cb) uv_stop, 20000, 0);

    uv_run(loop, UV_RUN_DEFAULT);
    
    uv_loop_close(loop);
    core_sched_destroy();
    db_engine_destroy();
    shm_ring_free(ring, "/my_data_hub", true);
    
    printf("EdgeOS Seed finished.\n");
    return 0;
}

