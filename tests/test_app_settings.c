/**
 * @file test_app_settings.c
 * @brief Settings schema validation, defaults and per-app value storage.
 */

#include <cmocka.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "app_settings_sqlite.h"
#include "edgeos/app_settings.h"
#include "sqlite_service.h"

typedef struct {
    uv_loop_t loop;
} fixture_t;

static fixture_t g_fixture;

static const char *k_schema =
    "{\"schemaVersion\":1,\"fields\":{"
    "\"threshold\":{\"type\":\"number\",\"default\":240,\"minimum\":180,\"maximum\":300},"
    "\"mode\":{\"type\":\"string\",\"enum\":[\"warn\",\"trip\"],\"default\":\"warn\"},"
    "\"count\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":10},"
    "\"enabled\":{\"type\":\"boolean\",\"default\":true},"
    "\"hosts\":{\"type\":\"array<string>\",\"default\":[\"a\"],\"maxLength\":3},"
    "\"ro\":{\"type\":\"string\",\"readonly\":true,\"default\":\"x\"},"
    "\"token\":{\"type\":\"string\"},"
    "\"apiKey\":{\"type\":\"string\",\"secret\":true}"
    "}}";

static sqlite_svc_t *make_db(const char *path) {
    unlink(path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    return sqlite_svc_create(&g_fixture.loop, &cfg);
}

static void test_schema_validation(void **state) {
    (void)state;
    char reason[160];
    assert_int_equal(edge_app_settings_validate_json(k_schema, reason, sizeof(reason)), 0);
    assert_int_equal(
        edge_app_settings_validate_json("{\"level\":{\"type\":\"integer\",\"default\":1}}", reason,
                                        sizeof(reason)),
        0);

    static const char *invalid[] = {
        "{\"fields\":{\"a\":{\"type\":\"string\"}},\"extra\":1}",
        "{\"fields\":{\"a\":{\"type\":\"string\",\"$ref\":\"#/x\"}}}",
        "{\"fields\":{\"a\":{\"type\":\"string\",\"pattern\":\"x\"}}}",
        "{\"fields\":{\"a\":{\"type\":\"string\",\"oneOf\":[]}}}",
        "{\"fields\":{\"a\":{\"type\":\"object\"}}}",
        "{\"fields\":{\"a\":{\"type\":\"integer\",\"default\":\"1\"}}}",
        "{\"fields\":{\"a\":{\"type\":\"integer\",\"default\":11,\"maximum\":10}}}",
        "{\"fields\":{\"a\":{\"type\":\"string\",\"enum\":[\"x\"],\"default\":\"y\"}}}",
        "{\"fields\":{\"a\":{\"type\":\"string\",\"minimum\":1}}}",
        "{\"fields\":{\"a\":{\"type\":\"number\",\"minLength\":1}}}",
        "{\"fields\":{\"a\":{\"type\":\"string\",\"minLength\":5,\"maxLength\":2}}}",
        "{\"schemaVersion\":2,\"fields\":{\"a\":{\"type\":\"string\"}}}",
        "{\"schemaVersion\":1,\"fields\":[]}",
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        int rc = edge_app_settings_validate_json(invalid[i], reason, sizeof(reason));
        if (rc != -1) print_message("case %zu accepted: %s (reason %s)\n", i, invalid[i], reason);
        assert_int_equal(rc, -1);
        assert_true(reason[0] != '\0');
    }
}

static void test_normalize_bare_schema(void **state) {
    (void)state;
    char reason[160];
    char *normalized = edge_app_settings_normalize_json(
        "{\"level\":{\"type\":\"integer\",\"default\":1}}", reason, sizeof(reason));
    assert_non_null(normalized);
    assert_non_null(strstr(normalized, "\"schemaVersion\":1"));
    assert_non_null(strstr(normalized, "\"fields\""));
    assert_non_null(strstr(normalized, "\"level\""));

    char value[32];
    assert_int_equal(edge_app_setting_default_json(normalized, "level", value, sizeof(value),
                                                   reason, sizeof(reason)),
                     1);
    assert_string_equal(value, "1");
    free(normalized);
}

static void test_defaults_and_stored_values(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db("/tmp/edgeos_settings_a.db");
    assert_non_null(db);
    assert_int_equal(app_settings_schema_save(db, "demo", k_schema), 0);

    char value[128];
    char reason[APP_SETTING_REASON_SIZE];
    assert_int_equal(app_settings_value_get(db, "demo", "threshold", value, sizeof(value), reason,
                                            sizeof(reason)),
                     1);
    assert_string_equal(value, "240");
    assert_int_equal(
        app_settings_value_get(db, "demo", "mode", value, sizeof(value), reason, sizeof(reason)),
        1);
    assert_string_equal(value, "\"warn\"");
    assert_int_equal(
        app_settings_value_get(db, "demo", "enabled", value, sizeof(value), reason, sizeof(reason)),
        1);
    assert_string_equal(value, "true");
    assert_int_equal(
        app_settings_value_get(db, "demo", "hosts", value, sizeof(value), reason, sizeof(reason)),
        1);
    assert_string_equal(value, "[\"a\"]");

    assert_int_equal(app_settings_value_set(db, "demo", "threshold", "250", reason, sizeof(reason)),
                     0);
    assert_int_equal(app_settings_value_get(db, "demo", "threshold", value, sizeof(value), reason,
                                            sizeof(reason)),
                     1);
    assert_string_equal(value, "250");

    assert_int_equal(app_settings_value_set(db, "demo", "mode", "\"trip\"", reason, sizeof(reason)),
                     0);
    assert_int_equal(
        app_settings_value_get(db, "demo", "mode", value, sizeof(value), reason, sizeof(reason)),
        1);
    assert_string_equal(value, "\"trip\"");

    assert_int_equal(app_settings_value_set(db, "demo", "count", "7", reason, sizeof(reason)), 0);
    assert_int_equal(app_settings_value_set(db, "demo", "enabled", "false", reason, sizeof(reason)),
                     0);
    assert_int_equal(
        app_settings_value_set(db, "demo", "hosts", "[\"a\",\"b\"]", reason, sizeof(reason)), 0);

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_settings_a.db");
}

static void test_not_set_unknown_and_no_schema(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db("/tmp/edgeos_settings_b.db");
    assert_non_null(db);
    assert_int_equal(app_settings_schema_save(db, "demo", k_schema), 0);

    char value[128];
    char reason[APP_SETTING_REASON_SIZE];
    assert_int_equal(
        app_settings_value_get(db, "demo", "token", value, sizeof(value), reason, sizeof(reason)),
        0);
    assert_string_equal(reason, "not_set");
    assert_int_equal(
        app_settings_value_get(db, "demo", "missing", value, sizeof(value), reason, sizeof(reason)),
        -1);
    assert_string_equal(reason, "not_found");
    assert_int_equal(
        app_settings_value_get(db, "other", "token", value, sizeof(value), reason, sizeof(reason)),
        -1);
    assert_string_equal(reason, "not_found");
    assert_int_equal(app_settings_value_set(db, "other", "token", "\"x\"", reason, sizeof(reason)),
                     -1);
    assert_string_equal(reason, "no_schema");

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_settings_b.db");
}

static void test_write_rejections(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db("/tmp/edgeos_settings_c.db");
    assert_non_null(db);
    assert_int_equal(app_settings_schema_save(db, "demo", k_schema), 0);

    struct {
        const char *key;
        const char *value;
        const char *reason;
    } cases[] = {
        {"ro", "\"y\"", "readonly"},
        {"missing", "1", "unknown_key"},
        {"threshold", "\"250\"", "type_mismatch"},
        {"threshold", "301", "out_of_range"},
        {"threshold", "179", "out_of_range"},
        {"count", "2.5", "type_mismatch"},
        {"count", "11", "out_of_range"},
        {"mode", "\"boom\"", "invalid_value"},
        {"hosts", "[\"a\",\"b\",\"c\",\"d\"]", "out_of_range"},
        {"hosts", "[1]", "type_mismatch"},
        {"enabled", "1", "type_mismatch"},
    };
    char reason[APP_SETTING_REASON_SIZE];
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        assert_int_equal(app_settings_value_set(db, "demo", cases[i].key, cases[i].value, reason,
                                                sizeof(reason)),
                         -1);
        assert_non_null(strstr(reason, cases[i].reason));
    }

    assert_int_equal(
        app_settings_value_set(db, "demo", "apiKey", "\"s3cret\"", reason, sizeof(reason)), 0);
    char value[64];
    assert_int_equal(
        app_settings_value_get(db, "demo", "apiKey", value, sizeof(value), reason, sizeof(reason)),
        1);
    assert_string_equal(value, "\"s3cret\"");

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_settings_c.db");
}

