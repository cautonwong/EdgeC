/**
 * @file iems_bess_aggregate.h
 * @brief BESS cabinet aggregate root: opaque handle, fixed 20-cabinet storage,
 *        arbitration + power allocation state machine.
 *
 * Owner thread only: every function below must be called from the iEMS libuv
 * loop thread that owns the aggregate. No locking is performed. Other threads
 * marshal through the composition root.
 *
 * The aggregate owns no device objects and allocates no heap; PCS/BMS state is
 * embedded by value and entered only through the consumer-defined ports.
 */

#ifndef IEMS_BESS_AGGREGATE_H
#define IEMS_BESS_AGGREGATE_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "iems_bess_ports.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IEMS_BESS_MAX_CABINETS 20

typedef struct iems_bess_s iems_bess_t;

/* shortcut: 8 KiB placeholder; P5-01c tightens to the measured
 * sizeof(struct iems_bess_s). */
typedef struct {
    alignas(max_align_t) uint8_t raw[8192];
} iems_bess_storage_t;

typedef enum {
    IEMS_BESS_POLICY_REDISTRIBUTE = 0,
    IEMS_BESS_POLICY_EQUAL_SPLIT_NO_REMAINDER = 1 /* v3 comparison only */
} iems_bess_alloc_policy_t;

typedef enum {
    IEMS_BESS_OK = 0,
    IEMS_BESS_PARTIAL = 1,
    IEMS_BESS_SHORTFALL = 2,
    IEMS_BESS_NO_ELIGIBLE = 3,
    IEMS_BESS_TRIPPED = -2,
    IEMS_BESS_NO_DATA = -3,
    IEMS_BESS_WATCHDOG = -4,
    IEMS_BESS_INVALID = -5,
    IEMS_BESS_STALE_REQUEST = -6
} iems_bess_status_t;

typedef enum {
    IEMS_BESS_STATE_INIT = 0,
    IEMS_BESS_STATE_READY,
    IEMS_BESS_STATE_DISPATCHING,
    IEMS_BESS_STATE_DEGRADED,
    IEMS_BESS_STATE_TRIPPED
} iems_bess_state_t;

typedef enum {
    IEMS_BESS_CAB_READY = 0,
    IEMS_BESS_CAB_LOW_SOC,
    IEMS_BESS_CAB_OFFLINE,
    IEMS_BESS_CAB_OFF,
    IEMS_BESS_CAB_BLOCKED_CHARGE,
    IEMS_BESS_CAB_BLOCKED_DISCHARGE,
    IEMS_BESS_CAB_FAULT,
    IEMS_BESS_CAB_FAULT_LATCHED,
    IEMS_BESS_CAB_COMMAND_FAILED,
    IEMS_BESS_CAB_STALE
} iems_bess_cab_state_t;

typedef enum {
    IEMS_BESS_REASON_OK = 0,
    IEMS_BESS_REASON_LIMIT,
    IEMS_BESS_REASON_LOW_SOC,
    IEMS_BESS_REASON_FAULT,
    IEMS_BESS_REASON_OFFLINE,
    IEMS_BESS_REASON_STALE,
    IEMS_BESS_REASON_OFF,
    IEMS_BESS_REASON_SOC_FULL,
    IEMS_BESS_REASON_SOC_EMPTY,
    IEMS_BESS_REASON_CMD_FAILED,
    IEMS_BESS_REASON_TRIPPED
} iems_bess_reason_t;

/* Site fault mask bits. The aggregate only consumes the mask; the composition
 * root maps fire / e-stop / system fault / BMS L3 / watchdog state into it. */
#define IEMS_BESS_SITE_FAULT_FIRE (1u << 0)
#define IEMS_BESS_SITE_FAULT_ESTOP (1u << 1)
#define IEMS_BESS_SITE_FAULT_SYSTEM (1u << 2)
#define IEMS_BESS_SITE_FAULT_BMS_L3 (1u << 3)
#define IEMS_BESS_SITE_FAULT_WATCHDOG (1u << 4)

typedef struct {
    double min, min_back, max_back, max; /* 0<=min<min_back<=max_back<max<=100 */
} iems_bess_soc_window_t;

typedef struct {
    uint8_t cabinet_id;
    double target_kw;
    iems_bess_cab_state_t state;
    iems_bess_reason_t reason;
    iems_bess_port_status_t submit_status;
} iems_bess_cab_result_t;

