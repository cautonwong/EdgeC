#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "shmring.h"

#define FUZZ_ITERATIONS 100000

static void test_fuzz_read_write(void **state) {
    (void)state;
    shm_ring_t ring;
    assert_int_equal(shmring_create(&ring, "fuzz_ring", 1024), 0);

    srand(time(NULL));
    uint64_t last_seq = 0;
    PipeMsg w_msg, r_msg;

    for (int i = 0; i < FUZZ_ITERATIONS; i++) {
        // Random Write
        pipe_msg_init_scalar(&w_msg, (uint32_t)rand(), (double)rand() / RAND_MAX,
                             (uint64_t)time(NULL));
        shm_ring_write(&ring, &w_msg);

        // Random Read
        if (rand() % 2 == 0) {
            if (shm_ring_read(&ring, &last_seq, &r_msg) == 0) {
                // Consistency check if seq updated
                assert_true(r_msg.ts > 0);
            }
        }
    }

    shmring_destroy(&ring);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fuzz_read_write),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