static void test_remove_clears_schema_and_values(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db("/tmp/edgeos_settings_d.db");
    assert_non_null(db);
    char reason[APP_SETTING_REASON_SIZE];
    assert_int_equal(app_settings_schema_save(db, "demo", k_schema), 0);
    assert_int_equal(app_settings_value_set(db, "demo", "token", "\"x\"", reason, sizeof(reason)),
                     0);
    /* A key dropped by a later schema is unknown even though its row remains. */
    assert_int_equal(
        app_settings_schema_save(
            db, "demo", "{\"schemaVersion\":1,\"fields\":{\"threshold\":{\"type\":\"number\"}}}"),
        0);
    char value[64];
    assert_int_equal(
        app_settings_value_get(db, "demo", "token", value, sizeof(value), reason, sizeof(reason)),
        -1);
    assert_string_equal(reason, "not_found");
    assert_int_equal(app_settings_remove(db, "demo"), 0);
    assert_int_equal(
        app_settings_value_get(db, "demo", "token", value, sizeof(value), reason, sizeof(reason)),
        -1);
    assert_string_equal(reason, "not_found");

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_settings_d.db");
}

static void test_upgrade_compatibility_guard(void **state) {
    (void)state;
    char reason[160];
    const char *old_schema =
        "{\"schemaVersion\":1,\"fields\":{"
        "\"threshold\":{\"type\":\"number\",\"default\":240},"
        "\"mode\":{\"type\":\"string\",\"default\":\"warn\"}}}";

    assert_int_equal(
        edge_app_settings_check_upgrade(old_schema, old_schema, reason, sizeof(reason)), 0);
    assert_int_equal(edge_app_settings_check_upgrade(NULL, old_schema, reason, sizeof(reason)), 0);

    const char *removed =
        "{\"schemaVersion\":1,\"fields\":{\"threshold\":{\"type\":\"number\",\"default\":240}}}";
    assert_int_equal(edge_app_settings_check_upgrade(old_schema, removed, reason, sizeof(reason)),
                     -1);
    assert_non_null(strstr(reason, "mode"));
    assert_non_null(strstr(reason, "removed"));

    const char *retyped =
        "{\"schemaVersion\":1,\"fields\":{"
        "\"threshold\":{\"type\":\"string\",\"default\":\"240\"},"
        "\"mode\":{\"type\":\"string\",\"default\":\"warn\"}}}";
    assert_int_equal(edge_app_settings_check_upgrade(old_schema, retyped, reason, sizeof(reason)),
                     -1);
    assert_non_null(strstr(reason, "threshold"));
    assert_non_null(strstr(reason, "type changed"));

    const char *new_required =
        "{\"schemaVersion\":1,\"fields\":{"
        "\"threshold\":{\"type\":\"number\",\"default\":240},"
        "\"mode\":{\"type\":\"string\",\"default\":\"warn\"},"
        "\"limit\":{\"type\":\"integer\"}}}";
    assert_int_equal(
        edge_app_settings_check_upgrade(old_schema, new_required, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "limit"));
    assert_non_null(strstr(reason, "no default"));

    const char *new_defaulted =
        "{\"schemaVersion\":1,\"fields\":{"
        "\"threshold\":{\"type\":\"number\",\"default\":240},"
        "\"mode\":{\"type\":\"string\",\"default\":\"warn\"},"
        "\"limit\":{\"type\":\"integer\",\"default\":0}}}";
    assert_int_equal(
        edge_app_settings_check_upgrade(old_schema, new_defaulted, reason, sizeof(reason)), 0);
}

