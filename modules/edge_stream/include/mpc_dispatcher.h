/**
 * @file mpc_dispatcher.h
 * @brief Model Predictive Control (MPC) Energy Dispatcher Aggregate Root (AEP-730 / AEP-752)
 *
 * Implements constrained QP-based rolling horizon power dispatch with:
 * 1. Physical Opaque Encapsulation (Opaque struct pointer)
 * 2. Caller-Allocated Storage (_Alignas(max_align_t) placeholder)
 * 3. Consumer-Defined Interfaces (mpc_dispatcher_ports_t)
 * 4. Zero-Heap Allocation during runtime execution
 */

#ifndef EDGE_STREAM_MPC_DISPATCHER_H
#define EDGE_STREAM_MPC_DISPATCHER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include "edge_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPC_MAX_HORIZON 10

/**
 * @brief Configuration parameters for MPC Dispatcher
 */
typedef struct {
    uint32_t horizon;           /**< Prediction horizon N (1 <= N <= MPC_MAX_HORIZON) */
    double dt_s;                /**< Control time step (seconds, typ. 1.0) */
    double capacity_ah;         /**< Battery nominal capacity (Ah, typ. 100.0) */
    double target_soc;          /**< Desired SOC setpoint (0.0 to 1.0, typ. 0.60) */
    double q_soc_weight;        /**< State penalty weight Q (typ. 1000.0) */
    double r_power_weight;      /**< Actuation penalty weight R (typ. 0.1) */
    double max_power_kw;        /**< Upper power bound (kW, >0) */
    double min_power_kw;        /**< Lower power bound (kW, <0) */
} mpc_dispatcher_config_t;

/**
 * @brief Consumer-Defined Port Interfaces for MPC Dispatcher
 */
typedef struct {
    edge_reader_port_t reader;  /**< Port for reading real-time telemetry (SOC, Voltage) */
    edge_writer_port_t writer;  /**< Port for writing computed optimal power setpoint */
    edge_clock_port_t  clock;   /**< Monotonic time port */
} mpc_dispatcher_ports_t;

/**
 * @brief Opaque Aggregate Root Type for MPC Dispatcher
 */
typedef struct mpc_dispatcher mpc_dispatcher_t;

/**
 * @brief Caller-Allocated Storage for MPC Dispatcher (Physical Opaque Encapsulation)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[4096];
} mpc_dispatcher_storage_t;

/**
 * @brief In-place placement constructor for MPC Dispatcher
 *
 * @param store Caller-provided storage buffer
 * @param cfg Dispatch configuration and constraints
 * @param ports Injected consumer-defined ports bundle (optional, may be NULL)
 * @return mpc_dispatcher_t* handle pointing into store, or NULL on error
 */
mpc_dispatcher_t *mpc_dispatcher_create_in(mpc_dispatcher_storage_t *store,
                                           const mpc_dispatcher_config_t *cfg,
                                           const mpc_dispatcher_ports_t *ports);

/**
 * @brief Execute one control step: Solve QP and determine optimal power output
 *
 * @param mpc Pointer to MPC dispatcher instance
 * @param current_soc Current measured SOC (0.0 to 1.0)
 * @param out_optimal_power_kw Output optimal power command (kW)
 * @return 0 on success, negative on error
 */
int mpc_dispatcher_step(mpc_dispatcher_t *mpc, double current_soc, double *out_optimal_power_kw);

/**
 * @brief Step MPC Dispatcher using Consumer-Defined Ports
 */
int mpc_dispatcher_step_ports(mpc_dispatcher_t *mpc, uint32_t tag_soc_in, uint32_t tag_p_out);

/**
 * @brief Reset internal MPC solver state
 */
void mpc_dispatcher_reset(mpc_dispatcher_t *mpc);

/* -------------------------------------------------------------------------- */
/* State Inspectors                                                           */
/* -------------------------------------------------------------------------- */
double mpc_dispatcher_get_last_power(const mpc_dispatcher_t *mpc);
uint32_t mpc_dispatcher_get_step_count(const mpc_dispatcher_t *mpc);
double mpc_dispatcher_get_target_soc(const mpc_dispatcher_t *mpc);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_STREAM_MPC_DISPATCHER_H */
