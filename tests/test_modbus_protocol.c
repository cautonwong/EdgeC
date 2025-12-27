#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include "modbus_protocol.h"
#include "shmring.h"
#include <unistd.h>

shm_ring_t *global_ring;

static void test_protocol_flow(void **state) {
    (void)state;
    shm_ring_t ring;
    char ring_name[] = "/test_ring_proto";
    
    shm_unlink(ring_name);

    assert_int_equal(shmring_create(&ring, ring_name, 8), 0);
    global_ring = &ring;

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    modbus_protocol_init(&loop);
    modbus_read_request(101); 

    int i;
    int found = 0;
    for (i=0; i<100; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        
        PipeMsg m;
        uint64_t seq = 0;
        if (shm_ring_read(&ring, &seq, &m) == 0) {
             if (m.payload.val_f > 100.0) {
                 found = 1;
                 break;
             }
        }
        uv_sleep(2); // libuv sleep
    }

    assert_int_equal(found, 1);

    uv_run(&loop, UV_RUN_DEFAULT);
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
