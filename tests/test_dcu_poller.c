#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <string.h>
#include "dcu_inventory.h"
#include "dcu_poller.h"

// 1. 验证轮询方案添加与解析
static void test_scheme_add(void **state) {
    (void)state;
    dcu_poller_t poller;
    int ret = dcu_poller_init(&poller);
    assert_int_equal(ret, 0);
    assert_int_equal(poller.scheme_count, 0);

    // 注册方案1 (5秒快速遥测，OAD列表：20000200 电压, 20010200 电流)
    uint32_t oads[2] = {0x20000200, 0x20010200};
    ret = dcu_poller_add_scheme(&poller, 1, 5, oads, 2);
    assert_int_equal(ret, 0);
    assert_int_equal(poller.scheme_count, 1);

    dcu_scheme_t *s = dcu_poller_get_scheme(&poller, 1);
    assert_non_null(s);
    assert_int_equal(s->scheme_id, 1);
    assert_int_equal(s->interval_s, 5);
    assert_int_equal(s->oad_count, 2);
    assert_int_equal(s->oads[0], 0x20000200);
    assert_int_equal(s->oads[1], 0x20010200);

    dcu_poller_destroy(&poller);
}

// 2. 验证 RS485 半双工总线上的串行队列生成及熔断跳过 (Queue Skip)
static void test_queue_generation_and_skip(void **state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    // 1. 注册 3 台电表
    dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "10000002", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "10000003", "dlt698", 9600, 1);

    // 2. 注册周期方案 (10s, 抄读 OAD 20000200)
    uint32_t oads[1] = {0x20000200};
    dcu_poller_add_scheme(&poller, 2, 10, oads, 1);

    // 3. 生成串行执行队列 (所有表都处于在线 ACTIVE 状态)
    dcu_poll_queue_t queue;
    int ret = dcu_poller_generate_queue(&poller, 2, &inv, &queue);
    
    assert_int_equal(ret, 0);
    assert_int_equal(queue.count, 3); // 3台在线，队列长度必须为 3
    assert_string_equal(queue.entries[0].meter_address, "10000001");
    assert_string_equal(queue.entries[1].meter_address, "10000002");
    assert_string_equal(queue.entries[2].meter_address, "10000003");
    assert_int_equal(queue.entries[0].oad_count, 1);
    assert_int_equal(queue.entries[0].oads[0], 0x20000200);

    // 4. 模拟表2熔断 (is_online = false, 进入 SUSPENDED 状态)
    production_meter_t *m2 = dcu_inventory_get(&inv, "10000002");
    assert_non_null(m2);
    m2->is_online = false;

    // 5. 重新为方案2生成物理串行轮询队列 -> 表2必须被完美跳过
    memset(&queue, 0, sizeof(dcu_poll_queue_t));
    ret = dcu_poller_generate_queue(&poller, 2, &inv, &queue);
    
    assert_int_equal(ret, 0);
    assert_int_equal(queue.count, 2); // 只有表1和表3，表2被跳过，长度为 2
    assert_string_equal(queue.entries[0].meter_address, "10000001");
    assert_string_equal(queue.entries[1].meter_address, "10000003"); // 表3被前移

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_scheme_add),
        cmocka_unit_test(test_queue_generation_and_skip),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
