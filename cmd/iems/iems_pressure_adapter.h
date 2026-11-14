/**
 * @file iems_pressure_adapter.h
 * @brief Composition-side pressure port for the IEMS app manager.
 */

#ifndef IEMS_PRESSURE_ADAPTER_H
#define IEMS_PRESSURE_ADAPTER_H

#include <stdint.h>

#include "edgeos/app_pressure.h"
#include "iems_app_runtime.h"
#include "rest_control_port.h"

struct edge_app_mgr_s;
typedef struct edge_app_mgr_s edge_app_mgr_t;

typedef struct {
    edge_app_mgr_t *mgr;
    iems_app_runtime_t *runtime;
    rest_control_port_t *control;
    app_pressure_policy_t parse_policy;
} iems_pressure_ctx_t;

/**
 * Build the pressure engine and its IEMS port. Reclaim is opt-in through
 * EDGEC_PRESSURE_RECLAIM=1; otherwise the engine only monitors and emits.
 */
void iems_pressure_init(app_pressure_t *pressure, app_pressure_port_t *port,
                        iems_pressure_ctx_t *ctx, edge_app_mgr_t *mgr,
                        iems_app_runtime_t *runtime, rest_control_port_t *control);

/** Run one engine step at a monotonic timestamp (uv_now). */
void iems_pressure_tick(app_pressure_t *pressure, uint64_t now_ms);

#endif /* IEMS_PRESSURE_ADAPTER_H */
