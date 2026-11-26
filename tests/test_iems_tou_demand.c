/**
 * @file test_iems_tou_demand.c
 * @brief Behavior tests for the pure TOU/price/demand strategy module.
 *
 * Expected values come from iems_v3 get_current_plan / ems_demand_* rules, not
 * from a run of the implementation.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "iems_tou_demand.h"

#define SEC(h, m) ((uint32_t)(h) * 3600u + (uint32_t)(m) * 60u)
#define EPS 1e-9

static iems_td_config_t base_config(void) {
    iems_td_config_t cfg = {0};
    cfg.soc_min_pct = 10.0;
    cfg.soc_max_pct = 95.0;
    cfg.max_charge_kw = 200.0;
    cfg.max_discharge_kw = 200.0;
    return cfg;
}

static iems_td_input_t at_time(uint32_t second_of_day) {
    iems_td_input_t in = {0};
    in.second_of_day = second_of_day;
    in.month = 1;
    in.soc_pct = 50.0;
    return in;
}

static void test_period_boundaries(void** state) {
    (void)state;

    const iems_td_period_t day = {.start_sec = SEC(8, 0),
                                  .end_sec = SEC(11, 30),
                                  .target_power_kw = 100.0,
                                  .soc_max_pct = 100.0};
    /* v3 compares now > start && now <= end: start instant excluded, end included. */
    assert_int_equal(iems_td_find_period(&day, 1, SEC(8, 0)), -1);
    assert_int_equal(iems_td_find_period(&day, 1, SEC(8, 0) + 1), 0);
    assert_int_equal(iems_td_find_period(&day, 1, SEC(11, 30)), 0);
    assert_int_equal(iems_td_find_period(&day, 1, SEC(11, 30) + 1), -1);
    assert_int_equal(iems_td_find_period(&day, 1, SEC(7, 59)), -1);

    const iems_td_period_t cross = {.start_sec = SEC(22, 0),
                                    .end_sec = SEC(6, 0),
                                    .target_power_kw = -50.0,
                                    .soc_max_pct = 100.0};
    assert_int_equal(iems_td_find_period(&cross, 1, SEC(23, 0)), 0);
    assert_int_equal(iems_td_find_period(&cross, 1, SEC(0, 0)), 0);
    assert_int_equal(iems_td_find_period(&cross, 1, SEC(6, 0)), 0);
    assert_int_equal(iems_td_find_period(&cross, 1, SEC(6, 0) + 1), -1);
    assert_int_equal(iems_td_find_period(&cross, 1, SEC(22, 0)), -1);
    assert_int_equal(iems_td_find_period(&cross, 1, SEC(21, 59) + 59), -1);

    /* 00:00-00:00 is v3's invalid slot, never matched. */
    const iems_td_period_t invalid = {.start_sec = 0, .end_sec = 0, .target_power_kw = 10.0};
    assert_int_equal(iems_td_find_period(&invalid, 1, SEC(12, 0)), -1);
    assert_int_equal(iems_td_find_period(&invalid, 1, 0), -1);

    /* First match wins. */
    const iems_td_period_t overlap[2] = {
        {.start_sec = SEC(8, 0), .end_sec = SEC(12, 0), .target_power_kw = 100.0},
        {.start_sec = SEC(9, 0), .end_sec = SEC(10, 0), .target_power_kw = -10.0}};
    assert_int_equal(iems_td_find_period(overlap, 2, SEC(9, 30)), 0);

    assert_int_equal(iems_td_find_period(NULL, 3, SEC(9, 0)), -1);
}

