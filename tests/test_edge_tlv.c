#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "edge_tlv.h"

static void test_tlv_pack_unpack_basic(void **state) {
    (void)state;
    uint8_t buffer[128];
    uint32_t tag_id = 12005;  // Raw Heart Rate
    double val = 76.5;
    uint64_t ts = 1700000000;

    int packed_len = edge_tlv_pack_scalar(buffer, sizeof(buffer), 1, tag_id, val, ts);
    assert_true(packed_len > 0);

    uint16_t out_type = 0;
    uint32_t out_tag_id = 0;
    double out_val = 0;
    uint64_t out_ts = 0;

    int res = edge_tlv_unpack_scalar(buffer, (size_t)packed_len, &out_type, &out_tag_id, &out_val,
                                     &out_ts);
    assert_int_equal(res, 0);
    assert_int_equal(out_type, 1);
    assert_int_equal(out_tag_id, tag_id);
    assert_double_equal(out_val, val, 0.001);
    assert_int_equal(out_ts, ts);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tlv_pack_unpack_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
