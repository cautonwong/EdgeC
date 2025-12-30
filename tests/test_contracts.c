#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "lookup.h"
#include "edge_tag_id.h"

static void test_contract_modbus_lookup(void **state) {
    (void)state;

    // Test a valid mapping defined in default_device.yaml
    edge_tag_id_t tag = lookup_tag_by_modbus_addr(40001);
    assert_int_equal(tag, TAG_GRID_POWER);

    // Test a non-existent address
    tag = lookup_tag_by_modbus_addr(9999);
    assert_int_equal(tag, TAG_INVALID);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_contract_modbus_lookup),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
