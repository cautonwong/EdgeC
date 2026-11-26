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
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "iems_driver_adapter.h"
#include "iems_node.h"
#include "datahub_priv.h"

typedef struct {
    uv_loop_t loop;
    iems_node_storage_t storage;
    iems_node_t *node;
} test_fixture_t;

// 每个测试进程用独立 shm,避开上一轮残留的镜像内容
static char g_shm_name[64];

static int setup_node(void **state) {
    test_fixture_t *fix = calloc(1, sizeof(test_fixture_t));
    uv_loop_init(&fix->loop);
    snprintf(g_shm_name, sizeof(g_shm_name), "IEMS_TEST_SHM_%d", (int)getpid());

    iems_node_config_t cfg = {
        .product_name = "IEMS_TEST",
        .shm_name = g_shm_name,
        .datahub_capacity = 256,
        .max_pcs_power_kw = 500.0,
        .transformer_capacity_kva = 630.0,
        .demand_limit_kw = 500.0,
    };

    fix->node = iems_node_create_in(&fix->storage, &fix->loop, &cfg);
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

static int fake_write(void *ctx, int bus_id, uint32_t slave_id, uint32_t tag_id, double value,
                      const char *proto, iems_write_done_fn on_done, void *on_done_user) {
    (void)ctx;
    (void)bus_id;
    (void)slave_id;
    (void)tag_id;
    (void)value;
    (void)proto;
    (void)on_done;
    (void)on_done_user;
    return 0;
}

/* 7. 南向采样镜像写入口:写进去能读出来,非法入参被拒 */
static void test_iems_node_ingest_sample(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;
    datahub_t *hub = iems_node_get_datahub(node);
    assert_non_null(hub);

    // 未采集过的 tag:ts 还是 0（/tmp/edgec-worktree-iems/cmd/iems/iems_node.h 里写的不变量）
    pipe_msg_t before;
    assert_int_equal(datahub_read_shadow(hub, 42, &before), 0);
    assert_int_equal(before.ts, 0);

    assert_int_equal(iems_node_ingest_sample(node, 0, IEMS_TAG_VOLTAGE_PHASE_A, 231.7, uv_hrtime()),
                     0);

    pipe_msg_t out;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &out), 0);
    assert_int_equal(out.tag_id, IEMS_TAG_VOLTAGE_PHASE_A);
    assert_true(out.ts > 0);
    assert_float_equal(out.payload.val.f, 231.7, 0.001);

    assert_int_equal(iems_node_ingest_sample(NULL, 0, IEMS_TAG_VOLTAGE_PHASE_A, 1.0, 1), -1);
    assert_int_equal(iems_node_ingest_sample(node, 0, 0, 1.0, 1), -1);
    // 不在 tag 映射里且超出 shadow 容量的 tag:写入失败要能看出来
    assert_int_equal(iems_node_ingest_sample(node, 0, 9999, 1.0, 1), -1);
}

/* 8. 迟到的轮询采样不得覆盖更晚的控制写;采样确实更新时仍要生效 */
static void test_iems_node_stale_sample_does_not_overwrite_control(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;
    datahub_t *hub = iems_node_get_datahub(node);
    assert_non_null(hub);

    iems_execution_port_t port = {.ctx = NULL, .write_tag = fake_write};
    assert_int_equal(iems_node_set_execution_port(node, port), 0);

    // 控制写先落 shadow,ts 是写提交时刻
    assert_int_equal(
        iems_node_write_tag(node, 0, 1, 0, IEMS_TAG_VOLTAGE_PHASE_A, 220.0, "ModbusRTU", NULL,
                            NULL),
        0);
    pipe_msg_t after_write;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &after_write), 0);
    assert_float_equal(after_write.payload.val.f, 220.0, 0.001);
    assert_true(after_write.ts > 0);

    // 读帧在控制写之前就已编码发出,结果现在才回来:采样更旧,必须丢弃
    assert_int_equal(iems_node_ingest_sample(node, 0, IEMS_TAG_VOLTAGE_PHASE_A, 231.7,
                                             after_write.ts - 1),
                     0);
    pipe_msg_t stale;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &stale), 0);
    assert_float_equal(stale.payload.val.f, 220.0, 0.001);

    // 采样发生在控制写之后:新读数必须生效,守卫不能变成永久保控制值
    assert_int_equal(iems_node_ingest_sample(node, 0, IEMS_TAG_VOLTAGE_PHASE_A, 222.0,
                                             after_write.ts + 1),
                     0);
    pipe_msg_t fresh;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &fresh), 0);
    assert_float_equal(fresh.payload.val.f, 222.0, 0.001);
}

static pipe_msg_t _read_device_shadow(datahub_t *hub, uint32_t tag_id, uint32_t device_id) {
    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    assert_int_equal(datahub_read_shadow_device(hub, tag_id, device_id, &msg), 0);
    return msg;
}

/* P6-04b1 A2: 样本身份是 (device_id, tag_id)。同 tag 两台设备互不覆盖；
 * device 0 的 tag 级视图取更晚者；设备槽与视图各用自己的 ts 守卫。 */
