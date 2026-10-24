/**
 * @file test_iems_node.c
 * @brief IEMS 商业储能聚合根与核心业务领域模型单元测试 (No-Contract)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "iems_driver_adapter.h"
#include "iems_node.h"

typedef struct {
    uv_loop_t loop;
    iems_node_t *node;
} test_fixture_t;

static int setup_node(void **state) {
    test_fixture_t *fix = calloc(1, sizeof(test_fixture_t));
    uv_loop_init(&fix->loop);

    iems_node_config_t cfg = {
        .product_name = "IEMS_TEST",
        .shm_name = "IEMS_TEST_SHM",
        .datahub_capacity = 256,
        .max_pcs_power_kw = 500.0,
        .transformer_capacity_kva = 630.0,
        .demand_limit_kw = 500.0,
    };

    fix->node = iems_node_create(&fix->loop, &cfg);
    assert_non_null(fix->node);

    int rc = iems_node_start(fix->node);
    assert_int_equal(rc, 0);

    *state = fix;
    return 0;
}

static int teardown_node(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    if (fix) {
        if (fix->node) {
            iems_node_destroy(fix->node);
        }
        uv_run(&fix->loop, UV_RUN_NOWAIT);
        uv_loop_close(&fix->loop);
        free(fix);
    }
    return 0;
}

/* 1. 验证聚合根创建、子聚合装配完整性 */
static void test_iems_node_assembly(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;

    assert_non_null(iems_node_get_bms(node));
    assert_non_null(iems_node_get_pcs(node));
    assert_non_null(iems_node_get_coordinator(node));
    assert_non_null(iems_node_get_fire_safety(node));
    assert_non_null(iems_node_get_pcs_dispatcher(node));
    assert_non_null(iems_node_get_datahub(node));
    assert_non_null(iems_node_get_container(node));
}

/* 2. 验证常规功率调度与多 PCS 分摊 */
static void test_iems_node_dispatch_normal(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;

    // 先上报健康 BMS 状态 (SOC 50%, 无告警)
    int rc = iems_node_update_bms_rack(node, 1, 750.0, 10.0, 50.0, 28.0);
    assert_int_equal(rc, 0);

    double actual_kw = 0.0;
    rc = iems_node_dispatch_power(node, 200.0, &actual_kw);
    assert_int_equal(rc, 0);
    assert_float_equal(actual_kw, 200.0, 0.001);

    // 检查 DataHub 影子寄存器
    datahub_t *hub = iems_node_get_datahub(node);
    pipe_msg_t shadow_msg;
    rc = datahub_read_shadow(hub, IEMS_TAG_PCS_CONTROL, &shadow_msg);
    assert_int_equal(rc, 0);
    assert_float_equal(shadow_msg.payload.val.f, 200.0, 0.001);
}

/* 3. 验证充放电 SOC 边界保护 */
static void test_iems_node_soc_boundaries(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;

    // 充满态 (SOC 99%)
    iems_node_update_bms_rack(node, 1, 780.0, 0.0, 99.0, 30.0);
    double actual_kw = 0.0;
    // 尝试继续充电 (-100 kW)，应当被约束截断为 0 kW
    int rc = iems_node_dispatch_power(node, -100.0, &actual_kw);
    assert_int_equal(rc, 0);
    assert_float_equal(actual_kw, 0.0, 0.001);

    // 放电应当正常允许
    rc = iems_node_dispatch_power(node, 100.0, &actual_kw);
    assert_int_equal(rc, 0);
    assert_float_equal(actual_kw, 100.0, 0.001);

    // 放空态 (SOC 3%)
    iems_node_update_bms_rack(node, 1, 620.0, 0.0, 3.0, 25.0);
    // 尝试继续放电 (100 kW)，应当被截断为 0 kW
    rc = iems_node_dispatch_power(node, 100.0, &actual_kw);
    assert_int_equal(rc, 0);
    assert_float_equal(actual_kw, 0.0, 0.001);

    // 充电应当正常允许
    rc = iems_node_dispatch_power(node, -100.0, &actual_kw);
    assert_int_equal(rc, 0);
    assert_float_equal(actual_kw, -100.0, 0.001);
}

