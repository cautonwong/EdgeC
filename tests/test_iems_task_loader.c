#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <uv.h>

#include "iems_task_loader.h"
#include "sqlite_service.h"
#include "stream_engine.h"
#include "edge_guard.h"

extern pipeline_stage_t *stage_vpp_logger_create(sqlite_svc_t *db);
extern void dpi_audit_signed_log(dpi_result_t res, uint32_t tag_id, double val, const char *reason);

static int g_callback_task_count = 0;
static uint32_t g_last_tag_id = 0;

static void _test_task_cb(const iems_loaded_task_t *task, void *user_data) {
    (void)user_data;
    if (task) {
        g_callback_task_count++;
        g_last_tag_id = task->tag_id;
    }
}

static void test_iems_task_loading_direct_product_code(void **state) {
    (void)state;
    const char *db_path = "test_iems_task_loader_direct.db";
    unlink(db_path);

    uv_loop_t *loop = uv_default_loop();
    sqlite_config_t cfg = {
        .strategy = STRATEGY_LITE,
        .db_path = db_path
    };
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);

    // 1. Initial state: tables do not exist yet or not ready -> return 0
    g_callback_task_count = 0;
    int rc = iems_poll_tasks_load(svc, _test_task_cb, NULL);
    assert_int_equal(rc, 0);
    assert_int_equal(g_callback_task_count, 0);

    // 2. Setup schema & seed data via sqlite_svc_exec
    sqlite3 *raw_db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_non_null(raw_db);

    const char *setup_sql =
        "CREATE TABLE IF NOT EXISTS matrix_meta (key TEXT PRIMARY KEY, val TEXT);"
        "CREATE TABLE IF NOT EXISTS inventory (id INTEGER PRIMARY KEY, addr TEXT, protocol TEXT);"
        "CREATE TABLE IF NOT EXISTS poll_tasks (bus_id INTEGER, device_id INTEGER, tag_id INTEGER);"
        "INSERT INTO inventory (id, addr, protocol) VALUES (1, '01', 'MODBUS');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (2, '02', 'MODBUS');"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 1, 1001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 2, 1002);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (10, 1, 9999);"; // Bus 10 should be filtered
    rc = sqlite3_exec(raw_db, setup_sql, NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);

    // 3. Before is_ready=1 -> must return 0
    rc = iems_poll_tasks_load(svc, _test_task_cb, NULL);
    assert_int_equal(rc, 0);
    assert_int_equal(g_callback_task_count, 0);

    // 4. Set is_ready=1
    rc = sqlite3_exec(raw_db, "INSERT INTO matrix_meta (key, val) VALUES ('is_ready', '1');", NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);

    // 5. Load tasks -> should load 2 tasks (bus 0 tag 1001, 1002; bus 10 ignored)
    g_callback_task_count = 0;
    rc = iems_poll_tasks_load(svc, _test_task_cb, NULL);
    assert_int_equal(rc, 2);
    assert_int_equal(g_callback_task_count, 2);
    assert_int_equal(g_last_tag_id, 1002);

    sqlite_svc_destroy(svc);
    unlink(db_path);
}

static void test_stage_vpp_logger_edge_env_guard(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    const char *db_path = "test_vpp_logger_guard.db";
    unlink(db_path);

    sqlite_config_t cfg = {
        .strategy = STRATEGY_LITE,
        .db_path = db_path
    };
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);

    // Case 1: EDGE_ENV=production without VPP_AUDIT_SECRET -> Fail-fast (returns NULL)
    setenv("EDGE_ENV", "production", 1);
    unsetenv("VPP_AUDIT_SECRET");
    pipeline_stage_t *stage_prod_missing = stage_vpp_logger_create(svc);
    assert_null(stage_prod_missing);

    // Case 2: EDGE_ENV=development without VPP_AUDIT_SECRET -> Dev fallback (returns valid stage)
    setenv("EDGE_ENV", "development", 1);
    unsetenv("VPP_AUDIT_SECRET");
    pipeline_stage_t *stage_dev = stage_vpp_logger_create(svc);
    assert_non_null(stage_dev);
    assert_string_equal(stage_dev->name, "VPP_LOGGER");
    if (stage_dev->cleanup_state) {
        stage_dev->cleanup_state(stage_dev->priv_state);
    }
    free(stage_dev);

    // Case 3: EDGE_ENV=production with valid VPP_AUDIT_SECRET -> Success
    setenv("EDGE_ENV", "production", 1);
    setenv("VPP_AUDIT_SECRET", "custom_prod_vpp_secret_2026", 1);
    pipeline_stage_t *stage_prod_valid = stage_vpp_logger_create(svc);
    assert_non_null(stage_prod_valid);
    assert_string_equal(stage_prod_valid->name, "VPP_LOGGER");
    if (stage_prod_valid->cleanup_state) {
        stage_prod_valid->cleanup_state(stage_prod_valid->priv_state);
    }
    free(stage_prod_valid);

    // Clean up
    unsetenv("EDGE_ENV");
    unsetenv("VPP_AUDIT_SECRET");
    sqlite_svc_destroy(svc);
    unlink(db_path);
}

static void test_secure_syslog_exporter_dpi_audit_log(void **state) {
    (void)state;
    // Case 1: Development environment with fallback secret
    setenv("EDGE_ENV", "development", 1);
    unsetenv("EDGE_GUARD_AUDIT_SECRET");
    // Should execute safely using dev fallback
    dpi_audit_signed_log(DPI_PASS, 1001, 220.0, "normal_operation");

    // Case 2: Custom secret provided
    setenv("EDGE_GUARD_AUDIT_SECRET", "test_audit_secret_key_abcdef123456", 1);
    dpi_audit_signed_log(DPI_BLOCK_RANGE, 1002, 380.0, "voltage_surge_block");

    // Clean up
    unsetenv("EDGE_ENV");
    unsetenv("EDGE_GUARD_AUDIT_SECRET");
}

static void test_secure_syslog_exporter_production_exit_fork(void **state) {
    (void)state;
    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        // Child process: set EDGE_ENV=production without secret -> must exit(EXIT_FAILURE)
        setenv("EDGE_ENV", "production", 1);
        unsetenv("EDGE_GUARD_AUDIT_SECRET");
        dpi_audit_signed_log(DPI_PASS, 1001, 220.0, "should_fail_in_production");
        // If it didn't exit, fail child with code 0
        exit(0);
    }

    int status = 0;
    pid_t waited = waitpid(pid, &status, 0);
    assert_int_equal(waited, pid);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), EXIT_FAILURE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_iems_task_loading_direct_product_code),
        cmocka_unit_test(test_stage_vpp_logger_edge_env_guard),
        cmocka_unit_test(test_secure_syslog_exporter_dpi_audit_log),
        cmocka_unit_test(test_secure_syslog_exporter_production_exit_fork),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
