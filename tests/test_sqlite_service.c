#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <sqlite3.h> // For SQLITE_OK

#include "sqlite_service.h"

// Callback context for the async SQL execution
typedef struct {
    int rc;
    int done;
} SqlTestCallbackData;

// Regular C callback function (replaces the C++ lambda)
static void sql_test_cb(int rc, void *user) {
    SqlTestCallbackData *data = (SqlTestCallbackData *)user;
    data->rc = rc;
    data->done = 1;
}

// Global loop variable for uv_run in cmocka tests. Not ideal for concurrent tests.
// For simplicity, following the pattern in impl_0.md snippets.
uv_loop_t *current_test_loop = NULL;

static void test_sql_insert(void **state) {
    (void) state; /* unused */

    uv_loop_t loop;
    uv_loop_init(&loop);
    current_test_loop = &loop; // Set global loop for sqlite_service_init

    // Initialize the SQLite service with a temporary in-memory database for testing
    // Using ":memory:" creates a private, temporary in-memory database.
    assert_int_equal(sqlite_service_init(&loop, ":memory:"), 0);

    SqlTestCallbackData cb_data = { .rc = -1, .done = 0 };

    // Create table asynchronously
    assert_int_equal(sqlite_exec_async(
        "CREATE TABLE IF NOT EXISTS t(a INT);",
        sql_test_cb,
        &cb_data
    ), 0);

    // Run the loop until the callback is called
    while (!cb_data.done) {
        uv_run(&loop, UV_RUN_ONCE);
    }
    assert_int_equal(cb_data.rc, SQLITE_OK); // SQLITE_OK = 0

    // Reset for next async call
    cb_data.rc = -1;
    cb_data.done = 0;

    // Insert data asynchronously
    assert_int_equal(sqlite_exec_async(
        "INSERT INTO t VALUES(100);",
        sql_test_cb,
        &cb_data
    ), 0);
    
    // Run the loop until the callback is called
    while (!cb_data.done) {
        uv_run(&loop, UV_RUN_ONCE);
    }
    assert_int_equal(cb_data.rc, SQLITE_OK);

    // Clean up SQLite service
    sqlite_service_close();

    // Ensure all libuv work is finished and handles are closed
    // uv_loop_alive() can be used to check for any active handles/requests
    while (uv_loop_alive(&loop)) {
        uv_run(&loop, UV_RUN_ONCE);
    }
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sql_insert),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
