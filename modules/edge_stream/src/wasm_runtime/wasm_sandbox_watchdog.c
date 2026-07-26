#include "wasm_sandbox_watchdog.h"
#include <string.h>

void wasm_sandbox_watchdog_init(wasm_sandbox_watchdog_t *wd, uint32_t max_mem, uint32_t max_cpu) {
    if (!wd) return;
    memset(wd, 0, sizeof(wasm_sandbox_watchdog_t));
    wd->max_memory_bytes = (max_mem > 0) ? max_mem : 16 * 1024 * 1024; // Default 16MB
    wd->max_cpu_time_ms = (max_cpu > 0) ? max_cpu : 50;                // Default 50ms per tick
    wd->terminated = false;
}

bool wasm_sandbox_watchdog_check(wasm_sandbox_watchdog_t *wd, uint32_t current_mem, uint32_t step_cpu_ms) {
    if (!wd || wd->terminated) return false;

    wd->current_memory_bytes = current_mem;
    wd->accumulated_cpu_ms += step_cpu_ms;

    if (wd->current_memory_bytes > wd->max_memory_bytes || wd->accumulated_cpu_ms > wd->max_cpu_time_ms) {
        wd->terminated = true;
        return false; // Violates watchdog budget, trigger termination
    }

    return true; // Resource consumption within budget
}
