#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "iems_southbound_adapter.h"

static void test_uart_device_path(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("UART", "/dev/ttyS1"), IEMS_BUS_UART);
}

static void test_uart_simulation_path_stays_uds(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("UART", "/tmp/vbus0.sock"), IEMS_BUS_UDS);
}

static void test_can_real_device_unsupported(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("CAN", "can0"), IEMS_BUS_UNSUPPORTED);
}

static void test_can_simulation_path_stays_uds(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("CAN", "/tmp/vbus1.sock"), IEMS_BUS_UDS);
    assert_int_equal(iems_southbound_transport_for("CAN", "vbus1.sock"), IEMS_BUS_UDS);
}

static void test_null_inputs_fall_back_to_uds(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for(NULL, NULL), IEMS_BUS_UDS);
}

static void test_empty_strings_fall_back_to_uds(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("", ""), IEMS_BUS_UDS);
}

static void test_type_is_case_insensitive(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("uart", "/dev/pts/3"), IEMS_BUS_UART);
}

static void test_tcp_url(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("TCP", "tcp://10.0.0.5:502"), IEMS_BUS_TCP);
}

static void test_tcp_path_wins_over_declared_type(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("UART", "tcp://10.0.0.5:502"), IEMS_BUS_TCP);
    assert_int_equal(iems_southbound_transport_for("CAN", "tcp://10.0.0.5:502"), IEMS_BUS_TCP);
}

static void test_tcp_without_scheme_stays_uds(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("TCP", "10.0.0.5:502"), IEMS_BUS_UDS);
    assert_int_equal(iems_southbound_transport_for("TCP", "/tmp/vbus0.sock"), IEMS_BUS_UDS);
}

static void test_tcp_type_is_case_insensitive(void **state) {
    (void)state;
    assert_int_equal(iems_southbound_transport_for("tcp", "tcp://10.0.0.5:502"), IEMS_BUS_TCP);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_uart_device_path),
        cmocka_unit_test(test_uart_simulation_path_stays_uds),
        cmocka_unit_test(test_can_real_device_unsupported),
        cmocka_unit_test(test_can_simulation_path_stays_uds),
        cmocka_unit_test(test_null_inputs_fall_back_to_uds),
        cmocka_unit_test(test_empty_strings_fall_back_to_uds),
        cmocka_unit_test(test_type_is_case_insensitive),
        cmocka_unit_test(test_tcp_url),
        cmocka_unit_test(test_tcp_path_wins_over_declared_type),
        cmocka_unit_test(test_tcp_without_scheme_stays_uds),
        cmocka_unit_test(test_tcp_type_is_case_insensitive),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
