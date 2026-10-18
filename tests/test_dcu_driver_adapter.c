/**
 * @file test_dcu_driver_adapter.c
 * @brief Unit tests for DCU autonomous driver adapter and point injection (PoC)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "dcu_driver_adapter.h"
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

static void test_dcu_driver_injection(void) {
    printf("[TEST] Testing dcu_get_driver_injection...\n");

    /* 1. Unknown protocol */
    assert(dcu_get_driver_injection(NULL) == NULL);
    assert(dcu_get_driver_injection("UNKNOWN_PROTO") == NULL);

    /* 2. DLT698 Injection */
    const driver_contract_injection_t *inj_698 = dcu_get_driver_injection("DLT698");
    assert(inj_698 != NULL);
    assert(inj_698->lookup_by_key != NULL);
    assert(inj_698->lookup_by_tag != NULL);

    const modbus_map_t *p698 = (const modbus_map_t *)inj_698->lookup_by_key(OAD_VOLTAGE);
    assert(p698 != NULL);
    assert(p698->tag_id == OAD_VOLTAGE);
    assert(p698->scaler > 0.0f);

    /* 3. DLT645 Injection */
    const driver_contract_injection_t *inj_645 = dcu_get_driver_injection("DLT645");
    assert(inj_645 != NULL);
    assert(inj_645->lookup_by_key != NULL);

    const dlt645_map_t *p645 = (const dlt645_map_t *)inj_645->lookup_by_key(0x02010100);
    assert(p645 != NULL);
    /* 645 A相电压 DI 0x02010100 映射至 698 OAD_VOLTAGE */
    assert(p645->tag_id == OAD_VOLTAGE);

    /* 4. Modbus Injection */
    const driver_contract_injection_t *inj_mb = dcu_get_driver_injection("ModbusRTU");
    assert(inj_mb != NULL);
    assert(inj_mb->lookup_by_key != NULL);

    printf("  --> Driver injection passed.\n");
}

int main(void) {
    printf("========================================\n");
    printf("  DCU Driver Adapter Unit Tests (PoC)   \n");
    printf("========================================\n");
    test_dcu_driver_registry();
    test_dcu_driver_injection();
    printf("ALL TESTS PASSED!\n");
    return 0;
}
