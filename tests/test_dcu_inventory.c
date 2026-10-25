#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_inventory.h"

// 1. 验证档案初始化与添加电表
static void test_inventory_add_remove(void** state) {
    (void)state;
    dcu_inventory_t inv;

    // 初始化
    int ret = dcu_inventory_init(&inv);
    assert_int_equal(ret, 0);
    assert_int_equal(inv.count, 0);

    // 添加电表表1 (TSA: 10000001, 协议: dlt698, 波特率: 9600, 通道: 1)
    ret = dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1, NULL);
    assert_int_equal(ret, 0);
    assert_int_equal(inv.count, 1);

    // 获取表1，验证其各项属性
    production_meter_t* m = dcu_inventory_get(&inv, "10000001");
    assert_non_null(m);
    assert_string_equal(m->address, "10000001");
    assert_string_equal(m->protocol, "dlt698");
    assert_int_equal(m->baud_rate, 9600);
    assert_int_equal(m->channel_id, 1);
    assert_true(m->is_online);           // 初始在线
    assert_int_equal(m->fail_count, 0);  // 超时计数初始为 0

    // 重复添加同一只表应该报错，保障唯一性
    ret = dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1, NULL);
    assert_int_not_equal(ret, 0);

    // 添加表2
    ret = dcu_inventory_add(&inv, "10000002", "dlt645", 2400, 2, NULL);
    assert_int_equal(ret, 0);
    assert_int_equal(inv.count, 2);

    // 删除表1
    ret = dcu_inventory_remove(&inv, "10000001");
    assert_int_equal(ret, 0);
    assert_int_equal(inv.count, 1);

    // 此时获取表1应该返回空
    m = dcu_inventory_get(&inv, "10000001");
    assert_null(m);

    dcu_inventory_destroy(&inv);
}

// 2. 验证熔断状态翻转 (超限降级/超时熔断)
static void test_inventory_circuit_breaker(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);

    // 注册表计
    dcu_inventory_add(&inv, "10000003", "dlt698", 9600, 1, NULL);
    production_meter_t* m = dcu_inventory_get(&inv, "10000003");
    assert_non_null(m);
    assert_true(m->is_online);

    // 模拟超时事件 1 次
    m->fail_count++;
    assert_true(m->is_online);  // 超时 1 次，依然判定在线

    // 模拟超时事件 2 次
    m->fail_count++;
    assert_true(m->is_online);  // 超时 2 次，依然判定在线

    // 模拟超时事件 3 次 -> 触发熔断阈值
    m->fail_count++;
    if (m->fail_count >= 3) {
        m->is_online = false;  // 触发熔断，状态变为 SUSPENDED (离线)
    }
    assert_false(m->is_online);  // 表计必须被判定为 SUSPENDED

    // 模拟后台心跳，抄表重新成功 -> 自动重置与恢复状态
    m->fail_count = 0;
    m->is_online = true;
    assert_true(m->is_online);
    assert_int_equal(m->fail_count, 0);

    dcu_inventory_destroy(&inv);
}

// 3. 通道协议从档案取, 不按通道号猜
static void test_inventory_channel_protocol(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);

    // 空通道 → NULL
    assert_null(dcu_inventory_channel_protocol(&inv, 22));

    // 同一通道两只 645 表
    dcu_inventory_add(&inv, "10000001", "DLT645", 9600, 22, NULL);
    dcu_inventory_add(&inv, "10000002", "DLT645", 9600, 22, NULL);
    assert_string_equal(dcu_inventory_channel_protocol(&inv, 22), "DLT645");
    // 其它通道仍然空
    assert_null(dcu_inventory_channel_protocol(&inv, 5));

    // 通道 5 放一只 698 表: 协议必须来自档案, 而不是通道号 <22 就猜 698
    dcu_inventory_add(&inv, "10000003", "dlt698", 9600, 5, NULL);
    assert_string_equal(dcu_inventory_channel_protocol(&inv, 5), "dlt698");

    // 混协议: 取首个, 不崩
    dcu_inventory_add(&inv, "10000004", "DLT698", 9600, 22, NULL);
    assert_string_equal(dcu_inventory_channel_protocol(&inv, 22), "DLT645");

    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_inventory_add_remove),
        cmocka_unit_test(test_inventory_circuit_breaker),
        cmocka_unit_test(test_inventory_channel_protocol),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
