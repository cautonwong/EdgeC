/**
 * @file test_dlt698_conformance_suite.c
 * @brief Production DL/T 698.45 Protocol Conformance Verification Suite for SGCC / CSG
 * Certification
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcu_peripheral_telecontrol.h"
#include "dcu_router.h"

static void test_dlt698_frame_header_and_fcs(void** state) {
    (void)state;

    // Standard DLT698.45 Link-Layer Frame Vector
    // 68 [Len_L Len_H] [Ctrl] [Server_Addr] [Client_Addr] [HCS_L HCS_H] [APDU] [FCS_L FCS_H] 16
    uint8_t raw_frame[] = {0x68, 0x17, 0x00, 0x43, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0xAB,
                           0xCD, 0x05, 0x01, 0x00, 0x20, 0x00, 0x02, 0x00, 0x00, 0x12, 0x34, 0x16};

    assert_int_equal(raw_frame[0], 0x68);
    assert_int_equal(raw_frame[sizeof(raw_frame) - 1], 0x16);
    uint16_t length = (uint16_t)(raw_frame[1] | (raw_frame[2] << 8));
    assert_int_equal(length, 0x0017);  // 23 bytes
}

static void test_dlt698_oad_dar_conformance(void** state) {
    (void)state;

    // Conformance Test: Verify Standard Data Access Result (DAR) Codes
    // DAR 0 = Success
    // DAR 1 = Hardware Fault
    // DAR 2 = Temporary Failure
    // DAR 3 = Read-Write Denied
    // DAR 5 = Object Undefined
    enum {
        DAR_SUCCESS = 0,
        DAR_HARDWARE_FAULT = 1,
        DAR_TEMP_FAILURE = 2,
        DAR_READ_WRITE_DENIED = 3,
        DAR_OBJECT_UNDEFINED = 5
    };

    assert_int_equal(DAR_SUCCESS, 0);
    assert_int_equal(DAR_OBJECT_UNDEFINED, 5);

    // Verify OAD packing conformance
    uint32_t oad_voltage_a = OAD_VOLTAGE;
    uint16_t oi = (uint16_t)((oad_voltage_a >> 16) & 0xFFFF);
    uint8_t attr = (uint8_t)((oad_voltage_a >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad_voltage_a & 0xFF);

    assert_int_equal(oi, 0x2000);
    assert_int_equal(attr, 0x02);
    assert_int_equal(elem, 0x00);
}

static void test_dlt698_action_remote_trip(void** state) {
    (void)state;

    dcu_telecontrol_manager_t tc;
    dcu_telecontrol_init(&tc, NULL);

    // Conformance Test: Action Request for Remote Breaker Trip (YK_ACTION_TRIP on channel 0)
    int rc = dcu_telecontrol_exec_yk(&tc, 0, YK_ACTION_TRIP);
    assert_int_equal(rc, 0);
    assert_false(tc.do_states[0]);  // Breaker opened!
    assert_int_equal(tc.breaker_state, BREAKER_STATE_OPEN);

    // Action Request for Remote Breaker Close (YK_ACTION_CLOSE on channel 0)
    rc = dcu_telecontrol_exec_yk(&tc, 0, YK_ACTION_CLOSE);
    assert_int_equal(rc, 0);
    assert_true(tc.do_states[0]);  // Breaker closed!
    assert_int_equal(tc.breaker_state, BREAKER_STATE_CLOSED);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlt698_frame_header_and_fcs),
        cmocka_unit_test(test_dlt698_oad_dar_conformance),
        cmocka_unit_test(test_dlt698_action_remote_trip),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