static void test_price_lookup(void** state) {
    (void)state;

    const iems_td_price_row_t rows[3] = {{1.0, 2.0, 3.0, 4.0},
                                         {5.0, 6.0, 7.0, 8.0},
                                         {9.0, 10.0, 11.0, 12.0}};
    const iems_td_price_table_t table = {.rows = rows, .count = 3};
    double rate = -1.0;

    assert_true(iems_td_price_lookup(&table, 1, IEMS_TD_PRICE_VALLEY, &rate));
    assert_float_equal(rate, 4.0, EPS);
    assert_true(iems_td_price_lookup(&table, 2, IEMS_TD_PRICE_TOP, &rate));
    assert_float_equal(rate, 5.0, EPS);
    assert_true(iems_td_price_lookup(&table, 3, IEMS_TD_PRICE_FLAT, &rate));
    assert_float_equal(rate, 11.0, EPS);
    assert_true(iems_td_price_lookup(&table, 1, IEMS_TD_PRICE_PEAK, &rate));
    assert_float_equal(rate, 2.0, EPS);

    rate = 42.0;
    assert_false(iems_td_price_lookup(&table, 0, IEMS_TD_PRICE_PEAK, &rate));
    assert_float_equal(rate, 42.0, EPS);
    assert_false(iems_td_price_lookup(&table, 4, IEMS_TD_PRICE_PEAK, &rate));
    assert_false(iems_td_price_lookup(&table, 1, IEMS_TD_PRICE_NONE, &rate));
    assert_false(iems_td_price_lookup(NULL, 1, IEMS_TD_PRICE_PEAK, &rate));
    assert_false(iems_td_price_lookup(&table, 1, IEMS_TD_PRICE_PEAK, NULL));
}

static void test_tou_soc_and_clamp(void** state) {
    (void)state;

    const iems_td_period_t periods[] = {
        {.start_sec = SEC(0, 0), .end_sec = SEC(6, 0), .target_power_kw = -100.0,
         .soc_max_pct = 100.0},
        {.start_sec = SEC(8, 0), .end_sec = SEC(12, 0), .target_power_kw = 300.0,
         .soc_max_pct = 100.0},
        {.start_sec = SEC(13, 0), .end_sec = SEC(14, 0), .target_power_kw = -100.0,
         .soc_max_pct = 80.0},
        {.start_sec = SEC(15, 0), .end_sec = SEC(16, 0), .target_power_kw = 100.0,
         .soc_max_pct = 100.0, .soc_min_pct = 20.0},
    };
    iems_td_config_t cfg = base_config();
    cfg.periods = periods;
    cfg.period_count = sizeof(periods) / sizeof(periods[0]);
    cfg.max_charge_kw = 60.0;
    cfg.max_discharge_kw = 200.0;

    iems_td_input_t in = at_time(SEC(2, 0));
    iems_td_decision_t d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, -60.0, EPS);
    assert_int_equal(d.action, IEMS_TD_ACTION_CHARGE);
    assert_int_equal(d.reason, IEMS_TD_REASON_TOU_CHARGE);

    in = at_time(SEC(9, 0));
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 200.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_TOU_DISCHARGE);

    in = at_time(SEC(2, 0));
    in.soc_pct = 95.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_SOC_LIMIT);
    assert_int_equal(d.action, IEMS_TD_ACTION_STANDBY);

    in = at_time(SEC(9, 0));
    in.soc_pct = 10.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_SOC_LIMIT);

    /* Period SOC ceiling narrows the global one. */
    in = at_time(SEC(13, 30));
    in.soc_pct = 85.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_SOC_LIMIT);

    /* Period SOC floor narrows the global one. */
    in = at_time(SEC(15, 30));
    in.soc_pct = 15.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_SOC_LIMIT);

    in = at_time(SEC(7, 0));
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_NO_PERIOD);
}

static void test_tou_price_missing(void** state) {
    (void)state;

    const iems_td_period_t period = {.start_sec = SEC(9, 0),
                                     .end_sec = SEC(10, 0),
                                     .target_power_kw = 100.0,
                                     .soc_max_pct = 100.0,
                                     .price_class = IEMS_TD_PRICE_PEAK};
    const iems_td_period_t unpriced = {.start_sec = SEC(9, 0),
                                       .end_sec = SEC(10, 0),
                                       .target_power_kw = 100.0,
                                       .soc_max_pct = 100.0};
    const iems_td_price_row_t rows[1] = {{1.0, 0.7, 0.5, 0.3}};
    iems_td_config_t cfg = base_config();
    cfg.periods = &period;
    cfg.period_count = 1;
    cfg.price_table.rows = rows;
    cfg.price_table.count = 1;

    iems_td_input_t in = at_time(SEC(9, 30));
    in.month = 2;
    iems_td_decision_t d = iems_td_evaluate(&cfg, NULL, &in);
    assert_false(d.price_valid);
    assert_float_equal(d.price_cny_per_kwh, 0.0, EPS);
    assert_float_equal(d.target_power_kw, 100.0, EPS);

    in.month = 1;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_true(d.price_valid);
    assert_float_equal(d.price_cny_per_kwh, 0.7, EPS);
    assert_float_equal(d.target_power_kw, 100.0, EPS);

    cfg.periods = &unpriced;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_false(d.price_valid);

    cfg.periods = &period;
    in.second_of_day = SEC(12, 0);
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_false(d.price_valid);
}

