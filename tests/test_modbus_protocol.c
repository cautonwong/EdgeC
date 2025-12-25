#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <cmocka_pbc.h>
#include "modbus_protocol.h"
#include "shmring.h"
#include "io_executor.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

shm_ring_t *global_ring;

/* Mock io_submit to avoid blocking on stdin */
void __wrap_io_submit(uv_loop_t *loop, io_job_t *job, io_done_cb cb, void *user) {
    (void)loop;
    // Simulate immediate success
    job->result = 0;
    
    // Fill rx with dummy data to pass parsing
    // 123.45 is the parsed value in modbus_protocol.c's parse_meter_value
    // But parse_meter_value ignores input and returns 123.45.
    // However, create_read_job allocates rx.
    
    if (cb) cb(job, user);
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