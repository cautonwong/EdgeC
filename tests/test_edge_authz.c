#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edgeos/edge_authz.h"

typedef struct {
    edge_authz_memory_store_t store;
    edge_authz_repo_t repo;
    edge_authz_engine_t engine;
} fixture_t;

static int setup(void **state) {
    fixture_t *fx = calloc(1, sizeof(*fx));
    if (!fx) return -1;
    edge_authz_memory_init(&fx->store);
    fx->repo = edge_authz_memory_repo(&fx->store);
    edge_authz_engine_init(&fx->engine, &fx->repo);
    edge_authz_engine_set_enforcement(&fx->engine, true);
    *state = fx;
    return 0;
}

static int teardown(void **state) {
    free(*state);
    return 0;
}

static edge_app_grant_t base_grant(const char *scope_json) {
    edge_app_grant_t g;
    memset(&g, 0, sizeof(g));
    snprintf(g.app_id, sizeof(g.app_id), "app.demo");
    snprintf(g.capability, sizeof(g.capability), "tags.read");
    snprintf(g.scope_json, sizeof(g.scope_json), "%s", scope_json);
    snprintf(g.purpose, sizeof(g.purpose), "display voltage");
    g.state = EDGE_GRANT_GRANTED;
    g.sensitivity_max = 1;
    g.granted_at = 100;
    return g;
}

static edge_authz_result_t check(fixture_t *fx, const char *scope, uint64_t now) {
    return edge_authz_check(&fx->engine, "app.demo", "tags.read", scope, now);
}

static void test_not_granted(void **state) {
    fixture_t *fx = *state;
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_NOT_GRANTED);
}

static void test_pending_grant_is_not_granted(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.state = EDGE_GRANT_PENDING;
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_NOT_GRANTED);
}

static void test_expired_grant(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.expires_at = 1000;
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(check(fx, "U_A", 999), EDGE_AUTHZ_OK);
    assert_int_equal(check(fx, "U_A", 1000), EDGE_AUTHZ_DENIED_EXPIRED);
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_DENIED_EXPIRED), "denied_expired");
}

static void test_permanent_grant_never_expires(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(check(fx, "U_A", UINT64_MAX), EDGE_AUTHZ_OK);
}

static void test_revoked_beats_expiry(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.revoked_at = 200;
    g.expires_at = 250;
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(check(fx, "U_A", 300), EDGE_AUTHZ_DENIED_REVOKED);
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_DENIED_REVOKED), "denied_revoked");
}

static void test_scope_out_of_bounds(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_OK);
    assert_int_equal(check(fx, "U_B", 100), EDGE_AUTHZ_DENIED_SCOPE);
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_DENIED_SCOPE), "denied_scope");
}

static void test_wildcard_is_literal_only(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_*\"]");
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_SCOPE);
    assert_int_equal(check(fx, "U_*", 100), EDGE_AUTHZ_OK);
}

static void test_sensitivity_above_grant_limit(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.sensitivity_max = 1;
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(edge_authz_memory_add_point(&fx->store, "U_A", 2), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_POLICY);
}

static void test_trust_insufficient_for_sensitive_point(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.sensitivity_max = 3;
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(edge_authz_memory_add_point(&fx->store, "U_A", 2), 0);

    assert_int_equal(edge_authz_memory_set_publisher(&fx->store, "app.demo", 1, true), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_POLICY);

    assert_int_equal(edge_authz_memory_set_publisher(&fx->store, "app.demo", 2, false), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_POLICY);

    assert_int_equal(edge_authz_memory_set_publisher(&fx->store, "app.demo", 2, true), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_OK);
}

static void test_marked_grant_requires_purpose(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.sensitivity_max = 2;
    g.purpose[0] = '\0';

    assert_int_equal(edge_authz_validate_grant(&g), EDGE_AUTHZ_INVALID_MANIFEST);
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_INVALID_MANIFEST), "invalid_manifest");

    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_POLICY);

    snprintf(g.purpose, sizeof(g.purpose), "工单 #123");
    assert_int_equal(edge_authz_validate_grant(&g), EDGE_AUTHZ_OK);
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_OK);
}

