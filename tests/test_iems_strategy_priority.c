/**
 * @file test_iems_strategy_priority.c
 * @brief Behavior tests for the pure EMS / per-LC key-precedence resolver.
 *
 * Expected values come from the P5-03d ticket 业务规则 and 数据形状:
 * ems current > deprecated > ABSENT, lc override > base > ABSENT, ABSENT is
 * not 0, conflicts are reported explicitly and never averaged, and a supplied
 * deprecated key marks deprecated_used. They are not derived from a run of the
 * implementation.
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "iems_strategy_priority.h"

#define EPS 1e-12

static const char *source_name(iems_prio_source_t source) {
    switch (source) {
        case IEMS_PRIO_SOURCE_ABSENT:
            return "ABSENT";
        case IEMS_PRIO_SOURCE_CURRENT:
            return "CURRENT";
        case IEMS_PRIO_SOURCE_DEPRECATED:
            return "DEPRECATED";
        case IEMS_PRIO_SOURCE_LC_OVERRIDE:
            return "LC_OVERRIDE";
        case IEMS_PRIO_SOURCE_LC_BASE:
            return "LC_BASE";
        default:
            return "?";
    }
}

static const char *error_name(iems_prio_error_t error) {
    switch (error) {
        case IEMS_PRIO_OK:
            return "OK";
        case IEMS_PRIO_ABSENT:
            return "ABSENT";
        case IEMS_PRIO_BAD_INPUT:
            return "BAD_INPUT";
        default:
            return "?";
    }
}

static const char *key_text(const double *key, char *buf, size_t len) {
    if (key == NULL) return "ABSENT";
    snprintf(buf, len, "%.17g", *key);
    return buf;
}

static void log_result(const char *kind, const char *name, const double *high,
                       const double *low, iems_prio_error_t rc,
                       const iems_prio_result_t *out) {
    char high_buf[40];
    char low_buf[40];
    printf("[priority] %s %-32s high=%s low=%s -> rc=%s value=%.17g source=%s "
           "deprecated_used=%d conflict=%d\n",
           kind, name, key_text(high, high_buf, sizeof high_buf),
           key_text(low, low_buf, sizeof low_buf), error_name(rc), out->value,
           source_name(out->source), out->deprecated_used, out->conflict);
}

static void assert_reset(const iems_prio_result_t *out) {
    assert_float_equal(out->value, 0.0, EPS);
    assert_int_equal(out->source, IEMS_PRIO_SOURCE_ABSENT);
    assert_false(out->deprecated_used);
    assert_false(out->conflict);
}

/* A BAD_INPUT result must overwrite a populated out, not leave stale fields;
 * without this sentinel a resolver that only returns the code still passes. */
static void set_sentinel(iems_prio_result_t *out) {
    out->value = 12345.0;
    out->source = IEMS_PRIO_SOURCE_CURRENT;
    out->deprecated_used = true;
    out->conflict = true;
}

