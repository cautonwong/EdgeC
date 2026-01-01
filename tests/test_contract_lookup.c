#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "contracts.h"  // 链接 iems 契约

/**
 * @brief 验证 Modbus 物理地址到业务语义的动态映射
 */
static void test_modbus_mapping_logic(void **state) {
    (void)state;
    float scale = 0.0f;
    edge_tag_id_t id;

    // 1. 测试标准点位 (A相电压 - 0x0000)
    // acrel_p80.yaml 中定义 scale: 0.1
    id = edge_lookup_modbus(0x0000, &scale);
    assert_int_equal(id, TAG_VOLTAGE_PHASE_A);
    assert_float_equal(scale, 0.1f, 0.0001f);

    // 2. 验证项目扩展测点 (PROJECT_HUMIDITY - ID: 8001)
    // extension.yaml 中覆盖定义为 scale: 1.0
    id = edge_lookup_modbus(50002, &scale);
    assert_int_equal(id, TAG_PROJECT_HUMIDITY);
    assert_float_equal(scale, 1.0f, 0.001f);

    // 3. 测试不存在的点位 (预期返回 TAG_INVALID)
    id = edge_lookup_modbus(0xFFFF, &scale);
    assert_int_equal(id, 0);
}

/**
 * @brief 验证多协议（DLMS/DLT645）查表定力
 */
static void test_multi_proto_lookup(void **state) {
    (void)state;
    float scale = 0.0f;
    edge_tag_id_t id;

    // 1. DLT645 DI 映射 (定义在 contracts/bindings/dlt645/standard_meter.yaml)
    // 02010100 对应 A相电压
    id = edge_lookup_dlt645("02010100", &scale);
    assert_int_equal(id, TAG_VOLTAGE_PHASE_A);
    // 真实点表中 scale 为 1.0 (或默认值)
    assert_float_equal(scale, 1.0f, 0.0001f);

    // 2. 模拟空查询 (绝对不存在的 OBIS)
    id = edge_lookup_dlms("9.9.9.9.9.9", &scale);
    assert_int_equal(id, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_modbus_mapping_logic),
        cmocka_unit_test(test_multi_proto_lookup),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