static void test_package_mismatch(void **state) {
    (void)state;
    assert_int_equal(edge_authz_check_package("sha256:a", "sha256:a"), EDGE_AUTHZ_OK);
    assert_int_equal(edge_authz_check_package("sha256:a", "sha256:b"),
                     EDGE_AUTHZ_DENIED_PACKAGE_MISMATCH);
    assert_int_equal(edge_authz_check_package(NULL, "sha256:a"),
                     EDGE_AUTHZ_DENIED_PACKAGE_MISMATCH);
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_DENIED_PACKAGE_MISMATCH),
                        "denied_package_mismatch");
}

static void test_result_codes(void **state) {
    (void)state;
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_OK), "ok");
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_DENIED_NOT_GRANTED),
                        "denied_not_granted");
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_DENIED_POLICY), "denied_policy");
    assert_string_equal(edge_authz_result_code(EDGE_AUTHZ_DENIED_QUOTA), "denied_quota");
}

static void test_enforcement_default_off(void **state) {
    fixture_t *fx = *state;
    edge_authz_engine_set_enforcement(&fx->engine, false);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_OK);
}

static void test_inherit_unchanged(void **state) {
    (void)state;
    edge_app_grant_t g = base_grant("[\"U_A\",\"U_B\"]");
    edge_app_grant_t out;
    assert_int_equal(edge_authz_inherit_grant(&g, "[\"U_A\",\"U_B\"]", &out),
                     EDGE_AUTHZ_INHERIT_UNCHANGED);
    assert_int_equal(out.state, EDGE_GRANT_GRANTED);
    assert_string_equal(out.scope_json, "[\"U_A\",\"U_B\"]");
}

static void test_inherit_narrowed(void **state) {
    (void)state;
    edge_app_grant_t g = base_grant("[\"U_A\",\"U_B\"]");
    edge_app_grant_t out;
    assert_int_equal(edge_authz_inherit_grant(&g, "[\"U_A\"]", &out),
                     EDGE_AUTHZ_INHERIT_NARROWED);
    assert_int_equal(out.state, EDGE_GRANT_GRANTED);
    assert_string_equal(out.scope_json, "[\"U_A\"]");
}

static void test_inherit_expanded_waits_for_approval(void **state) {
    (void)state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    edge_app_grant_t out;
    assert_int_equal(edge_authz_inherit_grant(&g, "[\"U_A\",\"U_C\"]", &out),
                     EDGE_AUTHZ_INHERIT_AWAITING);
    assert_int_equal(out.state, EDGE_GRANT_PENDING);
    assert_string_equal(out.scope_json, "[\"U_A\",\"U_C\"]");
}

static void test_inherit_does_not_revive_revoked(void **state) {
    (void)state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.revoked_at = 50;
    edge_app_grant_t out;
    assert_int_equal(edge_authz_inherit_grant(&g, "[\"U_A\"]", &out),
                     EDGE_AUTHZ_INHERIT_REVOKED);
    assert_int_equal(out.revoked_at, 50);
    assert_int_equal(out.state, EDGE_GRANT_GRANTED);
}

static void test_inherit_rejects_bad_scope_json(void **state) {
    (void)state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    edge_app_grant_t out;
    assert_int_equal(edge_authz_inherit_grant(&g, "{not json", &out),
                     EDGE_AUTHZ_INHERIT_INVALID);
}

/* --- Admin store over the in-memory repo --- */

static int memory_store_get(void *ctx, const char *app_id, const char *capability,
                            edge_app_grant_t *out) {
    edge_authz_memory_store_t *store = (edge_authz_memory_store_t *)ctx;
    for (size_t i = 0; i < store->grant_count; i++) {
        if (strcmp(store->grants[i].app_id, app_id) == 0 &&
            strcmp(store->grants[i].capability, capability) == 0) {
            *out = store->grants[i];
            return 1;
        }
    }
    return 0;
}

