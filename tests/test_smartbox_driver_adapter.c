/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_smartbox_driver_adapter.c
 * @brief Unit tests for SmartBox autonomous driver adapter and point mapping (No-Contract)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "smartbox_driver_adapter.h"

static void test_smartbox_driver_registry(void) {
    printf("[TEST] Testing smartbox_driver_registry_init...\n");

    /* 1. NULL pointer error handling */
    assert(smartbox_driver_registry_init(NULL) == -1);

    /* 2. Success registration */
    driver_manager_t *mgr = driver_manager_create();
    assert(mgr != NULL);
    assert(smartbox_driver_registry_init(mgr) == 0);

    /* 3. Verify driver discovery */
    const edge_driver_t *mb_rtu = driver_manager_find(mgr, "ModbusRTU");
    assert(mb_rtu != NULL);
    assert(strcmp(mb_rtu->name, "ModbusRTU") == 0);

    const edge_driver_t *mb_tcp = driver_manager_find(mgr, "ModbusTCP");
    assert(mb_tcp != NULL);
    assert(strcmp(mb_tcp->name, "ModbusTCP") == 0);

    const edge_driver_t *can = driver_manager_find(mgr, "CAN");
    assert(can != NULL);
    assert(strcmp(can->name, "CAN") == 0);

    driver_manager_destroy(mgr);
    printf("  --> Driver registration passed.\n");
}

static void test_smartbox_tag_mapping(void) {
    printf("[TEST] Testing smartbox_tag_map_fn...\n");

    /* 1. Device 0 slot mapping */
    int slot_sys = smartbox_tag_map_fn(TAG_BOX_SYSTEM_STATUS, 0);
    assert(slot_sys == 0);

    int slot_led = smartbox_tag_map_fn(TAG_BOX_INDICATOR_LED, 0);
    assert(slot_led == 1);

    int slot_soc = smartbox_tag_map_fn(TAG_BATTERY_SOC, 0);
    assert(slot_soc == 12);

    /* 2. Device 1 slot offset (stride 32) */
    int slot_dev1_soc = smartbox_tag_map_fn(TAG_BATTERY_SOC, 1);
    assert(slot_dev1_soc == 32 + 12);

    /* 3. Small direct tag */
    assert(smartbox_tag_map_fn(10, 0) == 10);

    /* 4. Invalid high tag */
    assert(smartbox_tag_map_fn(99999, 0) == -1);

    printf("  --> Tag mapping passed.\n");
}

static void test_smartbox_tag_metadata(void) {
    printf("[TEST] Testing smartbox_get_tag_metadata...\n");

    uint32_t count = 0;
    const edge_tag_meta_t *meta = smartbox_get_tag_metadata(&count);
    assert(meta != NULL);
    assert(count > 0);

    /* Verify lookup */
    const edge_tag_meta_t *soc_meta = smartbox_get_tag_meta(TAG_BATTERY_SOC);
    assert(soc_meta != NULL);
    assert(strcmp(soc_meta->type, "FLOAT") == 0);
    assert(strcmp(soc_meta->unit, "%") == 0);

    /* Verify non-existent */
    assert(smartbox_get_tag_meta(99999) == NULL);

    printf("  --> Tag metadata lookup passed.\n");
}

int main(void) {
    printf("============================================\n");
    printf("  SmartBox Driver Adapter Unit Tests (No-Contract) \n");
    printf("============================================\n");
    test_smartbox_driver_registry();
    test_smartbox_tag_mapping();
    test_smartbox_tag_metadata();
    printf("ALL TESTS PASSED!\n");
    return 0;
}
