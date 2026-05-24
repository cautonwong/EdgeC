#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <string.h>
#include <uv.h>
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_storage.h"
#include "dcu_router.h"

// 验证对象字典与属性/方法路由的测试用例
static void test_dcu_router_dispatch(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    
    // 1. 初始化底层的档案、调度器与持久化存储服务
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    
    dcu_poller_t poller;
    dcu_poller_init(&poller);
    
    dcu_storage_t storage;
    int ret = dcu_storage_init(&storage, loop, ":memory:");
    assert_int_equal(ret, 0);
    
    // 2. 初始化核心对象字典路由器
    ret = dcu_router_init_ex(&inv, &poller, &storage, NULL, NULL, NULL, NULL);
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
    assert_int_equal(method_res, 0); // 注册成功返回 0
    
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
    
    // 8. 验证远程负荷控制执行动作 ACTION 8000 -> 模拟合分闸
    method_res = -1;
    ret = dcu_router_action(((uint32_t)OI_REMOTE_CONTROL << 16) | 0x0100, NULL, &method_res);
    assert_int_equal(ret, 0);
    assert_int_equal(method_res, 0);
    
    // 9. 清理并销毁全部资源
    dcu_router_destroy();
    dcu_storage_destroy(&storage);
    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_router_dispatch),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