static void test_demand_threshold(void** state) {
    (void)state;

    iems_td_demand_config_t dc = {.mode = IEMS_TD_DEMAND_THRESHOLD,
                                  .limit_kw = 500.0,
                                  .lower_kw = 300.0,
                                  .charge_enabled = true};

    iems_td_demand_input_t din = {.grid_kw = 550.0};
    int8_t next = 0;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 50.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_DISCHARGE);

    /* Between thresholds the previous state holds. */
    din.grid_kw = 490.0;
    assert_float_equal(iems_td_demand_raw(&dc, IEMS_TD_ACTION_DISCHARGE, &din, &next), 0.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_DISCHARGE);
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 0.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_STANDBY);

    /* Crossing the lower threshold switches to charge. */
    din.grid_kw = 250.0;
    assert_float_equal(iems_td_demand_raw(&dc, IEMS_TD_ACTION_DISCHARGE, &din, &next), -50.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_CHARGE);

    /* Charge enable gates output, not the state transition (v3 sets flag=2). */
    dc.charge_enabled = false;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 0.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_CHARGE);
    dc.charge_enabled = true;

    /* v3 uses strict comparisons at both thresholds. */
    din.grid_kw = 500.0;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 0.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_STANDBY);
    din.grid_kw = 300.0;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 0.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_STANDBY);

    /* Full evaluation clamps and gates on SOC. */
    iems_td_config_t cfg = base_config();
    cfg.demand = dc;
    cfg.max_discharge_kw = 30.0;
    iems_td_input_t in = at_time(SEC(12, 0));
    in.grid_kw = 900.0;
    iems_td_decision_t d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 30.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_DEMAND_DISCHARGE);
    assert_true(d.demand_active);

    in.soc_pct = 10.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_SOC_LIMIT);
}

static void test_demand_window(void** state) {
    (void)state;

    iems_td_demand_config_t dc = {.mode = IEMS_TD_DEMAND_WINDOW,
                                  .limit_kw = 500.0,
                                  .charge_threshold_kw = 400.0,
                                  .charge_enabled = true,
                                  .window_min = 15};
    iems_td_demand_input_t din = {.demand_kw = 600.0, .window_elapsed_min = 1};
    int8_t next = 0;

    /* v3: (demand - target) * (elapsed - 1); the first minute has no effect. */
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 0.0, EPS);
    din.window_elapsed_min = 4;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 300.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_DISCHARGE);

    din.demand_kw = 520.0;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 60.0, EPS);

    din.demand_kw = 350.0;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), -150.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_CHARGE);

    din.demand_kw = 390.0;
    dc.charge_enabled = false;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 0.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_STANDBY);
    dc.charge_enabled = true;

    /* Elapsed minutes clamp to the window, and 0 behaves like minute 1. */
    din.demand_kw = 600.0;
    din.window_elapsed_min = 20;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 1400.0, EPS);
    din.window_elapsed_min = 0;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 0.0, EPS);

    /* v3 falls back to 15 minutes when the window is unset. */
    dc.window_min = 0;
    din.window_elapsed_min = 16;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 1400.0, EPS);

    /* Full evaluation clamps both directions. */
    dc.window_min = 15;
    din.window_elapsed_min = 15;
    iems_td_config_t cfg = base_config();
    cfg.demand = dc;
    cfg.max_discharge_kw = 200.0;
    cfg.max_charge_kw = 200.0;
    iems_td_input_t in = at_time(SEC(12, 0));
    in.demand_kw = 600.0;
    in.window_elapsed_min = 15;
    iems_td_decision_t d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 200.0, EPS);

    in.demand_kw = 0.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, -200.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_DEMAND_CHARGE);
}

