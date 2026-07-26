#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "spsc_ring.h"

static void test_spsc_ring_basic(void** state) {
    (void)state;

    spsc_ring_t ring;
    spsc_ring_init(&ring);

    assert_int_equal(spsc_ring_size(&ring), 0);

    spsc_event_t evt1 = {.timestamp = 1000, .tag_id = 12001, .value = 14.2, .flags = 0};
    spsc_event_t evt2 = {.timestamp = 1001, .tag_id = 12002, .value = 88.5, .flags = 1};

    assert_true(spsc_ring_push(&ring, &evt1));
    assert_true(spsc_ring_push(&ring, &evt2));
    assert_int_equal(spsc_ring_size(&ring), 2);

    spsc_event_t out_evt;
    assert_true(spsc_ring_pop(&ring, &out_evt));
    assert_int_equal(out_evt.tag_id, 12001);
    assert_float_equal(out_evt.value, 14.2, 0.001);

    assert_true(spsc_ring_pop(&ring, &out_evt));
    assert_int_equal(out_evt.tag_id, 12002);
    assert_float_equal(out_evt.value, 88.5, 0.001);

    assert_false(spsc_ring_pop(&ring, &out_evt));
    assert_int_equal(spsc_ring_size(&ring), 0);

    printf("🔥 [SUCCESS] SPSC lockless ring buffer test passed!\n");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_spsc_ring_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
