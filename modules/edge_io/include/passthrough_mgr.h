#ifndef PASSTHROUGH_MGR_H
#define PASSTHROUGH_MGR_H

typedef struct uv_loop_s uv_loop_t;
#include "bus_orchestrator.h"

typedef struct passthrough_mgr_s passthrough_mgr_t;

typedef struct {
    uint16_t port;
    int target_bus_id;
} passthrough_config_t;

passthrough_mgr_t *passthrough_mgr_create(uv_loop_t *loop, bus_orchestrator_t *orch, arbiter_t *arb);
int passthrough_mgr_start(passthrough_mgr_t *mgr, const passthrough_config_t *cfgs, int count);
void passthrough_mgr_destroy(passthrough_mgr_t *mgr);

#endif
