#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "iec_cp56time.h"

static void test_wire_size_is_seven_bytes(void **state) {
    (void)state;
    assert_int_equal(sizeof(iec_cp56time_t), 7);
}

static void test_round_trip(void **state) {
    (void)state;
    const uint64_t samples[] = {946684800000ULL, 1767323045678ULL, 4102444799999ULL};
    for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
        iec_cp56time_t ts;
        iec_cp56time_from_ms(&ts, samples[i]);
        assert_int_equal(iec_cp56time_to_ms(&ts), samples[i]);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wire_size_is_seven_bytes),
        cmocka_unit_test(test_round_trip),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
