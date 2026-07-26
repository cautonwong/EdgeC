/**
 * @file test_shm_waitset.c
 * @brief Unit tests for Zero-Copy Wait-Set Event Engine Operator
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <cmocka.h>
#include <string.h>

#include "shm_waitset.h"

static void test_shm_waitset_init_null(void **state) {
    (void)state;
    assert_int_equal(shm_waitset_init(NULL), -1);
}

static void test_shm_waitset_workflow(void **state) {
    (void)state;
    shm_waitset_t ws;
    assert_int_equal(shm_waitset_init(&ws), 0);

    uint32_t sub1 = 0, sub2 = 0;
    assert_int_equal(shm_waitset_register_subscriber(&ws, &sub1), 0);
    assert_int_equal(shm_waitset_register_subscriber(&ws, &sub2), 0);
    assert_int_equal(ws.active_subscribers, 2);

    /* Poll before publish -> return 1 (no new chunk) */
    const shm_chunk_t *chunk_ptr = NULL;
    assert_int_equal(shm_waitset_poll_chunk(&ws, sub1, &chunk_ptr), 1);

    /* Zero-copy Publish */
    uint8_t payload[64] = "ZeroCopyWaitSetPacketData";
    assert_int_equal(shm_waitset_publish_chunk(&ws, payload, 25, 123456789ULL), 0);

    /* Poll Sub1 -> return 0 (new chunk) */
    assert_int_equal(shm_waitset_poll_chunk(&ws, sub1, &chunk_ptr), 0);
    assert_non_null(chunk_ptr);
    assert_int_equal(chunk_ptr->data_len, 25);
    assert_int_equal(chunk_ptr->timestamp_ns, 123456789ULL);
    assert_memory_equal(chunk_ptr->payload, "ZeroCopyWaitSetPacketData", 25);

    /* Poll Sub1 again -> return 1 (already read) */
    assert_int_equal(shm_waitset_poll_chunk(&ws, sub1, &chunk_ptr), 1);

    /* Poll Sub2 -> return 0 (new chunk for sub2) */
    const shm_chunk_t *chunk_ptr2 = NULL;
    assert_int_equal(shm_waitset_poll_chunk(&ws, sub2, &chunk_ptr2), 0);
    assert_ptr_equal(chunk_ptr, chunk_ptr2); /* Verify exact same zero-copy pointer */

    /* Unregister */
    assert_int_equal(shm_waitset_unregister_subscriber(&ws, sub1), 0);
    assert_int_equal(ws.active_subscribers, 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_shm_waitset_init_null),
        cmocka_unit_test(test_shm_waitset_workflow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
