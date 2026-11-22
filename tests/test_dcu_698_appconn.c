#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "dcu_698_appconn.h"

static void test_object_list_shape_and_capacity(void **state) {
    (void)state;
    uint8_t buf[2048];
    size_t len = 0;
    assert_int_equal(dcu_698_appconn_object_list(buf, sizeof(buf), &len), 0);
    assert_true(len > 0);
    assert_int_equal(buf[0], 0x01); /* array */
    assert_int_equal(buf[1], 17);
    assert_int_equal(buf[2], 0x02); /* structure */
    assert_int_equal(buf[3], 0x02);

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
