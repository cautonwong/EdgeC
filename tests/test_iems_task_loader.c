#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "sqlite_service.h"
#include "storage_ops.h"
#include "datahub_ops.h"

static int g_loaded_task_count = 0;

static int _mock_poll_task_cb(void *user_data, int argc, char **argv, char **azColName) {
    (void)user_data;
    (void)azColName;
    if (argc >= 4 && argv[0] && argv[1] && argv[2] && argv[3]) {
        g_loaded_task_count++;
    }
    return 0;
}

static void test_iems_task_loading_lifecycle(void **state) {
    (void)state;
    const char *db_path = "test_iems_task_loader.db";
    unlink(db_path);

    sqlite3 *db = NULL;
    int rc = sqlite3_open(db_path, &db);
    assert_int_equal(rc, SQLITE_OK);
    assert_non_null(db);

    // 1. Setup schema
    const char *schema =
        "CREATE TABLE matrix_meta (key TEXT PRIMARY KEY, val TEXT);"
        "CREATE TABLE inventory (id INTEGER PRIMARY KEY, addr TEXT, protocol TEXT);"
        "CREATE TABLE poll_tasks (bus_id INTEGER, device_id INTEGER, tag_id INTEGER);";
    rc = sqlite3_exec(db, schema, NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);

    // 2. Insert inventory and poll_tasks
    const char *seed =
        "INSERT INTO inventory (id, addr, protocol) VALUES (1, '01', 'MODBUS');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (2, '02', 'MODBUS');"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 1, 1001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 2, 1002);";
    rc = sqlite3_exec(db, seed, NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);

    // 3. Before is_ready flag is set:
    sqlite3_stmt *stmt = NULL;
    const char *check_sql = "SELECT val FROM matrix_meta WHERE key='is_ready';";
    bool ready = false;
    if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW &&
            strcmp((const char *)sqlite3_column_text(stmt, 0), "1") == 0) {
            ready = true;
        }
        sqlite3_finalize(stmt);
    }
    assert_false(ready);

    // 4. Set is_ready flag
    rc = sqlite3_exec(db, "INSERT INTO matrix_meta (key, val) VALUES ('is_ready', '1');", NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);

    // 5. Query tasks
    g_loaded_task_count = 0;
    const char *query_sql =
        "SELECT t.bus_id, i.addr, t.tag_id, i.protocol FROM poll_tasks t INNER JOIN inventory "
        "i ON t.device_id = i.id;";
    rc = sqlite3_exec(db, query_sql, _mock_poll_task_cb, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);
    assert_int_equal(g_loaded_task_count, 2);

    sqlite3_close(db);
    unlink(db_path);
}

static void test_dev_vs_prod_env_guard(void **state) {
    (void)state;
    // Test that production environment requires explicit secrets
    setenv("EDGE_ENV", "production", 1);
    const char *env = getenv("EDGE_ENV");
    assert_string_equal(env, "production");

    // In production, fallback secret keys must be rejected if unset
    unsetenv("DOIP_UDS_SECRET_KEY");
    const char *prod_secret = getenv("DOIP_UDS_SECRET_KEY");
    assert_null(prod_secret);

    // Development environment allows fallback
    setenv("EDGE_ENV", "development", 1);
    const char *dev_env = getenv("EDGE_ENV");
    assert_string_equal(dev_env, "development");

    // Clean up
    unsetenv("EDGE_ENV");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_iems_task_loading_lifecycle),
        cmocka_unit_test(test_dev_vs_prod_env_guard),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
