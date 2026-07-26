#ifndef WASM_SANDBOX_WATCHDOG_H
#define WASM_SANDBOX_WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t max_memory_bytes;
    uint32_t max_cpu_time_ms;
    uint32_t current_memory_bytes;
    uint32_t accumulated_cpu_ms;
    bool terminated;
} wasm_sandbox_watchdog_t;

void wasm_sandbox_watchdog_init(wasm_sandbox_watchdog_t *wd, uint32_t max_mem, uint32_t max_cpu);
bool wasm_sandbox_watchdog_check(wasm_sandbox_watchdog_t *wd, uint32_t current_mem, uint32_t step_cpu_ms);

#ifdef __cplusplus
}
#endif

#endif // WASM_SANDBOX_WATCHDOG_H
