/**
 * @file storage_powercut_fault_injector.h
 * @brief Industrial Power-Cut & Storage Fault Injector Test Fixture (AEP-410)
 */

#ifndef STORAGE_POWERCUT_FAULT_INJECTOR_H
#define STORAGE_POWERCUT_FAULT_INJECTOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FAULT_INJECT_NONE = 0,
    FAULT_INJECT_EIO = 1,          // Simulate I/O Error
    FAULT_INJECT_TORN_WRITE = 2,    // Simulate Partial Sector / Half-Written Page
    FAULT_INJECT_SUDDEN_POWERCUT = 3 // Simulate Sudden Power Loss (abrupt truncation)
} storage_fault_type_t;

typedef struct {
    storage_fault_type_t active_fault;
    uint32_t ops_until_fault;      // Countdown until injecting fault
    uint32_t injected_fault_count;
    uint32_t recovered_count;
} storage_fault_injector_t;

/* Initialize Fault Injector */
void storage_fault_injector_init(storage_fault_injector_t *inj);

/* Arm Fault Injector with specific fault after N writes */
void storage_fault_injector_arm(storage_fault_injector_t *inj, storage_fault_type_t fault, uint32_t ops_delay);

/* Check and trigger fault during write operation */
bool storage_fault_injector_should_fail(storage_fault_injector_t *inj);

/* Inject Corrupt / Torn bytes into buffer to simulate torn write */
void storage_fault_injector_mangle_buffer(void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_POWERCUT_FAULT_INJECTOR_H
