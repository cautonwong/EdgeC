#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "shmring.h"

#define FUZZ_ITERATIONS 100000

static void test_fuzz_read_write(void** state) {
    (void)state;
    shm_ring_t ring;
    assert_int_equal(shmring_create(&ring, "fuzz_ring", 1024), 0);

    uint32_t seed = 1;
    const char* env_seed = getenv("EDGE_FUZZ_SEED");
    if (env_seed) seed = (uint32_t)strtoul(env_seed, NULL, 0);
    srand(seed);
    uint64_t last_seq = 0;
    uint32_t reads = 0;
    PipeMsg w_msg, r_msg;

    for (int i = 0; i < FUZZ_ITERATIONS; i++) {
        pipe_msg_init_scalar(&w_msg, (uint32_t)i, (double)i, (uint64_t)(i + 1));
        shm_ring_write(&ring, &w_msg);

        if (rand() % 2 == 0) {
            uint64_t seq = last_seq;
            if (shm_ring_read(&ring, &seq, &r_msg) == 0) {
                reads++;
                /* 任何读出的消息都必须是已写过的，且 tag 与值一一对应。 */
                assert_true(seq >= last_seq);
                last_seq = seq;
                assert_true(r_msg.tag_id <= (uint32_t)i);
                assert_true(r_msg.payload.val.f == (double)r_msg.tag_id);
                assert_true(r_msg.ts > 0);
            }
        }
    }

    assert_true(reads > 0);
    shmring_destroy(&ring);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fuzz_read_write),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
