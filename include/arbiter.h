#pragma once
#include <stdint.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== Bus Types ===== */
typedef enum {
    BUS_RS485_1 = 0,
    BUS_RS485_2,
    BUS_RS485_3,
    BUS_HPLC,
    BUS_CAN,
    BUS_MAX
} arbiter_bus_t;

/* ===== Request Priority ===== */
typedef enum {
    ARB_PRIO_LOW = 0,      
    ARB_PRIO_NORMAL = 10, 
    ARB_PRIO_HIGH = 50,   
    ARB_PRIO_CRITICAL = 100,
    ARB_PRIO_EMERGENCY = 200 /* Added for Orchestrator */
} arbiter_prio_t;

/* ===== Request Mode ===== */
typedef enum {
    ARB_SHARED = 0,   
    ARB_EXCLUSIVE     
} arbiter_mode_t;

typedef struct arbiter arbiter_t;

typedef void (*arbiter_cb)(
    arbiter_t *arb,
    arbiter_bus_t bus,
    void *user
);

/* ===== API ===== */
arbiter_t *arbiter_create(uv_loop_t *loop);
void arbiter_destroy(arbiter_t *arb);

/* 请求总线（异步回调） */
int arbiter_request(
    arbiter_t *arb,
    arbiter_bus_t bus,
    arbiter_prio_t prio,
    arbiter_mode_t mode,
    uint64_t timeout_ms,
    arbiter_cb on_grant,
    void *user
);

/* 主动释放 */
void arbiter_release(
    arbiter_t *arb,
    arbiter_bus_t bus,
    void *user
);

#ifdef __cplusplus
}
#endif
