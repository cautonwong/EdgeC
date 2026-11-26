/**
 * @file test_iems_alarm_state.c
 * @brief P5-04a live alarm table: raise/ack/recover/suppress transitions,
 *        idempotent and no-active no-ops, per-reason identity, capacity, and
 *        the tag-map port.
 */

#include <cmocka.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "iems_alarm_state.h"

static iems_alarm_table_t g_table;

static void reset_table(void) {
    memset(&g_table, 0, sizeof(g_table));
}

static void test_raise_ack_recover_lifecycle(void **state) {
    (void)state;
    reset_table();

    assert_int_equal(iems_alarm_state_raise(&g_table, 8005, 10101, 0), 0);
    const iems_alarm_state_t *row = iems_alarm_state_find(&g_table, 8005, 10101, 0);
    assert_non_null(row);
    assert_true(row->active);
    assert_false(row->acked);
    assert_false(row->suppressed);
    assert_int_equal(row->reason_code, 0);
    assert_int_equal(row->recovered_at, 0);
    assert_int_equal(g_table.count, 1);

    assert_int_equal(iems_alarm_state_ack(&g_table, 8005, 10101, 0), 0);
    assert_true(row->acked);
    assert_int_equal(iems_alarm_state_ack(&g_table, 8005, 10101, 0), 1);
    assert_true(row->acked);

    assert_int_equal(iems_alarm_state_recover(&g_table, 8005, 10101, 0, 1234), 0);
    assert_false(row->active);
    assert_true(row->acked);
    assert_int_equal(row->recovered_at, 1234);

    assert_int_equal(iems_alarm_state_recover(&g_table, 8005, 10101, 0, 9999), 1);
    assert_int_equal(row->recovered_at, 1234);

    /* A new occurrence reactivates and clears the previous acknowledgement. */
    assert_int_equal(iems_alarm_state_raise(&g_table, 8005, 10101, 0), 0);
    assert_true(row->active);
    assert_false(row->acked);
    assert_int_equal(row->recovered_at, 0);
    assert_int_equal(g_table.count, 1);
}

static void test_raise_is_idempotent(void **state) {
    (void)state;
    reset_table();

    assert_int_equal(iems_alarm_state_raise(&g_table, 1, 2, 0), 0);
    assert_int_equal(iems_alarm_state_ack(&g_table, 1, 2, 0), 0);
    assert_int_equal(iems_alarm_state_raise(&g_table, 1, 2, 0), 1);
    assert_int_equal(g_table.count, 1);

    const iems_alarm_state_t *row = iems_alarm_state_find(&g_table, 1, 2, 0);
    assert_non_null(row);
    assert_true(row->active);
    assert_true(row->acked);
    assert_int_equal(row->recovered_at, 0);
}

static void test_recover_without_active_is_noop(void **state) {
    (void)state;
    reset_table();

    assert_int_equal(iems_alarm_state_recover(&g_table, 1, 2, 0, 10), 1);
    assert_int_equal(g_table.count, 0);
    assert_null(iems_alarm_state_find(&g_table, 1, 2, 0));

    /* A suppressed-before-occurrence row is inactive, so recover is a no-op. */
    assert_int_equal(iems_alarm_state_suppress(&g_table, 1, 2, 0), 0);
    assert_int_equal(iems_alarm_state_recover(&g_table, 1, 2, 0, 10), 1);
    const iems_alarm_state_t *row = iems_alarm_state_find(&g_table, 1, 2, 0);
    assert_non_null(row);
    assert_false(row->active);
    assert_int_equal(row->recovered_at, 0);

    assert_int_equal(iems_alarm_state_resume(&g_table, 1, 2, 0), 0);
    assert_int_equal(iems_alarm_state_raise(&g_table, 1, 2, 0), 0);
    assert_int_equal(iems_alarm_state_recover(&g_table, 1, 2, 0, 77), 0);
    assert_int_equal(row->recovered_at, 77);
}