static void test_iems_node_per_device_slots(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;
    datahub_t *hub = iems_node_get_datahub(node);
    assert_non_null(hub);

    const uint64_t t1 = 1000000;
    const uint64_t mid = t1 + 50;
    const uint64_t t2 = t1 + 100;
    const uint64_t t3 = t2 + 100;

    assert_int_equal(iems_node_ingest_sample(node, 1, IEMS_TAG_BATTERY_SOC, 11.1, t1), 0);
    assert_int_equal(iems_node_ingest_sample(node, 5, IEMS_TAG_BATTERY_SOC, 55.5, t2), 0);

    pipe_msg_t dev1 = _read_device_shadow(hub, IEMS_TAG_BATTERY_SOC, 1);
    assert_int_equal(dev1.device_id, 1);
    assert_float_equal(dev1.payload.val.f, 11.1, 0.001);
    pipe_msg_t dev5 = _read_device_shadow(hub, IEMS_TAG_BATTERY_SOC, 5);
    assert_int_equal(dev5.device_id, 5);
    assert_float_equal(dev5.payload.val.f, 55.5, 0.001);

    pipe_msg_t view;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_BATTERY_SOC, &view), 0);
    assert_int_equal(view.device_id, 0);
    assert_float_equal(view.payload.val.f, 55.5, 0.001);

    /* 比设备 1 自己更旧的采样丢弃：设备槽和视图都不动 */
    assert_int_equal(iems_node_ingest_sample(node, 1, IEMS_TAG_BATTERY_SOC, 9.9, t1 - 1), 0);
    dev1 = _read_device_shadow(hub, IEMS_TAG_BATTERY_SOC, 1);
    assert_float_equal(dev1.payload.val.f, 11.1, 0.001);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_BATTERY_SOC, &view), 0);
    assert_float_equal(view.payload.val.f, 55.5, 0.001);

    /* 设备 1 的新采样仍早于视图（设备 5 的 t2）：设备槽要更新，视图保持更晚者。
     * 守卫按设备判新旧，不能拿全局视图把设备自己的新采样丢掉。 */
    assert_int_equal(iems_node_ingest_sample(node, 1, IEMS_TAG_BATTERY_SOC, 33.3, mid), 0);
    dev1 = _read_device_shadow(hub, IEMS_TAG_BATTERY_SOC, 1);
    assert_float_equal(dev1.payload.val.f, 33.3, 0.001);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_BATTERY_SOC, &view), 0);
    assert_float_equal(view.payload.val.f, 55.5, 0.001);

    /* 更晚的采样同时更新设备槽与视图 */
    assert_int_equal(iems_node_ingest_sample(node, 1, IEMS_TAG_BATTERY_SOC, 22.2, t3), 0);
    dev1 = _read_device_shadow(hub, IEMS_TAG_BATTERY_SOC, 1);
    assert_float_equal(dev1.payload.val.f, 22.2, 0.001);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_BATTERY_SOC, &view), 0);
    assert_float_equal(view.payload.val.f, 22.2, 0.001);
}

/* P6-04b1 A3: 控制写先落目标设备影子（无设备守卫），再发布视图；
 * 轮询结果按 (device, tag) 的时间戳丢弃更旧的采样。 */
static void test_iems_node_control_write_lands_in_device_slot(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;
    datahub_t *hub = iems_node_get_datahub(node);
    assert_non_null(hub);

    iems_execution_port_t port = {.ctx = NULL, .write_tag = fake_write};
    assert_int_equal(iems_node_set_execution_port(node, port), 0);

    assert_int_equal(iems_node_write_tag(node, 0, 1, 1, IEMS_TAG_VOLTAGE_PHASE_A, 220.0,
                                         "ModbusRTU", NULL, NULL),
                     0);
    pipe_msg_t dev1 = _read_device_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, 1);
    assert_int_equal(dev1.device_id, 1);
    assert_float_equal(dev1.payload.val.f, 220.0, 0.001);
    pipe_msg_t view;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &view), 0);
    assert_float_equal(view.payload.val.f, 220.0, 0.001);
    const uint64_t write_ts = dev1.ts;

    /* 读帧在控制写之前编码、结果晚到：采样更旧，设备槽和视图都不覆盖 */
    assert_int_equal(
        iems_node_ingest_sample(node, 1, IEMS_TAG_VOLTAGE_PHASE_A, 111.0, write_ts - 1), 0);
    dev1 = _read_device_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, 1);
    assert_float_equal(dev1.payload.val.f, 220.0, 0.001);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &view), 0);
    assert_float_equal(view.payload.val.f, 220.0, 0.001);

    /* 另一台设备的新采样只改自己和视图，控制写落的设备槽不动 */
    assert_int_equal(
        iems_node_ingest_sample(node, 5, IEMS_TAG_VOLTAGE_PHASE_A, 555.0, write_ts + 100), 0);
    pipe_msg_t dev5 = _read_device_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, 5);
    assert_int_equal(dev5.device_id, 5);
    assert_float_equal(dev5.payload.val.f, 555.0, 0.001);
    dev1 = _read_device_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, 1);
    assert_float_equal(dev1.payload.val.f, 220.0, 0.001);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &view), 0);
    assert_float_equal(view.payload.val.f, 555.0, 0.001);

    /* 设备 1 自己的新采样（仍早于视图）只更新设备槽：守卫按设备，不按视图 */
    assert_int_equal(
        iems_node_ingest_sample(node, 1, IEMS_TAG_VOLTAGE_PHASE_A, 222.0, write_ts + 50), 0);
    dev1 = _read_device_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, 1);
    assert_float_equal(dev1.payload.val.f, 222.0, 0.001);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &view), 0);
    assert_float_equal(view.payload.val.f, 555.0, 0.001);
}