static void test_demand_limit(void** state) {
    (void)state;

    const iems_td_demand_config_t dc = {.mode = IEMS_TD_DEMAND_LIMIT, .limit_kw = 500.0};
    iems_td_demand_input_t din = {.grid_kw = 550.0, .demand_kw = 400.0};
    int8_t next = 0;

    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 50.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_DISCHARGE);

    din.grid_kw = 400.0;
    din.demand_kw = 550.0;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 50.0, EPS);

    din.demand_kw = 400.0;
    assert_float_equal(iems_td_demand_raw(&dc, 0, &din, &next), 0.0, EPS);
    assert_int_equal(next, IEMS_TD_ACTION_STANDBY);

    /* Raw is unclamped; the evaluator applies the discharge limit. */
    iems_td_config_t cfg = base_config();
    cfg.demand = dc;
    cfg.max_discharge_kw = 30.0;
    iems_td_input_t in = at_time(SEC(12, 0));
    in.grid_kw = 900.0;
    in.demand_kw = 900.0;
    iems_td_decision_t d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 30.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_DEMAND_DISCHARGE);
}

static void test_window_accumulator(void** state) {
    (void)state;

    double sum = 0.0;
    uint32_t samples = 0;
    assert_float_equal(iems_td_window_average(sum, samples), 0.0, EPS);

    iems_td_window_add(&sum, &samples, 100.0, 9000);
    assert_float_equal(sum, 100.0, EPS);
    assert_int_equal(samples, 1);
    assert_float_equal(iems_td_window_average(sum, samples), 100.0, EPS);

    iems_td_window_add(&sum, &samples, -50.0, 9000);
    assert_float_equal(sum, 100.0, EPS);
    assert_int_equal(samples, 2);

    iems_td_window_add(&sum, &samples, 100.0, 3);
    assert_float_equal(sum, 200.0, EPS);
    assert_int_equal(samples, 3);

    /* Fourth sample trips the halving rule before the average is read. */
    iems_td_window_add(&sum, &samples, 100.0, 3);
    assert_float_equal(sum, 150.0, EPS);
    assert_int_equal(samples, 2);
    assert_float_equal(iems_td_window_average(sum, samples), 75.0, EPS);

    iems_td_window_add(NULL, NULL, 100.0, 9000);
}

static void test_combine_and_reasons(void** state) {
    (void)state;

    const iems_td_period_t periods[] = {
        {.start_sec = SEC(2, 0), .end_sec = SEC(6, 0), .target_power_kw = -100.0,
         .soc_max_pct = 100.0},
        {.start_sec = SEC(9, 0), .end_sec = SEC(11, 0), .target_power_kw = 100.0,
         .soc_max_pct = 100.0},
    };
    iems_td_config_t cfg = base_config();
    cfg.periods = periods;
    cfg.period_count = 2;
    cfg.demand.mode = IEMS_TD_DEMAND_LIMIT;
    cfg.demand.limit_kw = 500.0;
    cfg.demand.charge_enabled = true;

    iems_td_input_t in = at_time(SEC(9, 30));
    in.grid_kw = 650.0;
    in.demand_kw = 650.0;
    iems_td_decision_t d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 150.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_DEMAND_DISCHARGE);
    assert_true(d.demand_active);

    in.grid_kw = 520.0;
    in.demand_kw = 520.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 100.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_TOU_DISCHARGE);
    assert_true(d.demand_active);

    in.grid_kw = 400.0;
    in.demand_kw = 400.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 100.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_TOU_DISCHARGE);
    assert_false(d.demand_active);

    /* Window demand can add charge to a charge period. */
    cfg.demand.mode = IEMS_TD_DEMAND_WINDOW;
    cfg.demand.window_min = 15;
    cfg.demand.charge_threshold_kw = 400.0;
    in = at_time(SEC(3, 0));
    in.demand_kw = 350.0;
    in.window_elapsed_min = 4;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, -150.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_DEMAND_CHARGE);

    in.demand_kw = 450.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, -100.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_TOU_CHARGE);

    /* A demand charge target never flips an active TOU discharge. */
    in = at_time(SEC(9, 30));
    in.demand_kw = 350.0;
    in.window_elapsed_min = 4;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 100.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_TOU_DISCHARGE);
    assert_int_equal(d.action, IEMS_TD_ACTION_DISCHARGE);
    assert_true(d.demand_active);

    /* SOC gate wins over demand, but the demand is still reported active. */
    cfg.demand.mode = IEMS_TD_DEMAND_LIMIT;
    in = at_time(SEC(9, 30));
    in.grid_kw = 650.0;
    in.demand_kw = 650.0;
    in.soc_pct = 10.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_SOC_LIMIT);
    assert_true(d.demand_active);

    /* No TOU period: the demand decision stands alone. */
    in = at_time(SEC(12, 0));
    in.grid_kw = 550.0;
    in.demand_kw = 550.0;
    in.soc_pct = 50.0;
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_float_equal(d.target_power_kw, 50.0, EPS);
    assert_int_equal(d.reason, IEMS_TD_REASON_DEMAND_DISCHARGE);
}