static int memory_store_save(void *ctx, const edge_app_grant_t *grant) {
    return edge_authz_memory_add_grant((edge_authz_memory_store_t *)ctx, grant);
}

static int memory_store_list(void *ctx, const char *app_id, edge_app_grant_t *out, size_t max,
                             size_t *count) {
    edge_authz_memory_store_t *store = (edge_authz_memory_store_t *)ctx;
    size_t n = 0;
    for (size_t i = 0; i < store->grant_count && n < max; i++) {
        if (app_id && app_id[0] && strcmp(store->grants[i].app_id, app_id) != 0) continue;
        out[n++] = store->grants[i];
    }
    *count = n;
    return 0;
}

static edge_authz_store_t memory_admin_store(edge_authz_memory_store_t *store) {
    edge_authz_store_t out = {
        .ctx = store,
        .get_grant = memory_store_get,
        .save_grant = memory_store_save,
        .list_grants = memory_store_list,
    };
    return out;
}

static void test_approve_requires_purpose_and_stamps(void **state) {
    (void)state;
    edge_authz_memory_store_t mem;
    edge_authz_memory_init(&mem);
    edge_authz_store_t store = memory_admin_store(&mem);

    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.sensitivity_max = 2;
    g.purpose[0] = '\0';
    assert_int_equal(edge_authz_approve_grant(&store, &g, 500), EDGE_AUTHZ_INVALID_MANIFEST);
    assert_int_equal(mem.grant_count, 0);

    snprintf(g.purpose, sizeof(g.purpose), "ticket 42");
    assert_int_equal(edge_authz_approve_grant(&store, &g, 500), EDGE_AUTHZ_OK);
    assert_int_equal(g.state, EDGE_GRANT_GRANTED);
    assert_int_equal(g.granted_at, 500);
    assert_int_equal(g.revoked_at, 0);
}

static void test_revoke_sets_timestamp_and_repeats_fail(void **state) {
    (void)state;
    edge_authz_memory_store_t mem;
    edge_authz_memory_init(&mem);
    edge_authz_store_t store = memory_admin_store(&mem);

    edge_app_grant_t g = base_grant("[\"U_A\"]");
    assert_int_equal(edge_authz_approve_grant(&store, &g, 100), EDGE_AUTHZ_OK);
    assert_int_equal(edge_authz_revoke_grant(&store, "app.demo", "tags.read", 200),
                     EDGE_AUTHZ_OK);

    edge_app_grant_t out;
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.revoked_at, 200);
    assert_int_equal(edge_authz_revoke_grant(&store, "app.demo", "tags.read", 300),
                     EDGE_AUTHZ_OK);
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.revoked_at, 200); /* idempotent: first revocation wins */
}

static void test_inherit_upgrade_narrows_and_waits(void **state) {
    (void)state;
    edge_authz_memory_store_t mem;
    edge_authz_memory_init(&mem);
    edge_authz_store_t store = memory_admin_store(&mem);

    edge_app_grant_t g = base_grant("[\"U_A\",\"U_B\"]");
    assert_int_equal(edge_authz_approve_grant(&store, &g, 100), EDGE_AUTHZ_OK);

    assert_int_equal(
        edge_authz_inherit_upgrade(&store, "app.demo", "tags.read", "[\"U_A\"]", 200),
        EDGE_AUTHZ_INHERIT_NARROWED);
    edge_app_grant_t out;
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_string_equal(out.scope_json, "[\"U_A\"]");
    assert_int_equal(out.state, EDGE_GRANT_GRANTED);

    assert_int_equal(
        edge_authz_inherit_upgrade(&store, "app.demo", "tags.read", "[\"U_A\",\"U_C\"]", 300),
        EDGE_AUTHZ_INHERIT_AWAITING);
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.state, EDGE_GRANT_PENDING);
}