static void test_upgrade_check_keeps_stored_schema(void **state) {
    (void)state;
    sqlite_svc_t *db = make_db("/tmp/edgeos_settings_e.db");
    assert_non_null(db);
    char reason[APP_SETTING_REASON_SIZE];
    const char *old_schema =
        "{\"schemaVersion\":1,\"fields\":{\"threshold\":{\"type\":\"number\",\"default\":1}}}";
    assert_int_equal(app_settings_schema_save(db, "demo", old_schema), 0);
    assert_int_equal(app_settings_value_set(db, "demo", "threshold", "5", reason, sizeof(reason)),
                     0);

    const char *incompatible =
        "{\"schemaVersion\":1,\"fields\":{\"mode\":{\"type\":\"string\",\"default\":\"warn\"}}}";
    assert_int_equal(app_settings_upgrade_check(db, "demo", incompatible, reason, sizeof(reason)),
                     -1);
    assert_non_null(strstr(reason, "threshold"));
    assert_non_null(strstr(reason, "removed"));

    char value[64];
    assert_int_equal(app_settings_value_get(db, "demo", "threshold", value, sizeof(value), reason,
                                            sizeof(reason)),
                     1);
    assert_string_equal(value, "5");

    const char *compatible =
        "{\"schemaVersion\":1,\"fields\":{"
        "\"threshold\":{\"type\":\"number\",\"default\":2},"
        "\"interval\":{\"type\":\"integer\",\"default\":60}}}";
    assert_int_equal(app_settings_upgrade_check(db, "demo", compatible, reason, sizeof(reason)), 0);
    assert_int_equal(app_settings_upgrade_check(db, "demo", NULL, reason, sizeof(reason)), -1);
    assert_non_null(strstr(reason, "threshold"));
    assert_int_equal(app_settings_upgrade_check(db, "other", NULL, reason, sizeof(reason)), 0);

    sqlite_svc_destroy(db);
    unlink("/tmp/edgeos_settings_e.db");
}

static int setup(void **state) {
    (void)state;
    return 0;
}

static int teardown(void **state) {
    (void)state;
    return 0;
}

int main(void) {
    memset(&g_fixture, 0, sizeof(g_fixture));
    assert_int_equal(uv_loop_init(&g_fixture.loop), 0);
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_schema_validation, setup, teardown),
        cmocka_unit_test_setup_teardown(test_normalize_bare_schema, setup, teardown),
        cmocka_unit_test_setup_teardown(test_defaults_and_stored_values, setup, teardown),
        cmocka_unit_test_setup_teardown(test_not_set_unknown_and_no_schema, setup, teardown),
        cmocka_unit_test_setup_teardown(test_write_rejections, setup, teardown),
        cmocka_unit_test_setup_teardown(test_remove_clears_schema_and_values, setup, teardown),
        cmocka_unit_test_setup_teardown(test_upgrade_compatibility_guard, setup, teardown),
        cmocka_unit_test_setup_teardown(test_upgrade_check_keeps_stored_schema, setup, teardown),
    };
    int rc = cmocka_run_group_tests(tests, NULL, NULL);
    uv_run(&g_fixture.loop, UV_RUN_DEFAULT);
    uv_loop_close(&g_fixture.loop);
    return rc;
}
