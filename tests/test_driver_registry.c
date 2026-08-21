/**
 * @file test_driver_registry.c
 * @brief Unit tests for AEP-301 generated driver registry
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contracts.h"
#include "driver_manager.h"
#include "edge_driver.h"

static void test_generated_driver_registry_table(void) {
    printf("[TEST] Checking generated driver registry table metadata...\n");

    /* 1. Registry count must be non-zero */
    assert(g_edge_driver_registry_count > 0);
    printf("  --> g_edge_driver_registry_count = %zu\n", g_edge_driver_registry_count);

    /* 2. Each entry must have a valid protocol name and driver pointer */
    for (size_t i = 0; i < g_edge_driver_registry_count; i++) {
        const edge_driver_reg_entry_t *entry = &g_edge_driver_registry[i];
        assert(entry->protocol_name != NULL);
        assert(strlen(entry->protocol_name) > 0);
        assert(entry->driver != NULL);
        assert(entry->driver->name != NULL);
        printf("  --> Entry [%zu]: Protocol = %s, Driver Symbol Name = %s\n",
               i, entry->protocol_name, entry->driver->name);
    }
}

static void test_driver_registry_lifecycle_and_lookup(void) {
    printf("[TEST] Testing driver registry registration and lookup via driver_manager...\n");

    /* 1. Test NULL error handling */
    assert(edge_driver_registry_init(NULL) == -1);

    /* 2. Create driver manager */
    driver_manager_t *mgr = driver_manager_create();
    assert(mgr != NULL);

    /* 3. Initialize registry table into manager */
    int rc = edge_driver_registry_init(mgr);
    assert(rc == 0);
    (void)rc;

    /* 4. Verify all registered drivers can be located */
    for (size_t i = 0; i < g_edge_driver_registry_count; i++) {
        const char *proto = g_edge_driver_registry[i].protocol_name;
        const edge_driver_t *drv = driver_manager_find(mgr, proto);
        assert(drv != NULL);
        assert(drv == g_edge_driver_registry[i].driver);
        (void)drv;
    }

    /* 5. Verify driver instance creation for a registered driver */
    const char *first_proto = g_edge_driver_registry[0].protocol_name;
    edge_driver_ctx_t *ctx = driver_instance_create(mgr, first_proto, 101);
    assert(ctx != NULL);
    assert(ctx->driver == g_edge_driver_registry[0].driver);
    assert(ctx->device_id == 101);

    driver_instance_destroy(ctx, first_proto);
    driver_manager_destroy(mgr);

    printf("  --> Driver manager successfully populated from generated registry!\n");
}

int main(void) {
    printf("=== Starting AEP-301 Generated Driver Registry Tests ===\n");
    test_generated_driver_registry_table();
    test_driver_registry_lifecycle_and_lookup();
    printf("=== All AEP-301 Generated Driver Registry Tests PASSED ===\n");
    return 0;
}
