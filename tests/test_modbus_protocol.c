#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <cmocka_pbc.h>
#include "modbus_protocol.h"
#include "shmring.h"
#include "io_executor.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

shm_ring_t *global_ring;

/* Mock io_executor_submit to avoid real IO and threading */
int __wrap_io_executor_submit(io_job_t *job) {
    // Simulate immediate success
    job->result = IO_RESULT_SUCCESS;
    
    // In the new API, the callback is part of the job.
    // The test for modbus_read_request sets a callback that frees the job.
    if (job->cb) {
        job->cb(job, job->user_data);
    }

    return 0; // Return success
}

static void test_protocol_flow(void **state) {
    (void)state;
    shm_ring_t ring;
    char ring_name[] = "/test_ring_proto";
    shm_unlink(ring_name);
    shmring_create(&ring, ring_name, 8);
    global_ring = &ring;

    uv_loop_t loop;
    uv_loop_init(&loop);

    modbus_protocol_init(&loop);
    modbus_read_request(101); // Read meter 101

    // Wait for async IO to complete and write to ring
    int i;
    int found = 0;
    for (i=0; i<100; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        
        // Check ring
        PipeMsg m;
        uint64_t seq = 0;
        if (shm_ring_read(&ring, &seq, &m) == 0) {
             if (m.payload.val_f > 100.0) {
                 found = 1;
                 break;
             }
        }
        usleep(2000);
    }

    assert_int_equal(found, 1);

    uv_loop_close(&loop);
    shmring_close(&ring);
    shmring_destroy(&ring);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_protocol_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}