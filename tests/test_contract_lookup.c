#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contracts.h"

/**
 * [Test 1] 深度验证 Modbus 查找 (Acrel P80)
 */
static void test_lookup_modbus_advanced(void **state) {
    (void)state;
    float scale = 0.0f;

    // 验证 A相电压
    assert_int_equal(edge_lookup_modbus(0x0000, &scale), TAG_VOLTAGE_PHASE_A);
    assert_float_equal(scale, 0.1f, 0.0001f);

    // 验证 总有功功率
    assert_int_equal(edge_lookup_modbus(0x000C, &scale), TAG_GRID_POWER);
    assert_float_equal(scale, 1.0f, 0.0001f);
}

/**
 * [Test 2] 深度验证 DLT645 查找 (数字接口 & 字符串接口)
 */
static void test_lookup_dlt645_full(void **state) {
    (void)state;
    float scale = 0.0f;

    // 1. 字符串接口 (DI: 00010000 -> 正向有功总电能)
    assert_int_equal(edge_lookup_dlt645("00010000", &scale), TAG_ENERGY_ACTIVE_IMPORT);
    assert_float_equal(scale, 0.01f, 0.0001f);

    // 2. 生产级数字接口 (DI_NUM: 0x00010000)
    // 这一步直接走 Perfect Hash Mixer，性能最高
    assert_int_equal(edge_lookup_dlt645_num(0x00010000, &scale), TAG_ENERGY_ACTIVE_IMPORT);
    assert_float_equal(scale, 0.01f, 0.0001f);

    // 3. 验证 A相电压 DI (02010100)
    assert_int_equal(edge_lookup_dlt645_num(0x02010100, &scale), TAG_VOLTAGE_PHASE_A);
    assert_float_equal(scale, 0.1f, 0.0001f);
}

/**
 * [Test 3] 验证 CAN 静态映射表 (g_can_map)
 */
static void test_lookup_can_map(void **state) {
    (void)state;
    bool found = false;

    // 查找电池 SOC 对应的 CAN 帧解析配置
    for (int i = 0; i < g_can_map_len; i++) {
        if (g_can_map[i].id == TAG_BATTERY_SOC) {
            assert_int_equal(g_can_map[i].can_id, 0x1801F456);
            assert_float_equal(g_can_map[i].factor, 0.4f, 0.0001f);
            found = true;
            break;
        }
    }
    assert_true(found);
}

/**
 * [Test 4] 验证 IEC104 IOA 查找
 */
static void test_lookup_iec104(void **state) {
    (void)state;
    // 验证本地扩展定义的 104 地址
    assert_int_equal(edge_lookup_iec104(10001), TAG_BATTERY_SOC);
    assert_int_equal(edge_lookup_iec104(20001), TAG_GRID_POWER);

    // 非法 IOA
    assert_int_equal(edge_lookup_iec104(99999), TAG_INVALID);
}

/**
 * [Test 5] 验证编译器自检逻辑
 */
static void test_contract_self_consistency(void **state) {
    (void)state;
    // 该函数在 C 内部执行“闭环验证”，即从 YAML 地址出发，反查 Hash 表
    assert_int_equal(edge_contract_self_test(), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_lookup_modbus_advanced),
        cmocka_unit_test(test_lookup_dlt645_full),
        cmocka_unit_test(test_lookup_can_map),
        cmocka_unit_test(test_lookup_iec104),
        cmocka_unit_test(test_contract_self_consistency),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
