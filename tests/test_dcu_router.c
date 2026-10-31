#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uv.h>

#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "shmring.h"

static int _mock_count_freezes(void* ctx, uint16_t* out_count) {
    return dcu_storage_count_freezes((dcu_storage_t*)ctx, out_count);
}

static int _mock_read_shadow(void* ctx, uint32_t tag_id, uint32_t device_id, void* out_msg) {
    (void)ctx;
    (void)device_id;
    pipe_msg_t* msg = (pipe_msg_t*)out_msg;
    if (tag_id == OAD_VOLTAGE || tag_id == 1001) {  // 电压
        pipe_msg_init_scalar(msg, OAD_VOLTAGE, 220.5, uv_hrtime());
        return 0;
    } else if (tag_id == OAD_CURRENT || tag_id == 1002) {  // 电流
        pipe_msg_init_scalar(msg, OAD_CURRENT, 4.95, uv_hrtime());
        return 0;
    }
    return -1;
}

static int _mock_publish_realtime(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                                  uint64_t timestamp_ns) {
    (void)ctx;
    (void)tag_id;
    (void)meter_idx;
    (void)val;
    (void)timestamp_ns;
    return 0;
}

// 验证对象字典与属性/方法路由的测试用例
static void test_dcu_router_dispatch(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    // 1. 初始化底层的档案、调度器与持久化存储服务
    dcu_inventory_t test_inv;
    dcu_inventory_init(&test_inv);

    dcu_poller_t poller;
    dcu_poller_init(&poller);

    dcu_storage_t storage;
    int ret = dcu_storage_init(&storage, loop, ":memory:");
    assert_int_equal(ret, 0);

    router_sink_ops_t router_sink = {.ctx = &storage,
                                     .record_telemetry = (void*)dcu_storage_write_telemetry,
                                     .count_freezes = _mock_count_freezes};

    router_bus_ops_t router_bus = {.ctx = NULL,
                                   .msg_size = sizeof(pipe_msg_t),
                                   .publish_realtime = _mock_publish_realtime,
                                   .read_shadow = _mock_read_shadow};

    // 2. 初始化核心对象字典路由器
    ret = dcu_router_init_ex(&test_inv, &poller, router_sink, NULL, NULL, NULL, NULL,
                             router_bus);
    assert_int_equal(ret, 0);

    // 3. 读取 OI 6000 (档案表) 属性 2 -> 当前初始电表数应为 0
    uint16_t meter_count = 999;
    ret = dcu_router_get_attr(((uint32_t)OI_METER_ARCHIVE << 16) | 0x0200, &meter_count);
    assert_int_equal(ret, 0);
    assert_int_equal(meter_count, 0);

    // 4. 通过 OMD 方法路由调用 ACTION 注册新表计 (Method 127)
    int method_res = -1;
    ret = dcu_router_action(((uint32_t)OI_METER_ARCHIVE << 16) | 0x7F00, "20000001", &method_res);
    assert_int_equal(ret, 0);
    assert_int_equal(method_res, 0);  // 注册成功返回 0

    // 5. 再次通过 OAD 读取属性 2 -> 注册数应升为 1
    meter_count = 0;
    ret = dcu_router_get_attr(((uint32_t)OI_METER_ARCHIVE << 16) | 0x0200, &meter_count);
    assert_int_equal(ret, 0);
    assert_int_equal(meter_count, 1);

    // 6. 验证实时变量读取 OI 0x2000 (A相电压) -> 应返回 220.5V
    float voltage = 0.0f;
    ret = dcu_router_get_attr(OAD_VOLTAGE, &voltage);
    assert_int_equal(ret, 0);
    assert_float_equal(voltage, 220.5f, 0.01f);

    // 7. 验证实时变量读取 OI 0x2001 (A相电流) -> 应返回 4.95A
    float current = 0.0f;
    ret = dcu_router_get_attr(OAD_CURRENT, &current);
    assert_int_equal(ret, 0);
    assert_float_equal(current, 4.95f, 0.01f);

    /* 8. 远程负荷控制 ACTION 8000：**动作明确未实现**（缺 698 ACTION-Request 编码器）。
     *
     * 这条断言以前是 `ret == 0` —— 那是在给"假成功"背书：处理函数只把动作值写进
     * 影子表就返回成功，主站以为拉闸成功，继电器根本没动。
     * 现在断言 -4：**路由是通的**（不是 -2「方法不存在」），但动作明确拒绝，
     * 绝不伪装成功。等 698 ACTION-Request 的编码器和报文样例齐了再改成真下发。 */
    method_res = -1;
    ret = dcu_router_action(((uint32_t)OI_REMOTE_CONTROL << 16) | 0x0100, NULL, &method_res);
    assert_int_equal(ret, -4);

    // 9. 验证影子抓拍引擎 (Generic Snapshot Engine)
    dcu_event_obj_t evt_3000 = {
        .oi = OI_VOLTAGE_DROP, .snapshot_count = 2, .snapshot_oads = {OAD_VOLTAGE, OAD_CURRENT}};

    // 重新初始化以注册事件对象
    dcu_router_destroy();
    ret = dcu_router_init_ex(&test_inv, &poller, router_sink, NULL, NULL, &evt_3000, NULL,
                             router_bus);
    assert_int_equal(ret, 0);

    pipe_msg_t snapshot_msgs[10];
    int captured = dcu_obj_snapshot(OI_VOLTAGE_DROP, 0, snapshot_msgs, 10);
    assert_int_equal(captured, 2);
    assert_float_equal(snapshot_msgs[0].payload.val.f, 220.5f, 0.01f);
    assert_float_equal(snapshot_msgs[1].payload.val.f, 4.95f, 0.01f);

    // 10. 清理并销毁全部资源
    dcu_router_destroy();
    dcu_storage_destroy(&storage);
    dcu_inventory_destroy(&test_inv);
}

/* 槽位公式的上界必须落在影子容量内。
 *
 * `slot = device_id * DCU_SLOTS_PER_DEVICE + tag_slot`，而 datahub 的
 * `write_shadow_idx` 对 idx >= capacity 是**静默丢弃**。DCU 默认容量原来写死
 * 32768，只够 1024 只表（MAX_METERS 是 4096）——第 1025 只表起，值写不进去
 * 也没有任何痕迹。这条用例锁住"最后一只表的最后一个槽位仍可写"。 */
static void test_slot_upper_bound_fits_capacity(void** state) {
    (void)state;

    /* 用公式自己的最大 tag_slot 去顶：任何 OAD 的 slot 都 < SLOTS_PER_DEVICE */
    int worst = dcu_tag_map_fn(0xFFFFFFFFu, MAX_METERS - 1);
    assert_true(worst >= 0);
    assert_true((size_t)worst < (size_t)MAX_METERS * DCU_SLOTS_PER_DEVICE);

    /* 常用点位也要在范围内（防止某天改公式时把上界顶出容量） */
    int v = dcu_tag_map_fn(0x20000200u, MAX_METERS - 1);
    assert_true(v >= 0);
    assert_true((size_t)v < (size_t)MAX_METERS * DCU_SLOTS_PER_DEVICE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_router_dispatch),
        cmocka_unit_test(test_slot_upper_bound_fits_capacity),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
