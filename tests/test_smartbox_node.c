/**
 * @file test_smartbox_node.c
 * @brief SmartBox Edge Controller Aggregate Root Unit Tests (No-Contract)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "smartbox_driver_adapter.h"
#include "smartbox_node.h"

typedef struct {
    uv_loop_t loop;
    smartbox_node_t *node;
} test_fixture_t;

static int setup_node(void **state) {
    test_fixture_t *fix = (test_fixture_t *)calloc(1, sizeof(test_fixture_t));
    uv_loop_init(&fix->loop);

    smartbox_node_config_t cfg = {
        .product_name = "smart_box_test",
        .shm_name = "smart_box_test_shm",
        .datahub_capacity = 256,
        .transformer_rated_kva = 630.0,
    };

    fix->node = smartbox_node_create(&fix->loop, &cfg);
    assert_non_null(fix->node);

    int rc = smartbox_node_start(fix->node);
    assert_int_equal(rc, 0);

    *state = fix;
    return 0;
}

static int teardown_node(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    if (fix) {
        if (fix->node) {
            smartbox_node_destroy(fix->node);
        }
        uv_run(&fix->loop, UV_RUN_NOWAIT);
        uv_loop_close(&fix->loop);
        free(fix);
    }
    return 0;
}

/* 1. 验证聚合根创建、子聚合装配完整性 */
static void test_smartbox_node_assembly(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    smartbox_node_t *node = fix->node;

    assert_non_null(smartbox_node_get_thermal_model(node));
    assert_non_null(smartbox_node_get_switchgear_bay(node));
    assert_non_null(smartbox_node_get_datahub(node));
    assert_non_null(smartbox_node_get_container(node));
}

/* 2. 验证变压器热寿命评估模型与状态同步 */
static void test_smartbox_node_thermal_evaluation(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    smartbox_node_t *node = fix->node;
    smartbox_thermal_model_t *model = smartbox_node_get_thermal_model(node);
    datahub_t *hub = smartbox_node_get_datahub(node);

    /* 额定满载基准测试 (Load=630kVA, TopOil=80℃, Ambient=30℃) */
    int rc = smartbox_node_evaluate_thermal(node, 30.0, 80.0, 630.0, 3600.0);
    assert_int_equal(rc, 0);
    assert_float_equal(model->hotspot_temp_c, 110.0, 0.01);
    assert_false(model->is_thermal_alarm);

    pipe_msg_t msg;
    rc = datahub_read_shadow(hub, TAG_BOX_SYSTEM_STATUS, &msg);
    assert_int_equal(rc, 0);
    assert_int_equal((int)msg.payload.val.f, 1);

    /* 重载热过载报警测试 (Load=945kVA, TopOil=95℃, Ambient=35℃) */
    rc = smartbox_node_evaluate_thermal(node, 35.0, 95.0, 945.0, 3600.0);
    assert_int_equal(rc, 0);
    assert_true(model->hotspot_temp_c > 140.0);
    assert_true(model->is_thermal_alarm);

    rc = datahub_read_shadow(hub, TAG_BOX_SYSTEM_STATUS, &msg);
    assert_int_equal(rc, 0);
    assert_int_equal((int)msg.payload.val.f, 2);
}

/* 3. 验证电气五防智能闭锁操作拦截与合法流转 */
static void test_smartbox_node_five_safety_interlock(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    smartbox_node_t *node = fix->node;
    smartbox_switchgear_bay_t *bay = smartbox_node_get_switchgear_bay(node);

    const char *reason = NULL;

    /* 初始态：检修状态，接地刀闸闭合，柜门关闭，线路无电 */
    bay->is_earth_switch_closed = true;
    bay->is_line_charged = false;
    bay->is_busbar_charged = false;
    bay->is_cubicle_door_open = false;
    bay->is_bus_isolator_closed = true;
    bay->is_line_isolator_closed = true;

    /* 防误一：禁止带接地线(接地刀闸)合断路器 */
    int rc = smartbox_node_operate_breaker(node, true, &reason);
    assert_true(rc < 0);
    assert_false(bay->is_breaker_closed);
    assert_non_null(reason);

    /* 分接地刀闸 */
    rc = smartbox_node_operate_earth_switch(node, false, &reason);
    assert_int_equal(rc, 0);
    assert_false(bay->is_earth_switch_closed);

    /* 此时可合断路器 */
    rc = smartbox_node_operate_breaker(node, true, &reason);
    assert_int_equal(rc, 0);
    assert_true(bay->is_breaker_closed);

    /* 防误二：断路器合闸送电状态下，禁止带电合接地刀闸 */
    rc = smartbox_node_operate_earth_switch(node, true, &reason);
    assert_true(rc < 0);
    assert_false(bay->is_earth_switch_closed);

    /* 分断路器恢复安全状态 */
    rc = smartbox_node_operate_breaker(node, false, &reason);
    assert_int_equal(rc, 0);
    assert_false(bay->is_breaker_closed);

    /* 分隔离开关 (五防规程: 合接地刀闸前断路器和隔离开关均须分闸) */
    bay->is_bus_isolator_closed = false;
    bay->is_line_isolator_closed = false;

    /* 再次合接地刀闸成功 */
    rc = smartbox_node_operate_earth_switch(node, true, &reason);
    assert_int_equal(rc, 0);
    assert_true(bay->is_earth_switch_closed);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_smartbox_node_assembly, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_smartbox_node_thermal_evaluation, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_smartbox_node_five_safety_interlock, setup_node, teardown_node),
    };

    printf("============================================\n");
    printf("   SmartBox Aggregate Root Tests (No-Contract)\n");
    printf("============================================\n");
    return cmocka_run_group_tests(tests, NULL, NULL);
}
