/**
 * @file storage_powercut_fault_injector.c
 * @brief Industrial Power-Cut & Storage Fault Injector Implementation (AEP-410)
 */

#include "storage_powercut_fault_injector.h"

#include <string.h>

void storage_fault_injector_init(storage_fault_injector_t *inj) {
    if (!inj) return;
    memset(inj, 0, sizeof(*inj));
    inj->active_fault = FAULT_INJECT_NONE;
}

void storage_fault_injector_arm(storage_fault_injector_t *inj, storage_fault_type_t fault, uint32_t ops_delay) {
    if (!inj) return;
    inj->active_fault = fault;
    inj->ops_until_fault = ops_delay;
}

bool storage_fault_injector_should_fail(storage_fault_injector_t *inj) {
    if (!inj || inj->active_fault == FAULT_INJECT_NONE) return false;

    if (inj->ops_until_fault > 0) {
        inj->ops_until_fault--;
        return false;
    }

    // Trigger fault
    inj->injected_fault_count++;
    return true;
}

void storage_fault_injector_mangle_buffer(void *buf, size_t len) {
    if (!buf || len < 4) return;
    uint8_t *p = (uint8_t *)buf;
    // Overwrite the latter half with zero/garbage simulating power cut mid-page
    size_t half = len / 2;
    memset(p + half, 0xAA, len - half);
}
