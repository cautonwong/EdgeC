#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_appconn.h"

static int contains(const uint8_t *hay, size_t hay_len, const uint8_t *needle, size_t needle_len) {
    for (size_t i = 0; i + needle_len <= hay_len; i++) {
        if (memcmp(hay + i, needle, needle_len) == 0) return 1;
    }
    return 0;
}

static void test_object_list_shape_and_capacity(void **state) {
    (void)state;
    uint8_t buf[2048];
    size_t len = 0;
    assert_int_equal(dcu_698_appconn_object_list(buf, sizeof(buf), &len), 0);
    assert_true(len > 0);
    assert_int_equal(buf[0], 0x01); /* array */
    assert_int_equal(buf[1], 31);
    assert_int_equal(buf[2], 0x02); /* structure */
    assert_int_equal(buf[3], 0x02);

    static const uint8_t param_4000[] = {
        0x02, 0x02, 0x50, 0x40, 0x00, 0x02, 0x02, 0x01, 0x03, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03,
        0x02, 0x02, 0x11, 0x03, 0x16, 0x03, 0x02, 0x02, 0x11, 0x04, 0x16, 0x03, 0x01, 0x00,
    };
    assert_true(contains(buf, len, param_4000, sizeof(param_4000)));

    uint8_t small[4];
    size_t small_len = 0;
    assert_int_not_equal(dcu_698_appconn_object_list(small, sizeof(small), &small_len), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_object_list_shape_and_capacity),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