static void test_suppress_and_resume_gate_raise(void **state) {
    (void)state;
    reset_table();

    assert_int_equal(iems_alarm_state_suppress(&g_table, 9, 3, 0), 0);
    const iems_alarm_state_t *row = iems_alarm_state_find(&g_table, 9, 3, 0);
    assert_non_null(row);
    assert_false(row->active);
    assert_true(row->suppressed);
    assert_int_equal(g_table.count, 1);

    assert_int_equal(iems_alarm_state_suppress(&g_table, 9, 3, 0), 1);
    assert_int_equal(iems_alarm_state_raise(&g_table, 9, 3, 0), 1);
    assert_false(row->active);

    assert_int_equal(iems_alarm_state_resume(&g_table, 9, 3, 0), 0);
    assert_false(row->suppressed);
    assert_int_equal(iems_alarm_state_resume(&g_table, 9, 3, 0), 1);

    assert_int_equal(iems_alarm_state_raise(&g_table, 9, 3, 0), 0);
    assert_true(row->active);

    /* Suppression is orthogonal to an already-active occurrence. */
    assert_int_equal(iems_alarm_state_suppress(&g_table, 9, 3, 0), 0);
    assert_true(row->active);
    assert_true(row->suppressed);
    assert_int_equal(iems_alarm_state_recover(&g_table, 9, 3, 0, 20), 0);
    assert_false(row->active);
    assert_true(row->suppressed);
    assert_int_equal(iems_alarm_state_raise(&g_table, 9, 3, 0), 1);
    assert_false(row->active);
}

static void test_same_key_with_different_codes_are_distinct(void **state) {
    (void)state;
    reset_table();

    assert_int_equal(iems_alarm_state_raise(&g_table, 8005, 10101, 0), 0);
    assert_int_equal(iems_alarm_state_raise(&g_table, 8005, 10102, 0), 0);
    assert_int_equal(g_table.count, 2);
    assert_int_equal(iems_alarm_state_ack(&g_table, 8005, 10101, 0), 0);
    assert_true(iems_alarm_state_find(&g_table, 8005, 10101, 0)->acked);
    assert_false(iems_alarm_state_find(&g_table, 8005, 10102, 0)->acked);
    assert_int_equal(iems_alarm_state_recover(&g_table, 8005, 10102, 0, 5), 0);
    assert_true(iems_alarm_state_find(&g_table, 8005, 10101, 0)->active);
    assert_false(iems_alarm_state_find(&g_table, 8005, 10102, 0)->active);
}

static void test_same_key_and_code_with_different_reasons_are_distinct(void **state) {
    (void)state;
    reset_table();

    assert_int_equal(iems_alarm_state_raise(&g_table, 8005, 10101, 1), 0);
    assert_int_equal(iems_alarm_state_raise(&g_table, 8005, 10101, 2), 0);
    assert_int_equal(g_table.count, 2);
    assert_true(iems_alarm_state_find(&g_table, 8005, 10101, 1)->active);
    assert_true(iems_alarm_state_find(&g_table, 8005, 10101, 2)->active);

    assert_int_equal(iems_alarm_state_ack(&g_table, 8005, 10101, 1), 0);
    assert_true(iems_alarm_state_find(&g_table, 8005, 10101, 1)->acked);
    assert_false(iems_alarm_state_find(&g_table, 8005, 10101, 2)->acked);

    assert_int_equal(iems_alarm_state_recover(&g_table, 8005, 10101, 2, 5), 0);
    assert_true(iems_alarm_state_find(&g_table, 8005, 10101, 1)->active);
    assert_false(iems_alarm_state_find(&g_table, 8005, 10101, 2)->active);
    assert_int_equal(iems_alarm_state_find(&g_table, 8005, 10101, 2)->recovered_at, 5);
    assert_int_equal(iems_alarm_state_find(&g_table, 8005, 10101, 1)->recovered_at, 0);

    /* Suppression is per row too. */
    assert_int_equal(iems_alarm_state_suppress(&g_table, 8005, 10101, 3), 0);
    assert_false(iems_alarm_state_find(&g_table, 8005, 10101, 1)->suppressed);
    assert_int_equal(iems_alarm_state_raise(&g_table, 8005, 10101, 3), 1);
}

static void test_capacity_fails_closed(void **state) {
    (void)state;
    reset_table();

    for (uint32_t i = 0; i < IEMS_ALARM_STATE_MAX; i++) {
        assert_int_equal(iems_alarm_state_raise(&g_table, i + 1, 7, 7), 0);
    }
    assert_int_equal(g_table.count, IEMS_ALARM_STATE_MAX);
    assert_int_equal(iems_alarm_state_raise(&g_table, IEMS_ALARM_STATE_MAX + 1, 7, 7), -1);
    assert_int_equal(iems_alarm_state_suppress(&g_table, IEMS_ALARM_STATE_MAX + 2, 7, 7), -1);
    assert_int_equal(g_table.count, IEMS_ALARM_STATE_MAX);
}

