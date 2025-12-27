#pragma once
#include <stdint.h>
#include <uv.h>
#include <stdbool.h>

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
    ARB_PRIO_EMERGENCY = 200 
} arbiter_prio_t;

/* ===== Request Mode ===== */
typedef enum {
    ARB_SHARED = 0,   
    ARB_EXCLUSIVE     
} arbiter_mode_t;

typedef struct arbiter arbiter_t;
typedef struct arb_req arbiter_req_t;

typedef void (*arbiter_on_start_cb)(void *user);
typedef void (*arbiter_on_preempt_cb)(void *user);
typedef void (*arbiter_on_done_cb)(void *user);

struct arb_req {
    arbiter_prio_t priority;
    arbiter_mode_t mode;
    uint64_t timeout_ms;
    
    arbiter_on_start_cb   on_start;
    arbiter_on_preempt_cb on_preempt;
    arbiter_on_done_cb    on_done;
    
    void *user_start;
    void *user_preempt;
    void *user_done;

    /* Internal */
    uint64_t expire_at;
    struct arb_req *next;
};

/* ===== API ===== */
int  arbiter_init(arbiter_t **arb, uv_loop_t *loop);
void arbiter_shutdown(arbiter_t *arb);

/* Register a bus by name (placeholder for now) */
typedef void* arbiter_bus_handle_t;
arbiter_bus_handle_t arbiter_bus_register(arbiter_t *arb, const char *name);

/* Submit a request */
int arbiter_submit(arbiter_bus_handle_t bus_h, arbiter_req_t *req);

/* Active Release */
void arbiter_release(arbiter_bus_handle_t bus_h, arbiter_req_t *req);

#ifdef __cplusplus
}
#endif