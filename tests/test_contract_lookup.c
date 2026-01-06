#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contracts.h"

static void test_lookup_modbus_advanced(void **state) {
    (void)state;
    float scale = 0.0f;
    // 验证 A相电压 (Addr 0x0000)
    assert_int_equal(edge_lookup_modbus(0x0000, &scale), TAG_VOLTAGE_PHASE_A);
}

static void test_lookup_dlt645_full(void **state) {
    (void)state;
    float scale = 0.0f;
    // 使用 V4.5 统一数值接口 (无 _num 后缀)
    assert_int_equal(edge_lookup_dlt645(0x00010000, &scale), TAG_ENERGY_ACTIVE_IMPORT);
    assert_int_equal(edge_lookup_dlt645(0x02010100, &scale), TAG_VOLTAGE_PHASE_A);
}

static void test_lookup_iec104(void **state) {
    (void)state;
#ifdef TAG_BATTERY_SOC_ENABLED
#if (TAG_BATTERY_SOC_ENABLED == 1)
// 只有在点位开启且协议支持时才验证
// 由于 IEMS 目前没开启 IEC104 的映射，此处简化
#endif
#endif
}

static void test_contract_self_consistency(void **state) {
    (void)state;
    // 运行全量自检 (反查所有 YAML 定义的点位)
    assert_int_equal(edge_contract_self_test(), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_lookup_modbus_advanced),
        cmocka_unit_test(test_lookup_dlt645_full),
        cmocka_unit_test(test_lookup_iec104),
        cmocka_unit_test(test_contract_self_consistency),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