/* Public result DTO; not the aggregate layout. `allocated_kw` is the sum of
 * every cabinet target including the low SOC reserve, `shortfall_kw` is the
 * unmet requested magnitude (>=0), `reserved_low_soc_kw` is the charge
 * magnitude reserved for low SOC cabinets (>=0). When status is
 * OK/SHORTFALL/NO_ELIGIBLE and `low_soc_override` is false the ledger holds as
 * |requested| = |allocated| + shortfall. PARTIAL excludes failed submits from
 * `allocated` without turning them into shortfall. */
typedef struct {
    iems_bess_status_t status;
    iems_bess_state_t state; /* machine state after this cycle (design 2.2) */
    double requested_kw, allocated_kw, shortfall_kw, reserved_low_soc_kw;
    bool low_soc_override;
    uint32_t command_seq;
    uint64_t request_id, input_epoch, config_epoch;
    uint8_t cabinet_count;
    iems_bess_cab_result_t cabinet[IEMS_BESS_MAX_CABINETS];
} iems_bess_result_t;

/* Validation contract (every rejected field fails create_in instead of being
 * clamped): count 1..20; lc_no non-empty, at most 15 chars, unique; SOC window
 * ordered and within 0..100; low_soc_threshold_pct in 0..100, low_soc_power_kw
 * >= 0; heartbeat_ms > 0, deadband_kw >= 0, bms_stale_ms > 0,
 * command_timeout_ms > 0, command_fail_limit >= 1; policy in range. */
typedef struct {
    uint8_t cabinet_count; /* 1..20 */
    struct {
        char lc_no[16];
    } cabinet[IEMS_BESS_MAX_CABINETS];
    iems_bess_soc_window_t soc_window;
    double low_soc_threshold_pct, low_soc_power_kw;
    bool low_soc_enable;
    double heartbeat_ms, deadband_kw, bms_stale_ms, command_timeout_ms;
    uint8_t command_fail_limit;
    iems_bess_alloc_policy_t policy;
} iems_bess_config_t;

/* Returns NULL on any invalid config or missing port function; never leaves a
 * half-initialized instance. The same storage may be reused after destroy. */
iems_bess_t *iems_bess_create_in(iems_bess_storage_t *storage,
                                 const iems_bess_config_t *cfg,
                                 iems_bess_ports_t ports);

/* Idempotent; NULL-safe; does not free caller storage. */
void iems_bess_destroy(iems_bess_t *bess);

/* Power command. Returns 0 for OK/PARTIAL/SHORTFALL/NO_ELIGIBLE (the command
 * ran, detail is in the result) and a negative iems_bess_status_t for safe
 * rejects (TRIPPED/WATCHDOG/NO_DATA/INVALID/STALE_REQUEST).
 *
 * Idempotency: the same (request_id, requested_kw) returns the cached result
 * without touching any port. An older request_id returns STALE_REQUEST; the
 * same id with a different value returns INVALID. On INVALID/STALE_REQUEST
 * *out is left untouched. */
int iems_bess_request_power(iems_bess_t *bess, double requested_kw,
                            uint64_t request_id, iems_bess_result_t *out);

/* Re-collects snapshots and re-applies the last applied power command (0 when
 * none). Does not change the idempotency cache. */
int iems_bess_tick(iems_bess_t *bess, iems_bess_result_t *out);

int iems_bess_set_onoff(iems_bess_t *bess, bool on, uint64_t request_id,
                        iems_bess_result_t *out);
int iems_bess_set_connect(iems_bess_t *bess, bool grid_connected,
                          uint64_t request_id, iems_bess_result_t *out);

/* Latches a site trip and pushes zero to every cabinet on the 0->1 transition.
 * Clearing the mask does not clear the latch. */
int iems_bess_set_site_fault(iems_bess_t *bess, uint32_t fault_bits);

/* Clears the sticky fault set of one cabinet (1..cabinet_count). */
int iems_bess_reset_faults(iems_bess_t *bess, uint8_t cabinet_id);

/* Clears the site trip only when the fault mask is already zero; returns
 * IEMS_BESS_TRIPPED while bits remain. */
int iems_bess_reset_site_faults(iems_bess_t *bess);

int iems_bess_set_soc_window(iems_bess_t *bess, const iems_bess_soc_window_t *w);
int iems_bess_set_low_soc(iems_bess_t *bess, bool enable,
                          double threshold_pct, double power_kw);

/* Copies the most recent result; returns IEMS_BESS_INVALID when there is none. */
int iems_bess_get_last_result(const iems_bess_t *bess, iems_bess_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_BESS_AGGREGATE_H */
