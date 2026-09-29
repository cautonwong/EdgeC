/**
 * @file test_ebpf_guard.c
 * @brief Unit tests for Kernel-Level eBPF Security Guard Operator
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ebpf_guard.h"

static void test_ebpf_guard_init_null(void **state) {
    (void)state;
    ebpf_guard_mgr_storage_t store;
    ebpf_guard_mgr_t *mgr = ebpf_guard_mgr_create_in(&store);
    assert_non_null(mgr);
    assert_int_equal(ebpf_guard_init(NULL), -1);
    assert_int_equal(ebpf_guard_init(mgr), 0);
    assert_true(ebpf_guard_is_enforce_mode(mgr));
}

static void test_ebpf_guard_workflow(void **state) {
    (void)state;
    ebpf_guard_mgr_storage_t store;
    ebpf_guard_mgr_t *mgr = ebpf_guard_mgr_create_in(&store);
    assert_non_null(mgr);
    assert_int_equal(ebpf_guard_init(mgr), 0);

    uint32_t probe_id = 0;
    assert_int_equal(ebpf_guard_attach_probe(mgr, EBPF_PROBE_SHM_WRITE_AUDIT, &probe_id), 0);
    assert_int_equal(probe_id, 1);

    /* Valid event audit -> return 0 */
    assert_int_equal(ebpf_guard_audit_event(mgr, probe_id, 1234, false), 0);

    /* Unauthorized SHM mutation attempt -> return 1 (Intercepted) */
    assert_int_equal(ebpf_guard_audit_event(mgr, probe_id, 9999, true), 1);

    assert_int_equal(ebpf_guard_get_probe_events_processed(mgr, probe_id), 2);
    assert_int_equal(ebpf_guard_get_probe_violations(mgr, probe_id), 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ebpf_guard_init_null),
        cmocka_unit_test(test_ebpf_guard_workflow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