static void test_ems_current_and_deprecated_equal(void **state) {
    (void)state;
    const double current = 10.0;
    const double deprecated = 10.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(&current, &deprecated, &out);
    log_result("ems", "current+deprecated equal", &current, &deprecated, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_float_equal(out.value, 10.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_CURRENT);
    assert_true(out.deprecated_used);
    assert_false(out.conflict);
}

static void test_ems_current_and_deprecated_conflict(void **state) {
    (void)state;
    const double current = 10.0;
    const double deprecated = 20.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(&current, &deprecated, &out);
    log_result("ems", "current+deprecated conflict", &current, &deprecated, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    /* Higher priority wins; the forbidden behavior is averaging to 15. */
    assert_float_equal(out.value, 10.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_CURRENT);
    assert_true(out.deprecated_used);
    assert_true(out.conflict);
}

static void test_ems_current_only(void **state) {
    (void)state;
    const double current = 7.5;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(&current, NULL, &out);
    log_result("ems", "current only", &current, NULL, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_float_equal(out.value, 7.5, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_CURRENT);
    assert_false(out.deprecated_used);
    assert_false(out.conflict);
}

static void test_ems_deprecated_only(void **state) {
    (void)state;
    const double deprecated = 20.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(NULL, &deprecated, &out);
    log_result("ems", "deprecated only", NULL, &deprecated, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_float_equal(out.value, 20.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_DEPRECATED);
    assert_true(out.deprecated_used);
    assert_false(out.conflict);
}

static void test_ems_absent(void **state) {
    (void)state;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(NULL, NULL, &out);
    log_result("ems", "both absent", NULL, NULL, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_ABSENT);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_ABSENT);
    assert_reset(&out);
}

static void test_ems_zero_is_not_absent(void **state) {
    (void)state;
    const double zero = 0.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(&zero, NULL, &out);
    log_result("ems", "current zero", &zero, NULL, rc, &out);
    /* A configured 0 is a value; only the return code may say ABSENT. */
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_float_equal(out.value, 0.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_CURRENT);
}

static void test_ems_bad_input(void **state) {
    (void)state;
    const double valid = 5.0;
    const double nan_value = NAN;
    const double inf_value = INFINITY;
    iems_prio_result_t out;

    assert_int_equal(iems_strategy_priority_resolve_ems(&valid, NULL, NULL),
                     IEMS_PRIO_BAD_INPUT);

    set_sentinel(&out);
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(&nan_value, NULL, &out);
    log_result("ems", "NaN current", &nan_value, NULL, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_BAD_INPUT);
    assert_reset(&out);

    set_sentinel(&out);
    rc = iems_strategy_priority_resolve_ems(NULL, &inf_value, &out);
    log_result("ems", "Inf deprecated", NULL, &inf_value, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_BAD_INPUT);
    assert_reset(&out);

    /* A finite current does not rescue a non-finite deprecated key. */
    set_sentinel(&out);
    rc = iems_strategy_priority_resolve_ems(&valid, &nan_value, &out);
    log_result("ems", "finite current + NaN deprecated", &valid, &nan_value, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_BAD_INPUT);
    assert_reset(&out);
}

static void test_lc_override_and_base_equal(void **state) {
    (void)state;
    const double override_value = 30.0;
    const double base = 30.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_lc(&override_value, &base, &out);
    log_result("lc", "override+base equal", &override_value, &base, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_float_equal(out.value, 30.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_LC_OVERRIDE);
    assert_false(out.deprecated_used);
    assert_false(out.conflict);
}

static void test_lc_override_and_base_conflict(void **state) {
    (void)state;
    const double override_value = 30.0;
    const double base = 40.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_lc(&override_value, &base, &out);
    log_result("lc", "override+base conflict", &override_value, &base, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    /* Override wins outright; never 35 or 40. */
    assert_float_equal(out.value, 30.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_LC_OVERRIDE);
    assert_false(out.deprecated_used);
    assert_true(out.conflict);
}

static void test_lc_override_only(void **state) {
    (void)state;
    const double override_value = 30.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_lc(&override_value, NULL, &out);
    log_result("lc", "override only", &override_value, NULL, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_float_equal(out.value, 30.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_LC_OVERRIDE);
    assert_false(out.conflict);
}

static void test_lc_base_only(void **state) {
    (void)state;
    const double base = 40.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_lc(NULL, &base, &out);
    log_result("lc", "base only", NULL, &base, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_float_equal(out.value, 40.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_LC_BASE);
    assert_false(out.deprecated_used);
    assert_false(out.conflict);
}

static void test_lc_absent(void **state) {
    (void)state;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_lc(NULL, NULL, &out);
    log_result("lc", "both absent", NULL, NULL, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_ABSENT);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_ABSENT);
    assert_reset(&out);
}

static void test_lc_zero_is_not_absent(void **state) {
    (void)state;
    const double zero = 0.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_lc(NULL, &zero, &out);
    log_result("lc", "base zero", NULL, &zero, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_float_equal(out.value, 0.0, EPS);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_LC_BASE);
}

static void test_lc_bad_input(void **state) {
    (void)state;
    const double valid = 5.0;
    const double nan_value = NAN;
    const double inf_value = INFINITY;
    iems_prio_result_t out;

    assert_int_equal(iems_strategy_priority_resolve_lc(&valid, NULL, NULL),
                     IEMS_PRIO_BAD_INPUT);

    set_sentinel(&out);
    iems_prio_error_t rc = iems_strategy_priority_resolve_lc(&nan_value, &valid, &out);
    log_result("lc", "NaN override", &nan_value, &valid, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_BAD_INPUT);
    assert_reset(&out);

    set_sentinel(&out);
    rc = iems_strategy_priority_resolve_lc(NULL, &inf_value, &out);
    log_result("lc", "Inf base", NULL, &inf_value, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_BAD_INPUT);
    assert_reset(&out);
}

/* Sub-1e-9 differences must still be conflicts: any epsilon comparison would
 * silently average away a real key disagreement. */
static void test_ems_tiny_exact_conflict(void **state) {
    (void)state;
    const double current = 1e-300;
    const double deprecated = 2e-300;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(&current, &deprecated, &out);
    log_result("ems", "1e-300 vs 2e-300", &current, &deprecated, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_true(out.value == 1e-300);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_CURRENT);
    assert_true(out.deprecated_used);
    assert_true(out.conflict);
}

static void test_lc_nextafter_exact_conflict(void **state) {
    (void)state;
    const double override_value = 1.0;
    const double base = nextafter(1.0, 2.0);
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_lc(&override_value, &base, &out);
    log_result("lc", "1.0 vs nextafter(1.0,2.0)", &override_value, &base, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_true(out.value == 1.0);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_LC_OVERRIDE);
    assert_false(out.deprecated_used);
    assert_true(out.conflict);
}

/* +0.0 and -0.0 compare equal under == (not bitwise), so they are not a
 * conflict; the higher-priority key still supplies the value. */
static void test_ems_signed_zero_is_not_conflict(void **state) {
    (void)state;
    const double current = 0.0;
    const double deprecated = -0.0;
    iems_prio_result_t out;
    iems_prio_error_t rc = iems_strategy_priority_resolve_ems(&current, &deprecated, &out);
    log_result("ems", "+0.0 vs -0.0", &current, &deprecated, rc, &out);
    assert_int_equal(rc, IEMS_PRIO_OK);
    assert_int_equal(out.source, IEMS_PRIO_SOURCE_CURRENT);
    assert_true(out.deprecated_used);
    assert_false(out.conflict);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ems_current_and_deprecated_equal),
        cmocka_unit_test(test_ems_current_and_deprecated_conflict),
        cmocka_unit_test(test_ems_current_only),
        cmocka_unit_test(test_ems_deprecated_only),
        cmocka_unit_test(test_ems_absent),
        cmocka_unit_test(test_ems_zero_is_not_absent),
        cmocka_unit_test(test_ems_bad_input),
        cmocka_unit_test(test_ems_tiny_exact_conflict),
        cmocka_unit_test(test_ems_signed_zero_is_not_conflict),
        cmocka_unit_test(test_lc_override_and_base_equal),
        cmocka_unit_test(test_lc_override_and_base_conflict),
        cmocka_unit_test(test_lc_override_only),
        cmocka_unit_test(test_lc_base_only),
        cmocka_unit_test(test_lc_absent),
        cmocka_unit_test(test_lc_zero_is_not_absent),
        cmocka_unit_test(test_lc_bad_input),
        cmocka_unit_test(test_lc_nextafter_exact_conflict),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
