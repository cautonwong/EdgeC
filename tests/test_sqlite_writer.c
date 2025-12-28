#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <uv.h>

#include "sqlite_writer.h"

static void test_sqlite_wal_write(void **state) {
    (void)state;
    sqlite_writer_t *w;

    unlink("test_wal.db");

    assert_int_equal(sqlite_writer_init(&w, "test_wal.db"), 0);

    // Sync exec
    assert_int_equal(
        sqlite_writer_exec(w,
                           "CREATE TABLE IF NOT EXISTS t_data (id INTEGER PRIMARY KEY, val REAL);"),
        SQLITE_OK);

    // Async batched writes
    for (int i = 0; i < 50; i++) {
        char sql[128];
        snprintf(sql, sizeof(sql), "INSERT INTO t_data (id, val) VALUES (%d, %f);", i,
                 (double)i * 1.1);
        sqlite_writer_submit(w, sql);
    }

    // Run loop to process work
    // We need to run enough ticks to let thread pool finish
    uv_loop_t *loop = uv_default_loop();
    for (int i = 0; i < 200; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        usleep(5000);  // 5ms
    }

    // Verify results synchronously
    // Check count
    int rc;
    sqlite3 *check_db;
    rc = sqlite3_open_v2("test_wal.db", &check_db, SQLITE_OPEN_READONLY, NULL);
    assert_int_equal(rc, SQLITE_OK);

    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(check_db, "SELECT count(*) FROM t_data;", -1, &stmt, NULL);
    assert_int_equal(rc, SQLITE_OK);

    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    int count = sqlite3_column_int(stmt, 0);
    assert_int_equal(count, 50);

    sqlite3_finalize(stmt);
    sqlite3_close(check_db);

    sqlite_writer_destroy(w);
    unlink("test_wal.db");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sqlite_wal_write),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
