/**
 * @file iems_bess_ports.h
 * @brief Ports owned by the BESS aggregate (consumer-defined).
 *
 * The aggregate includes only this header; composition-root adapters implement
 * it. A port status means "accepted/rejected", never "executed on the device":
 * the execution port has no completion callback, so device confirmation needs
 * the P5-01c readback path.
 *
 * Fault bit contract: adapters publish device-fault sources in bits 0..30.
 * Bit 31 is reserved for the aggregate's own command-failure latch.
 */

#ifndef IEMS_BESS_PORTS_H
#define IEMS_BESS_PORTS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IEMS_BESS_PORT_OK = 0,
    IEMS_BESS_PORT_REJECTED = -1,
    IEMS_BESS_PORT_TIMEOUT = -2,
    IEMS_BESS_PORT_IO = -3,
    IEMS_BESS_PORT_NO_DATA = -4
} iems_bess_port_status_t;

typedef enum {
    IEMS_BESS_PCS_MODE_PQ = 0,
    IEMS_BESS_PCS_MODE_FAULT = 1,
    IEMS_BESS_PCS_MODE_STANDBY = 2
} iems_bess_pcs_mode_t; /* subset of iems_pcs_mode_t (iems_pcs_controller.h) */

typedef struct {
    uint8_t cabinet_id;          /* 1..20 */
    double active_power_kw;      /* positive discharge, negative charge */
    double reactive_power_kvar;  /* shortcut: always 0 in P5-01b; wired with the P5-01c reactive path. */
    iems_bess_pcs_mode_t mode;
    uint32_t command_seq;
    uint64_t issued_ts_ns;
} iems_bess_pcs_command_t;

typedef struct {
    iems_bess_port_status_t status;
    uint64_t accepted_ts_ns; /* accepted at the port, not executed on the device */
} iems_bess_pcs_ack_t;

typedef struct {
    bool online;
    bool onoff_acked;
    bool valid;
    uint32_t fault_bits; /* one bit per device fault source; bits 0..30 */
    double measured_active_kw;
    double max_charge_kw;     /* non-negative magnitude */
    double max_discharge_kw;  /* non-negative magnitude */
    uint64_t sample_ts_ns;
} iems_bess_pcs_status_t;

typedef struct {
    void *ctx;
    iems_bess_port_status_t (*submit)(void *ctx, const iems_bess_pcs_command_t *cmd,
                                      iems_bess_pcs_ack_t *ack);
    iems_bess_port_status_t (*read_status)(void *ctx, uint8_t cabinet_id,
                                           iems_bess_pcs_status_t *out);
} iems_bess_pcs_port_t;

typedef struct {
    bool online;
    bool valid;
    double soc_pct;
    double max_charge_kw;    /* non-negative magnitude */
    double max_discharge_kw; /* non-negative magnitude */
    uint32_t fault_bits;     /* one bit per device fault source; bits 0..30 */
    uint64_t sample_ts_ns;
} iems_bess_bms_sample_t;

typedef struct {
    void *ctx;
    /* Caller supplies samples[IEMS_BESS_MAX_CABINETS], dense cabinet order.
     * The adapter increments out_epoch for every complete snapshot. */
    iems_bess_port_status_t (*read_all)(void *ctx, iems_bess_bms_sample_t *samples,
                                        uint8_t capacity, uint8_t *out_count,
                                        uint64_t *out_epoch);
} iems_bess_bms_port_t;

typedef struct {
    void *ctx;
    uint64_t (*now_ns)(void *ctx); /* monotonic clock; never wall time */
} iems_bess_clock_port_t;

typedef struct {
    iems_bess_pcs_port_t pcs;
    iems_bess_bms_port_t bms;
    iems_bess_clock_port_t clock;
} iems_bess_ports_t;

#ifdef __cplusplus
}
#endif

#endif /* IEMS_BESS_PORTS_H */
