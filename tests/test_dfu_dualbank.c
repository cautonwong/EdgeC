/**
 * @file test_dfu_dualbank.c
 * @brief Unit tests for Dual-Bank A/B OTA Firmware DFU Engine Operator
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dfu_dualbank.h"

static void test_dfu_dualbank_init_null(void **state) {
    (void)state;
    assert_int_equal(dfu_dualbank_init(NULL), -1);
}

static void test_dfu_dualbank_stage_confirm(void **state) {
    (void)state;
    dfu_dualbank_t dfu;
    assert_int_equal(dfu_dualbank_init(&dfu), 0);
    assert_int_equal(dfu.active_bank, DFU_BANK_A);

    uint8_t image[64] = "FirmwareImagePayloadV2";
    uint32_t bad_sig = 0x12345678;
    assert_int_equal(dfu_dualbank_stage_image(&dfu, image, sizeof(image), 0x020000, bad_sig), -2);

    uint32_t good_sig = 0x4F544131;
    assert_int_equal(dfu_dualbank_stage_image(&dfu, image, sizeof(image), 0x020000, good_sig), 0);
    assert_int_equal(dfu.state, DFU_STATE_TESTING);
    assert_int_equal(dfu.pending_bank, DFU_BANK_B);

    /* Confirm health -> swap finalized */
    assert_int_equal(dfu_dualbank_confirm_health(&dfu), 0);
    assert_int_equal(dfu.active_bank, DFU_BANK_B);
    assert_int_equal(dfu.active_version, 0x020000);
    assert_int_equal(dfu.state, DFU_STATE_STABLE);
}

static void test_dfu_dualbank_rollback(void **state) {
    (void)state;
    dfu_dualbank_t dfu;
    assert_int_equal(dfu_dualbank_init(&dfu), 0);

    uint8_t image[64] = "FirmwareImagePayloadV2";
    uint32_t good_sig = 0x4F544131;
    assert_int_equal(dfu_dualbank_stage_image(&dfu, image, sizeof(image), 0x020000, good_sig), 0);
    assert_int_equal(dfu.state, DFU_STATE_TESTING);

    /* Fail health check -> rollback */
    assert_int_equal(dfu_dualbank_trigger_rollback(&dfu), 0);
    assert_int_equal(dfu.state, DFU_STATE_REVERTED);
    assert_int_equal(dfu.active_bank, DFU_BANK_A);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dfu_dualbank_init_null),
        cmocka_unit_test(test_dfu_dualbank_stage_confirm),
        cmocka_unit_test(test_dfu_dualbank_rollback),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