static void test_inherit_upgrade_keeps_revoked_and_creates_pending(void **state) {
    (void)state;
    edge_authz_memory_store_t mem;
    edge_authz_memory_init(&mem);
    edge_authz_store_t store = memory_admin_store(&mem);

    assert_int_equal(
        edge_authz_inherit_upgrade(&store, "app.demo", "tags.read", "[\"U_NEW\"]", 100),
        EDGE_AUTHZ_INHERIT_AWAITING);
    edge_app_grant_t out;
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.state, EDGE_GRANT_PENDING);

    edge_app_grant_t g = base_grant("[\"U_A\"]");
    assert_int_equal(edge_authz_approve_grant(&store, &g, 200), EDGE_AUTHZ_OK);
    assert_int_equal(edge_authz_revoke_grant(&store, "app.demo", "tags.read", 300),
                     EDGE_AUTHZ_OK);

    assert_int_equal(
        edge_authz_inherit_upgrade(&store, "app.demo", "tags.read", "[\"U_A\"]", 400),
        EDGE_AUTHZ_INHERIT_REVOKED);
    assert_int_equal(store.get_grant(store.ctx, "app.demo", "tags.read", &out), 1);
    assert_int_equal(out.revoked_at, 300);
}

static void test_restricted_mode_caps_sensitivity(void **state) {
    fixture_t *fx = *state;
    edge_app_grant_t g = base_grant("[\"U_A\"]");
    g.sensitivity_max = 3;
    assert_int_equal(edge_authz_memory_add_grant(&fx->store, &g), 0);
    assert_int_equal(edge_authz_memory_add_point(&fx->store, "U_A", 1), 0);

    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_POLICY);
    assert_int_equal(edge_authz_memory_set_publisher(&fx->store, "app.demo", 0, true), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_DENIED_POLICY);
    assert_int_equal(edge_authz_memory_set_publisher(&fx->store, "app.demo", 1, true), 0);
    assert_int_equal(check(fx, "U_A", 100), EDGE_AUTHZ_OK);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_not_granted, setup, teardown),
        cmocka_unit_test_setup_teardown(test_pending_grant_is_not_granted, setup, teardown),
        cmocka_unit_test_setup_teardown(test_expired_grant, setup, teardown),
        cmocka_unit_test_setup_teardown(test_permanent_grant_never_expires, setup, teardown),
        cmocka_unit_test_setup_teardown(test_revoked_beats_expiry, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scope_out_of_bounds, setup, teardown),
        cmocka_unit_test_setup_teardown(test_wildcard_is_literal_only, setup, teardown),
        cmocka_unit_test_setup_teardown(test_sensitivity_above_grant_limit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_restricted_mode_caps_sensitivity, setup, teardown),
        cmocka_unit_test_setup_teardown(test_trust_insufficient_for_sensitive_point, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_marked_grant_requires_purpose, setup, teardown),
        cmocka_unit_test_setup_teardown(test_package_mismatch, setup, teardown),
        cmocka_unit_test(test_result_codes),
        cmocka_unit_test_setup_teardown(test_enforcement_default_off, setup, teardown),
        cmocka_unit_test(test_inherit_unchanged),
        cmocka_unit_test(test_inherit_narrowed),
        cmocka_unit_test(test_inherit_expanded_waits_for_approval),
        cmocka_unit_test(test_inherit_does_not_revive_revoked),
        cmocka_unit_test(test_inherit_rejects_bad_scope_json),
        cmocka_unit_test(test_approve_requires_purpose_and_stamps),
        cmocka_unit_test(test_revoke_sets_timestamp_and_repeats_fail),
        cmocka_unit_test(test_inherit_upgrade_narrows_and_waits),
        cmocka_unit_test(test_inherit_upgrade_keeps_revoked_and_creates_pending),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
