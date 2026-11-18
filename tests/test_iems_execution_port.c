#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "iems_node.h"
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"

typedef struct {
    int calls;
    int bus_id;
    uint32_t tag_id;
    double value;
} fake_exec_t;

static int fake_write(void *ctx, int bus_id, uint32_t tag_id, double value, const char *proto) {
    fake_exec_t *exec = ctx;
    (void)proto;
    exec->calls++;
    exec->bus_id = bus_id;
    exec->tag_id = tag_id;
    exec->value = value;
    return 0;
}

static void test_write_goes_through_injected_port(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "IEMS_EXEC_TEST_%d", (int)getpid());
    iems_node_config_t cfg = {
        .product_name = "IEMS_TEST",
        .shm_name = shm_name,
        .datahub_capacity = 128,
    };
    iems_node_storage_t storage;
    iems_node_t *node = iems_node_create_in(&storage, &loop, &cfg);
    assert_non_null(node);

    fake_exec_t exec = {0};
    assert_int_equal(iems_node_set_execution_port(
                         node, (iems_execution_port_t){.ctx = &exec, .write_tag = fake_write}),
                     0);

    assert_int_equal(iems_node_write_tag(node, 3, 0x1234u, 42.5, "ModbusRTU"), 0);
    assert_int_equal(exec.calls, 1);
    assert_int_equal(exec.bus_id, 3);
    assert_int_equal(exec.tag_id, 0x1234u);
    assert_true(exec.value == 42.5);

    iems_node_destroy(node);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

static void test_reject_and_trip_write_zero(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "IEMS_EXEC_TRIP_%d", (int)getpid());
    iems_node_config_t cfg = {
        .product_name = "IEMS_TEST",
        .shm_name = shm_name,
        .datahub_capacity = 128,
    };
    iems_node_storage_t storage;
    iems_node_t *node = iems_node_create_in(&storage, &loop, &cfg);
    assert_non_null(node);

    fake_exec_t exec = {0};
    assert_int_equal(iems_node_set_execution_port(
                         node, (iems_execution_port_t){.ctx = &exec, .write_tag = fake_write}),
                     0);

    assert_int_equal(iems_node_emergency_trip(node, "test trip"), 0);
    assert_int_equal(exec.calls, 1);
    assert_int_equal(exec.tag_id, IEMS_TAG_PCS_CONTROL);
    assert_true(exec.value == 0.0);

    exec.calls = 0;
    int rc = iems_node_write_tag(node, 0, IEMS_TAG_PCS_CONTROL, 100.0, "ModbusRTU");
    assert_int_equal(rc, -2); /* fire interlock */
    assert_int_equal(exec.calls, 1);
    assert_int_equal(exec.tag_id, IEMS_TAG_PCS_CONTROL);
    assert_true(exec.value == 0.0);

    iems_node_destroy(node);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

/* 控制下发的点位翻译：2 寄存器点位必须把 length 编进原生标识，否则 FC16
 * 不可达；单寄存器保持 1；未知点位维持原样。 */
static void test_write_translation_carries_reg_count(void **state) {
    (void)state;
    uint32_t native = 0;
    double raw = 0.0;

    iems_exec_translate_write("ModbusRTU", IEMS_TAG_GRID_POWER, 305419896.0, &native, &raw);
    assert_int_equal((int)(native & 0xFFFFu), 12);
    assert_int_equal((int)(native >> 16), 2);
    assert_true(raw == 305419896.0);

    iems_exec_translate_write("ModbusRTU", IEMS_TAG_BATTERY_SOC, 55.0, &native, &raw);
    assert_int_equal((int)(native >> 16), 1);
    assert_true(raw == 55.0);

    iems_exec_translate_write("ModbusRTU", 999999u, 7.0, &native, &raw);
    assert_int_equal(native, 999999u);
    assert_true(raw == 7.0);

    /* 原生标识按协议取：CAN 换成 CAN ID，DLT645 换成 DI */
    iems_exec_translate_write("CAN", IEMS_TAG_BATTERY_SOC, 55.0, &native, &raw);
    assert_int_equal(native, 0x18FF50E5u);
    iems_exec_translate_write("DLT645", IEMS_TAG_GRID_POWER, 12.3, &native, &raw);
    assert_int_equal(native, 0x02030000u);
    assert_true(fabs(raw - 12.3) < 1e-9); /* 非 Modbus 不做点表标定 */
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_write_goes_through_injected_port),
        cmocka_unit_test(test_reject_and_trip_write_zero),
        cmocka_unit_test(test_write_translation_carries_reg_count),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