static void test_idempotent(void** state) {
    (void)state;

    iems_td_demand_config_t dc = {.mode = IEMS_TD_DEMAND_THRESHOLD,
                                  .limit_kw = 500.0,
                                  .lower_kw = 300.0,
                                  .charge_enabled = true};
    iems_td_config_t cfg = base_config();
    cfg.demand = dc;

    iems_td_state_t st = {.demand_action = IEMS_TD_ACTION_DISCHARGE};
    iems_td_input_t in = at_time(SEC(12, 0));
    in.grid_kw = 490.0;

    iems_td_decision_t first = iems_td_evaluate(&cfg, &st, &in);
    for (int i = 0; i < 100; i++) {
        iems_td_decision_t again = iems_td_evaluate(&cfg, &st, &in);
        assert_float_equal(again.target_power_kw, first.target_power_kw, EPS);
        assert_int_equal(again.reason, first.reason);
        assert_int_equal(again.action, first.action);
        assert_int_equal(again.next_state.demand_action, first.next_state.demand_action);
    }

    /* Feeding the returned state back converges to the same decision. */
    iems_td_decision_t next = iems_td_evaluate(&cfg, &first.next_state, &in);
    assert_float_equal(next.target_power_kw, first.target_power_kw, EPS);
    assert_int_equal(next.next_state.demand_action, first.next_state.demand_action);
}

static void test_invalid_inputs(void** state) {
    (void)state;

    iems_td_decision_t d = iems_td_evaluate(NULL, NULL, NULL);
    assert_int_equal(d.reason, IEMS_TD_REASON_INVALID);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
    assert_int_equal(d.action, IEMS_TD_ACTION_STANDBY);

    iems_td_config_t cfg = base_config();
    d = iems_td_evaluate(&cfg, NULL, NULL);
    assert_int_equal(d.reason, IEMS_TD_REASON_INVALID);

    iems_td_input_t in = at_time(SEC(12, 0));
    d = iems_td_evaluate(&cfg, NULL, &in);
    assert_int_equal(d.reason, IEMS_TD_REASON_NO_PERIOD);
    assert_float_equal(d.target_power_kw, 0.0, EPS);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_period_boundaries),
        cmocka_unit_test(test_price_lookup),
        cmocka_unit_test(test_tou_soc_and_clamp),
        cmocka_unit_test(test_tou_price_missing),
        cmocka_unit_test(test_demand_threshold),
        cmocka_unit_test(test_demand_window),
        cmocka_unit_test(test_demand_limit),
        cmocka_unit_test(test_window_accumulator),
        cmocka_unit_test(test_combine_and_reasons),
        cmocka_unit_test(test_idempotent),
        cmocka_unit_test(test_invalid_inputs),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