/* 4. 验证 BMS 二级降额与三级热失控闭锁不变量 */
static void test_iems_node_bms_invariants(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;

    // 触发二级告警 (Temp = 48.0°C > 45.0°C: 降半功率)
    iems_node_update_bms_rack(node, 1, 750.0, 10.0, 60.0, 48.0);
    double actual_kw = 0.0;
    int rc = iems_node_dispatch_power(node, 200.0, &actual_kw);
    assert_int_equal(rc, 0);
    assert_float_equal(actual_kw, 100.0, 0.001);

    // 触发三级告警 (Temp = 58.0°C > 55.0°C: 热失控严重告警，彻底闭锁停机)
    iems_node_update_bms_rack(node, 1, 750.0, 10.0, 60.0, 58.0);
    rc = iems_node_dispatch_power(node, 200.0, &actual_kw);
    assert_int_equal(rc, -3);
    assert_float_equal(actual_kw, 0.0, 0.001);
}

/* 5. 验证消防安全联动闭锁与紧急停机 */
static void test_iems_node_fire_safety_trip(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;

    // 正常状态
    iems_node_update_bms_rack(node, 1, 750.0, 0.0, 50.0, 25.0);
    double actual_kw = 0.0;
    assert_int_equal(iems_node_dispatch_power(node, 150.0, &actual_kw), 0);
    assert_float_equal(actual_kw, 150.0, 0.001);

    // 触发紧急停机
    int rc = iems_node_emergency_trip(node, "SMOKE_DETECTED_ZONE_1");
    assert_int_equal(rc, 0);

    // 停机后再次尝试调度功率必须被拒绝 (-2)
    rc = iems_node_dispatch_power(node, 100.0, &actual_kw);
    assert_int_equal(rc, -2);
    assert_float_equal(actual_kw, 0.0, 0.001);

    // 验证 DataHub PCS 状态归零
    datahub_t *hub = iems_node_get_datahub(node);
    pipe_msg_t msg;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_PCS_CONTROL, &msg), 0);
    assert_float_equal(msg.payload.val.f, 0.0, 0.001);
}

/* 6. 验证控制环路延迟统计与看门狗超时降级 (AEP-407) */
static void test_iems_node_control_loop_latency_watchdog(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;

    // 1. 验证默认看门狗配置 (10ms = 10000us)
    iems_loop_watchdog_stats_t stats;
    assert_int_equal(iems_node_get_watchdog_stats(node, &stats), 0);
    assert_int_equal(stats.watchdog_budget_us, 10000);
    assert_false(stats.watchdog_degraded);
    assert_int_equal(stats.watchdog_trips, 0);

    // 2. 正常工况下连续调度并度量延迟分布 (50 轮循环)
    iems_node_update_bms_rack(node, 1, 750.0, 0.0, 60.0, 25.0);
    for (int i = 0; i < 50; i++) {
        double actual_kw = 0.0;
        int rc = iems_node_dispatch_power(node, 100.0 + (double)i, &actual_kw);
        assert_int_equal(rc, 0);
        assert_float_equal(actual_kw, (float)(100.0 + (double)i), 0.001f);
    }

    assert_int_equal(iems_node_get_watchdog_stats(node, &stats), 0);
    assert_false(stats.watchdog_degraded);
    assert_int_equal(stats.watchdog_trips, 0);
    assert_true(stats.max_cycle_us < 10000);  // 远低于 10ms 工业安全预算

    // 3. 注入看门狗超限条件 (budget = 0 强制触发超时)
    assert_int_equal(iems_node_set_watchdog_budget(node, 0), 0);
    double degraded_kw = -1.0;
    int rc = iems_node_dispatch_power(node, 50.0, &degraded_kw);
    // 第一步超限完成并触发看门狗置位
    assert_int_equal(iems_node_get_watchdog_stats(node, &stats), 0);
    assert_true(stats.watchdog_degraded);
    assert_true(stats.watchdog_trips >= 1);

    // 4. 处于超时降级状态时，后续写调度必须被拒绝 (-4) 并强制零功率
    rc = iems_node_dispatch_power(node, 80.0, &degraded_kw);
    assert_int_equal(rc, -4);
    assert_float_equal(degraded_kw, 0.0, 0.001);

    // 5. 恢复安全预算并重置降级状态
    assert_int_equal(iems_node_set_watchdog_budget(node, 10000), 0);
    assert_int_equal(iems_node_reset_watchdog_degrade(node), 0);
    assert_int_equal(iems_node_get_watchdog_stats(node, &stats), 0);
    assert_false(stats.watchdog_degraded);

    // 恢复后可正常调度
    rc = iems_node_dispatch_power(node, 60.0, &degraded_kw);
    assert_int_equal(rc, 0);
    assert_float_equal(degraded_kw, 60.0, 0.001);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_iems_node_assembly, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_dispatch_normal, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_soc_boundaries, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_bms_invariants, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_fire_safety_trip, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_control_loop_latency_watchdog, setup_node, teardown_node),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