static int synthetic_map(void *ctx, const char *class_type, const char *vendor, const char *model,
                         const char *tag, uint32_t *edgec_tag) {
    (void)ctx;
    if (!class_type || !vendor || !model || !tag) return -1;
    if (strcmp(class_type, "PCS") != 0) return -1;
    if (strcmp(vendor, "Livoltek") != 0 || strcmp(model, "PCS") != 0) return -1;
    if (strcmp(tag, "FaultCode1") == 0) {
        *edgec_tag = 8005;
        return 0;
    }
    if (strcmp(tag, "ZeroMapped") == 0) {
        *edgec_tag = 0;
        return 0;
    }
    return -1;
}

static void test_mapped_raise_uses_port(void **state) {
    (void)state;
    reset_table();
    iems_alarm_tag_map_t map = {.map_tag = synthetic_map, .ctx = NULL};

    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "PCS", "Livoltek", "PCS",
                                                   "FaultCode1", 10101, 1),
                     0);
    const iems_alarm_state_t *row = iems_alarm_state_find(&g_table, 8005, 10101, 1);
    assert_non_null(row);
    assert_true(row->active);
    assert_int_equal(g_table.count, 1);

    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "PCS", "Livoltek", "PCS",
                                                   "FaultCode1", 10101, 1),
                     1);
    assert_int_equal(g_table.count, 1);

    /* Same mapped tag and code with another reason is another row. */
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "PCS", "Livoltek", "PCS",
                                                   "FaultCode1", 10101, 2),
                     0);
    assert_non_null(iems_alarm_state_find(&g_table, 8005, 10101, 2));
    assert_int_equal(g_table.count, 2);

    /* The class type reaches the port and is part of the vendor identity. */
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "EMS", "Livoltek", "PCS",
                                                   "FaultCode1", 10101, 1),
                     -1);
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "PCS", "Livoltek", "PCS",
                                                   "Unknown", 10101, 1),
                     -1);
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "PCS", "Other", "PCS",
                                                   "FaultCode1", 10101, 1),
                     -1);
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "PCS", "Livoltek", "PCS",
                                                   "ZeroMapped", 10101, 1),
                     -1);
    assert_int_equal(g_table.count, 2);

    assert_int_equal(iems_alarm_state_raise_mapped(NULL, &map, "PCS", "Livoltek", "PCS",
                                                   "FaultCode1", 1, 0),
                     -1);
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, NULL, "PCS", "Livoltek", "PCS",
                                                   "FaultCode1", 1, 0),
                     -1);
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, NULL, "Livoltek", "PCS",
                                                   "FaultCode1", 1, 0),
                     -1);
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "", "Livoltek", "PCS",
                                                   "FaultCode1", 1, 0),
                     -1);
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "PCS", NULL, "PCS",
                                                   "FaultCode1", 1, 0),
                     -1);
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &map, "PCS", "Livoltek", "PCS", "",
                                                   1, 0),
                     -1);

    iems_alarm_tag_map_t no_map = {.map_tag = NULL, .ctx = NULL};
    assert_int_equal(iems_alarm_state_raise_mapped(&g_table, &no_map, "PCS", "Livoltek", "PCS",
                                                   "FaultCode1", 1, 0),
                     -1);
}

static void test_null_table_is_rejected(void **state) {
    (void)state;
    assert_int_equal(iems_alarm_state_raise(NULL, 1, 2, 0), -1);
    assert_int_equal(iems_alarm_state_ack(NULL, 1, 2, 0), -1);
    assert_int_equal(iems_alarm_state_recover(NULL, 1, 2, 0, 0), -1);
    assert_int_equal(iems_alarm_state_suppress(NULL, 1, 2, 0), -1);
    assert_int_equal(iems_alarm_state_resume(NULL, 1, 2, 0), -1);
    assert_null(iems_alarm_state_find(NULL, 1, 2, 0));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_raise_ack_recover_lifecycle),
        cmocka_unit_test(test_raise_is_idempotent),
        cmocka_unit_test(test_recover_without_active_is_noop),
        cmocka_unit_test(test_suppress_and_resume_gate_raise),
        cmocka_unit_test(test_same_key_with_different_codes_are_distinct),
        cmocka_unit_test(test_same_key_and_code_with_different_reasons_are_distinct),
        cmocka_unit_test(test_capacity_fails_closed),
        cmocka_unit_test(test_mapped_raise_uses_port),
        cmocka_unit_test(test_null_table_is_rejected),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
