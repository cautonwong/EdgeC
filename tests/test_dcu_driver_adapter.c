/**
 * @file test_dcu_driver_adapter.c
 * @brief Unit tests for DCU autonomous driver adapter and point injection (PoC)
 */

/* 构建类型是 Release 时 assert 会被 NDEBUG 全部抹掉，测试变成空壳 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "dcu_driver_adapter.h"
#include "modbus_codec.h"
#include "dcu_router.h"
#include "driver_manager.h"
#include "edge_driver.h"

static void test_dcu_driver_registry(void) {
    printf("[TEST] Testing dcu_driver_registry_init...\n");

    /* 1. NULL check */
    assert(dcu_driver_registry_init(NULL) == -1);

    /* 2. Success registration */
    driver_manager_t *mgr = driver_manager_create();
    assert(mgr != NULL);
    assert(dcu_driver_registry_init(mgr) == 0);

    /* 3. Verify driver discovery */
    const edge_driver_t *dlt698 = driver_manager_find(mgr, "DLT698");
    assert(dlt698 != NULL);
    assert(strcmp(dlt698->name, "DLT698") == 0);

    const edge_driver_t *dlt645 = driver_manager_find(mgr, "DLT645");
    assert(dlt645 != NULL);
    assert(strcmp(dlt645->name, "DLT645") == 0);

    const edge_driver_t *modbus = driver_manager_find(mgr, "ModbusRTU");
    assert(modbus != NULL);

    driver_manager_destroy(mgr);
    printf("  --> Driver registration passed.\n");
}

static void test_dcu_point_translation(void) {
    printf("[TEST] Testing dcu point translation (OAD <-> 协议号)...\n");

    /* [ADR-002] 驱动不再拿点位表，翻译全在产品侧：这里验的就是这条边界。
     * 驱动侧的输入只有协议号（645 是 DI、Modbus 是地址+个数、698 是 OAD）。 */

    /* 1. DLT645：OAD ↔ DI 双向，且量纲非 0 */
    assert(dcu_driver_adapter_oad_to_proto("DLT645", OAD_VOLTAGE) == 0x02010100);
    assert(dcu_driver_adapter_proto_to_oad("DLT645", 0x02010100) == OAD_VOLTAGE);
    assert(dcu_driver_adapter_scale("DLT645", 0x02010100) > 0.0);

    assert(dcu_driver_adapter_proto_to_oad("DLT645", 0x02010200) == OAD_VOLTAGE_B);
    assert(dcu_driver_adapter_proto_to_oad("DLT645", 0x02010300) == OAD_VOLTAGE_C);
    assert(dcu_driver_adapter_proto_to_oad("DLT645", 0x02800002) == OAD_GRID_FREQUENCY);

    /* 未注册的协议号/未知协议必须返回 0，而不是猜一个 */
    assert(dcu_driver_adapter_proto_to_oad("DLT645", 0xDEADBEEF) == 0);
    assert(dcu_driver_adapter_scale("DLT645", 0xDEADBEEF) == 0.0);
    assert(dcu_driver_adapter_oad_to_proto("UNKNOWN", OAD_VOLTAGE) == 0);

    /* 2. DLT698：三相与功率、频率都要能翻成协议号 */
    assert(dcu_driver_adapter_oad_to_proto("DLT698", OAD_VOLTAGE) != 0);
    assert(dcu_driver_adapter_oad_to_proto("DLT698", OAD_VOLTAGE_B) != 0);
    assert(dcu_driver_adapter_oad_to_proto("DLT698", OAD_VOLTAGE_C) != 0);
    assert(dcu_driver_adapter_oad_to_proto("DLT698", OAD_ACTIVE_POWER_TOTAL) != 0);
    assert(dcu_driver_adapter_oad_to_proto("DLT698", OAD_GRID_FREQUENCY) != 0);

    /* 3. Modbus：点位标识 = 地址 + 寄存器个数，长度不能丢 */
    uint32_t mb_vb = dcu_driver_adapter_oad_to_proto("ModbusRTU", OAD_VOLTAGE_B);
    assert(mb_vb != 0);
    assert(MODBUS_NATIVE_ADDR(mb_vb) == 1);
    uint32_t mb_freq = dcu_driver_adapter_oad_to_proto("ModbusRTU", OAD_GRID_FREQUENCY);
    assert(mb_freq != 0);
    assert(MODBUS_NATIVE_ADDR(mb_freq) == 28);

    printf("  --> Point translation passed.\n");
}

static void test_dcu_slot_mapping(void) {
    printf("[TEST] Testing dcu_tag_map_fn dynamic hash slot mappings (No-Array)...\n");

    /* Device 1 (base 32) */
    assert(dcu_tag_map_fn(OAD_VOLTAGE, 1) == 32 + 0);
    assert(dcu_tag_map_fn(1001, 1) == 32 + 0);
    assert(dcu_tag_map_fn(OAD_ACTIVE_ENERGY, 1) == 32 + 1);
    assert(dcu_tag_map_fn(OAD_CURRENT, 1) == 32 + 2);

    /* Dynamic hashing without array: device slots are strictly isolated */
    int slot_dev1 = dcu_tag_map_fn(OAD_VOLTAGE_B, 1);
    int slot_dev2 = dcu_tag_map_fn(OAD_VOLTAGE_B, 2);
    assert(slot_dev1 >= 32 && slot_dev1 < 64);
    assert(slot_dev2 >= 64 && slot_dev2 < 96);
    assert(slot_dev1 + 32 == slot_dev2);

    /* Zero/Invalid tag rejected */
    assert(dcu_tag_map_fn(0, 1) == -1);

    printf("  --> Dynamic slot mapping passed.\n");
}

int main(void) {
    printf("========================================\n");
    printf("  DCU Driver Adapter Unit Tests (PoC)   \n");
    printf("========================================\n");
    test_dcu_driver_registry();
    test_dcu_point_translation();
    test_dcu_slot_mapping();
    printf("ALL TESTS PASSED!\n");
    return 0;
}
