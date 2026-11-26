#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter_types.h"
#include "dlt645_codec.h"
#include "iems_node.h"
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"

typedef struct {
    int calls;
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    double value;
    iems_write_done_fn on_done;
    void *on_done_user;
} fake_exec_t;

static int fake_write(void *ctx, int bus_id, uint32_t slave_id, uint32_t tag_id, double value,
                      const char *proto, iems_write_done_fn on_done, void *on_done_user) {
    fake_exec_t *exec = ctx;
    (void)proto;
    exec->calls++;
    exec->bus_id = bus_id;
    exec->slave_id = slave_id;
    exec->tag_id = tag_id;
    exec->value = value;
    exec->on_done = on_done;
    exec->on_done_user = on_done_user;
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

    assert_int_equal(iems_node_write_tag(node, 3, 7, 0, 0x1234u, 42.5, "ModbusRTU", NULL, NULL), 0);
    assert_int_equal(exec.calls, 1);
    assert_int_equal(exec.bus_id, 3);
    assert_int_equal(exec.slave_id, 7);
    assert_int_equal(exec.tag_id, 0x1234u);
    assert_true(exec.value == 42.5);
    assert_null(exec.on_done);

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
    assert_int_equal(exec.bus_id, 0);
    assert_int_equal(exec.slave_id, 1); /* 安全跳闸沿用 PCS 既有接线 */
    assert_int_equal(exec.tag_id, IEMS_TAG_PCS_CONTROL);
    assert_true(exec.value == 0.0);

    exec.calls = 0;
    int rc = iems_node_write_tag(node, 0, 1, 0, IEMS_TAG_PCS_CONTROL, 100.0, "ModbusRTU", NULL,
                                 NULL);
    assert_int_equal(rc, -2); /* fire interlock */
    assert_int_equal(exec.calls, 1);
    assert_int_equal(exec.tag_id, IEMS_TAG_PCS_CONTROL);
    assert_true(exec.value == 0.0);

    iems_node_destroy(node);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

/* 控制下发的点位翻译：2 寄存器点位必须把 length 编进原生标识，否则 FC16
 * 不可达；单寄存器保持 1；未知点位维持原样。DLT645 写必须带 2/4 字节宽度
 * 描述，写值按点表 scaler 逆向换算。 */
static void test_write_translation_carries_reg_count(void **state) {
    (void)state;
    uint32_t native = 0;
    double raw = 0.0;
    const void *desc = NULL;
    size_t desc_len = 0;

    assert_int_equal(iems_exec_translate_write("ModbusRTU", IEMS_TAG_GRID_POWER, 305419896.0,
                                               &native, &raw, &desc, &desc_len),
                     0);
    assert_int_equal((int)(native & 0xFFFFu), 12);
    assert_int_equal((int)(native >> 16), 2);
    assert_true(raw == 305419896.0);
    assert_null(desc);
    assert_int_equal(desc_len, 0);

    assert_int_equal(iems_exec_translate_write("ModbusRTU", IEMS_TAG_BATTERY_SOC, 55.0, &native,
                                               &raw, &desc, &desc_len),
                     0);
    assert_int_equal((int)(native >> 16), 1);
    assert_true(raw == 55.0);

    assert_int_equal(iems_exec_translate_write("ModbusRTU", 999999u, 7.0, &native, &raw, &desc,
                                               &desc_len),
                     0);
    assert_int_equal(native, 999999u);
    assert_true(raw == 7.0);

    /* 原生标识按协议取：CAN 换成 CAN ID，DLT645 换成 DI */
    assert_int_equal(iems_exec_translate_write("CAN", IEMS_TAG_BATTERY_SOC, 55.0, &native, &raw,
                                               &desc, &desc_len),
                     0);
    assert_int_equal(native, 0x18FF50E5u);
    assert_null(desc);

    /* DLT645：220.1 V -> raw 2201（scaler 0.1），宽度 2 字节 */
    assert_int_equal(iems_exec_translate_write("DLT645", IEMS_TAG_VOLTAGE_PHASE_A, 220.1, &native,
                                               &raw, &desc, &desc_len),
                     0);
    assert_int_equal(native, 0x02010100u);
    assert_true(raw == 2201.0);
    assert_non_null(desc);
    assert_int_equal(desc_len, sizeof(dlt645_write_desc_t));
    assert_int_equal(((const dlt645_write_desc_t *)desc)->data_len, 2);

    /* 电能量点位宽度 4 字节 */
    assert_int_equal(iems_exec_translate_write("DLT645", IEMS_TAG_ENERGY_ACTIVE_IMP, 12.34, &native,
                                               &raw, &desc, &desc_len),
                     0);
    assert_true(raw == 1234.0);
    assert_int_equal(((const dlt645_write_desc_t *)desc)->data_len, 4);

    /* 宽度 3 字节的点位驱动不支持：提交前拒绝，不把非法宽度传给驱动 */
    assert_int_equal(iems_exec_translate_write("DLT645", IEMS_TAG_GRID_POWER, 12.3, &native, &raw,
                                               &desc, &desc_len),
                     -1);
}

typedef struct {
    int calls;
    iems_write_done_status_t status;
} done_probe_t;

static void probe_done(void *user, iems_write_done_status_t status) {
    done_probe_t *probe = user;
    probe->calls++;
    probe->status = status;
}

/* 域端口透传：iems_node_write_tag 的可选完成回调原样交给执行端口，
 * 端口回调的结果回到调用方；不带回调时端口收到 NULL（旧行为不变）。 */
static void test_node_passes_completion_callback_to_port(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "IEMS_EXEC_DONE_%d", (int)getpid());
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

    done_probe_t probe = {0};
    assert_int_equal(
        iems_node_write_tag(node, 3, 7, 0, 0x1234u, 42.5, "ModbusRTU", probe_done, &probe), 0);
    assert_int_equal(exec.calls, 1);
    assert_non_null(exec.on_done);
    assert_ptr_equal(exec.on_done_user, &probe);

    /* 请求层回调触发时，状态与用户指针原样回到调用方 */
    exec.on_done(exec.on_done_user, IEMS_WRITE_DONE_TIMEOUT);
    assert_int_equal(probe.calls, 1);
    assert_int_equal(probe.status, IEMS_WRITE_DONE_TIMEOUT);

    /* 不带回调：端口拿到 NULL，不登记结果 */
    assert_int_equal(iems_node_write_tag(node, 3, 7, 0, 0x1234u, 1.0, "ModbusRTU", NULL, NULL), 0);
    assert_int_equal(exec.calls, 2);
    assert_null(exec.on_done);
    assert_null(exec.on_done_user);

    iems_node_destroy(node);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

/* 总线状态 → 域三态：只有 TIMEOUT 单独区分，其余非零都按失败。 */
static void test_bus_status_maps_to_domain_status(void **state) {
    (void)state;
    assert_int_equal(iems_exec_map_write_status(CMD_STATUS_SUCCESS), IEMS_WRITE_DONE_OK);
    assert_int_equal(iems_exec_map_write_status(CMD_STATUS_TIMEOUT), IEMS_WRITE_DONE_TIMEOUT);
    assert_int_equal(iems_exec_map_write_status(CMD_STATUS_IO_ERROR), IEMS_WRITE_DONE_FAILED);
    assert_int_equal(iems_exec_map_write_status(CMD_STATUS_PROTO_ERROR), IEMS_WRITE_DONE_FAILED);
    assert_int_equal(iems_exec_map_write_status(CMD_STATUS_QUEUE_FULL), IEMS_WRITE_DONE_FAILED);
    assert_int_equal(iems_exec_map_write_status(CMD_STATUS_RATE_LIMITED), IEMS_WRITE_DONE_FAILED);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_write_goes_through_injected_port),
        cmocka_unit_test(test_reject_and_trip_write_zero),
        cmocka_unit_test(test_write_translation_carries_reg_count),
        cmocka_unit_test(test_node_passes_completion_callback_to_port),
        cmocka_unit_test(test_bus_status_maps_to_domain_status),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
