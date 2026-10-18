#ifndef EDGE_WASM_APP_SCHEDULER_H
#define EDGE_WASM_APP_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
typedef struct uv_loop_s uv_loop_t;
typedef struct datahub_s datahub_t;
#include "wasm_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wasm_scheduler_s wasm_scheduler_t;

wasm_scheduler_t *wasm_scheduler_create(datahub_t *hub, uv_loop_t *loop, uint32_t tick_period_ms);
void wasm_scheduler_destroy(wasm_scheduler_t *sched);

int wasm_scheduler_start(wasm_scheduler_t *sched);
int wasm_scheduler_stop(wasm_scheduler_t *sched);

int wasm_scheduler_load_app(wasm_scheduler_t *sched, const char *name, const char *wasm_path, uint32_t heap_size, uint32_t stack_size, uint32_t timeout_ms);
int wasm_scheduler_load_app_ex(wasm_scheduler_t *sched, const char *name, const char *wasm_path,
                              uint32_t heap_size, uint32_t stack_size, uint32_t timeout_ms,
                              uint32_t period_ms, uint32_t cpu_budget_ms, uint32_t deadline_ms);
int wasm_scheduler_get_app_stats(wasm_scheduler_t *sched, const char *name,
                                uint64_t *out_ticks, uint64_t *out_overruns);
int wasm_scheduler_unload_app(wasm_scheduler_t *sched, const char *name);
int wasm_scheduler_reload_app(wasm_scheduler_t *sched, const char *name, const char *wasm_path, uint32_t heap_size, uint32_t stack_size, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // EDGE_WASM_APP_SCHEDULER_H