typedef struct {
    int count;
    pipe_msg_t last;
} view_event_count_t;

static void _count_view_event(datahub_t *hub, const pipe_msg_t *msg, void *user_ctx) {
    (void)hub;
    view_event_count_t *c = (view_event_count_t *)user_ctx;
    c->count++;
    c->last = *msg;
}

/* P6-04b1 A5: 设备样本只让 tag 级读者收到一条视图事件；per-device 影子不进 ring。
 * app host 订阅用 device_id = 0，和这里验证的 filter 一致。 */
static void test_iems_node_device_sample_emits_single_view_event(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;
    datahub_t *hub = iems_node_get_datahub(node);
    assert_non_null(hub);

    datahub_sub_filter_t filter = {
        .tag_id = IEMS_TAG_VOLTAGE_PHASE_A,
        .tag_mask = 0,
        .domain_mask = 0,
        .device_id = 0,
        .notify_on_change_only = false,
        .deadband = 0.0,
    };
    view_event_count_t events = {0};
    int sub_id = datahub_subscribe_filtered(hub, &filter, _count_view_event, &events);
    assert_true(sub_id > 0);

    uint64_t ring_seq = datahub_get_ring_seq(hub);
    assert_int_equal(
        iems_node_ingest_sample(node, 2, IEMS_TAG_VOLTAGE_PHASE_A, 231.7, uv_hrtime()), 0);

    assert_int_equal(events.count, 1);
    assert_int_equal(events.last.device_id, 0);
    assert_int_equal(events.last.tag_id, IEMS_TAG_VOLTAGE_PHASE_A);
    assert_float_equal(events.last.payload.val.f, 231.7, 0.001);

    /* 设备自己的槽拿到了带身份的样本 */
    pipe_msg_t dev = _read_device_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, 2);
    assert_int_equal(dev.device_id, 2);
    assert_float_equal(dev.payload.val.f, 231.7, 0.001);

    /* ring 里只有一条 device 0 的视图消息（per-device 影子不写 ring） */
    pipe_msg_t ring_msg;
    assert_int_equal(datahub_read_ring(hub, &ring_seq, &ring_msg), 0);
    assert_int_equal(ring_msg.device_id, 0);
    assert_int_equal(ring_msg.tag_id, IEMS_TAG_VOLTAGE_PHASE_A);
    assert_float_equal(ring_msg.payload.val.f, 231.7, 0.001);
    assert_int_equal(datahub_read_ring(hub, &ring_seq, &ring_msg), -1);

    assert_int_equal(datahub_unsubscribe(hub, sub_id), 0);
}

/* P6-04b1 A6: 设备槽越界必须失败且不写视图；空参数/0 tag 行为不变。 */
static void test_iems_node_device_slot_failure_is_visible(void **state) {
    test_fixture_t *fix = (test_fixture_t *)*state;
    iems_node_t *node = fix->node;
    datahub_t *hub = iems_node_get_datahub(node);
    assert_non_null(hub);

    pipe_msg_t view;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_BATTERY_SOC, &view), 0);
    assert_int_equal(view.ts, 0);

    /* capacity 256：device 20 的槽 20*16+dense 越界，写失败可见，视图不写 */
    assert_int_equal(iems_node_ingest_sample(node, 20, IEMS_TAG_BATTERY_SOC, 1.5, 1000), -1);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_BATTERY_SOC, &view), 0);
    assert_int_equal(view.ts, 0);

    assert_int_equal(iems_node_ingest_sample(NULL, 1, IEMS_TAG_BATTERY_SOC, 1.0, 1), -1);
    assert_int_equal(iems_node_ingest_sample(node, 1, 0, 1.0, 1), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_iems_node_assembly, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_dispatch_normal, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_soc_boundaries, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_bms_invariants, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_fire_safety_trip, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_control_loop_latency_watchdog, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_ingest_sample, setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_stale_sample_does_not_overwrite_control,
                                        setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_per_device_slots, setup_node,
                                        teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_control_write_lands_in_device_slot,
                                        setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_device_sample_emits_single_view_event,
                                        setup_node, teardown_node),
        cmocka_unit_test_setup_teardown(test_iems_node_device_slot_failure_is_visible, setup_node,
                                        teardown_node),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
