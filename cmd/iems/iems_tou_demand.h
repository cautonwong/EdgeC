/**
 * @file iems_tou_demand.h
 * @brief P5-03b/c: pure TOU period/price and demand strategy decisions.
 *
 * Data shape: a caller-resolved day plan is a table of second-of-day periods
 * with a signed target power (positive discharge, negative charge), optional
 * SOC bounds and an optional price class. A demand config selects one demand
 * model (limit, threshold or window). The evaluator takes the clock, one
 * measurement snapshot and the previous threshold state, and returns the
 * decision plus the next state. It performs no IO, reads no clock and keeps no
 * mutable globals, so the same inputs always produce the same decision.
 *
 * Semantics follow iems_v3 get_current_plan (jsonct.c:572): a period matches
 * when now > start && now <= end, or now > start || now <= end when end <=
 * start. A 00:00-00:00 period is invalid. First match wins.
 *
 * Deliberate v3 deviations in this slice:
 * - THRESHOLD latches only on instantaneous-grid threshold crossings. v3's
 *   fixed demand also requires the cabinet PCS output to be 0 before it
 *   (re)latches (strategy_demand_fixed.c:356) and runs xl_antiReflux
 *   anti-overshoot after the decision; neither is implemented here.
 * - WINDOW applies the v3 minute-index formula (demand - target) * (elapsed-1)
 *   but does not implement ems_demand_window_end_treatment
 *   (strategy_demand_ctrl.c:1799), which replaces the last-window-minute
 *   target with an energy-metered estimate.
 */

#ifndef IEMS_TOU_DEMAND_H
#define IEMS_TOU_DEMAND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Decision sign convention: positive discharge, negative charge (正放负充). */
typedef enum {
    IEMS_TD_ACTION_CHARGE = -1,
    IEMS_TD_ACTION_STANDBY = 0,
    IEMS_TD_ACTION_DISCHARGE = 1
} iems_td_action_t;

typedef enum {
    IEMS_TD_PRICE_NONE = 0,
    IEMS_TD_PRICE_TOP = 1,
    IEMS_TD_PRICE_PEAK = 2,
    IEMS_TD_PRICE_FLAT = 3,
    IEMS_TD_PRICE_VALLEY = 4
} iems_td_price_class_t;

typedef struct {
    uint32_t start_sec; /* second of day, 0..86399 */
    uint32_t end_sec;   /* second of day, 0..86400; <= start means cross-midnight */
    double target_power_kw; /* signed; positive discharge, negative charge */
    double soc_min_pct;     /* period narrowing of the global floor, 0..100 */
    double soc_max_pct;     /* period narrowing of the global ceiling, 0..100 */
    iems_td_price_class_t price_class; /* IEMS_TD_PRICE_NONE = unclassified */
} iems_td_period_t;

/* One row of price.json: the monthly top/peak/flat/valley rates. rows[0] is
 * January; iems_td_price_lookup takes month 1..12. */
typedef struct {
    double top;
    double peak;
    double flat;
    double valley;
} iems_td_price_row_t;

typedef struct {
    const iems_td_price_row_t *rows;
    size_t count;
} iems_td_price_table_t;

typedef enum {
    IEMS_TD_DEMAND_OFF = 0,
    /* EdgeC coordinator model: max(instant, window demand) - limit, no charge. */
    IEMS_TD_DEMAND_LIMIT = 1,
    /* iems_v3 fixed demand: instantaneous grid vs upper/lower with state hold. */
    IEMS_TD_DEMAND_THRESHOLD = 2,
    /* iems_v3 dynamic demand: (demand - target) * elapsed window minutes. */
    IEMS_TD_DEMAND_WINDOW = 3
} iems_td_demand_mode_t;

typedef struct {
    iems_td_demand_mode_t mode;
    double limit_kw;            /* LIMIT/WINDOW target demand; THRESHOLD discharge start */
    double lower_kw;            /* THRESHOLD charge start */
    double charge_threshold_kw; /* WINDOW charge start */
    bool charge_enabled;        /* caller applies the calendar charge window */
    uint16_t window_min;        /* WINDOW length; 0 uses v3's 15-minute default */
} iems_td_demand_config_t;

typedef struct {
    double grid_kw;             /* instantaneous grid import, >=0 means import */
    double demand_kw;           /* windowed demand statistic for the same interval */
    uint16_t window_elapsed_min; /* 1..window_min; 0 is treated as 1 */
} iems_td_demand_input_t;

typedef struct {
    int8_t demand_action; /* iems_td_action_t carried across threshold evaluations */
} iems_td_state_t;

typedef struct {
    uint32_t second_of_day; /* caller's clock, 0..86399 */
    uint8_t month;          /* 1..12, used for price lookup */
    double soc_pct;
    double grid_kw;
    double demand_kw;
    uint16_t window_elapsed_min;
} iems_td_input_t;

typedef struct {
    const iems_td_period_t *periods;
    size_t period_count;
    iems_td_price_table_t price_table;
    iems_td_demand_config_t demand;
    double soc_min_pct;
    double soc_max_pct;
    double max_charge_kw;    /* positive magnitude */
    double max_discharge_kw; /* positive magnitude */
} iems_td_config_t;

typedef enum {
    IEMS_TD_REASON_NONE = 0,
    IEMS_TD_REASON_TOU_CHARGE,
    IEMS_TD_REASON_TOU_DISCHARGE,
    IEMS_TD_REASON_DEMAND_CHARGE,
    IEMS_TD_REASON_DEMAND_DISCHARGE,
    IEMS_TD_REASON_SOC_LIMIT,
    IEMS_TD_REASON_NO_PERIOD,
    IEMS_TD_REASON_INVALID
} iems_td_reason_t;

typedef struct {
    double target_power_kw;
    double price_cny_per_kwh;
    iems_td_reason_t reason;
    iems_td_action_t action;
    bool price_valid;
    bool demand_active;
    iems_td_state_t next_state;
} iems_td_decision_t;

/* Returns the first matching period index, or -1 when none matches. */
int iems_td_find_period(const iems_td_period_t *periods, size_t count, uint32_t second_of_day);

/* Returns false and leaves *out_rate untouched when the month row or the
 * price class is absent. */
bool iems_td_price_lookup(const iems_td_price_table_t *table, uint8_t month,
                          iems_td_price_class_t price_class, double *out_rate);

/* Sample-window accumulator. Imports (power_kw > 0) are summed, negative
 * samples only advance the sample count. When samples exceeds max_samples
 * both sum and count halve, matching the 100 ms coordinator window. */
void iems_td_window_add(double *sum_kw, uint32_t *samples, double power_kw, uint32_t max_samples);
double iems_td_window_average(double sum_kw, uint32_t samples);

/* Demand-only target before SOC gating and charge/discharge clamping. Returns
 * a signed kW value and writes the next threshold state through next_action
 * (may be NULL). */
double iems_td_demand_raw(const iems_td_demand_config_t *cfg, int8_t prev_action,
                          const iems_td_demand_input_t *in, int8_t *next_action);

/* Full decision: TOU baseline, demand overlay, SOC gate, clamp, price lookup. */
iems_td_decision_t iems_td_evaluate(const iems_td_config_t *cfg, const iems_td_state_t *state,
                                    const iems_td_input_t *in);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_TOU_DEMAND_H */
