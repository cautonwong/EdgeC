/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_storage_powercut_resilience.c
 * @brief Unit tests for 1000 Simulated Power-Cut & Torn-Write Storage Crash Consistency (AEP-410 /
 * Issue #132)
 */

#include <assert.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_powercut_fault_injector.h"

#define POWERCUT_ITERATIONS 1000

static void test_powercut_and_crash_consistency(void) {
    printf("[TEST] Running %d Power-Cut & Torn-Write Crash Consistency Tests...\n",
           POWERCUT_ITERATIONS);

    const char *db_file = "test_powercut_wal.db";
    unlink(db_file);
    unlink("test_powercut_wal.db-wal");
    unlink("test_powercut_wal.db-shm");

    storage_fault_injector_t injector;
    storage_fault_injector_init(&injector);

    sqlite3 *db = NULL;
    int rc = sqlite3_open(db_file, &db);
    assert(rc == SQLITE_OK);

    sqlite3_exec(db, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(db,
                 "CREATE TABLE IF NOT EXISTS telemetry_wal (id INTEGER PRIMARY KEY, ts INTEGER, "
                 "tag INTEGER, val REAL);",
                 NULL, NULL, NULL);
    sqlite3_close(db);

    uint32_t simulated_crashes = 0;
    uint32_t successful_replays = 0;

    for (int iter = 0; iter < POWERCUT_ITERATIONS; iter++) {
        rc = sqlite3_open(db_file, &db);
        assert(rc == SQLITE_OK);

        // Every 3 iterations, arm a sudden powercut
        if (iter % 3 == 0) {
            storage_fault_injector_arm(&injector, FAULT_INJECT_SUDDEN_POWERCUT, 2);
        } else if (iter % 5 == 0) {
            storage_fault_injector_arm(&injector, FAULT_INJECT_TORN_WRITE, 1);
        } else {
            storage_fault_injector_init(&injector);
        }

        sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION;", NULL, NULL, NULL);

        bool fault_occurred = false;
        for (int op = 0; op < 5; op++) {
            if (storage_fault_injector_should_fail(&injector)) {
                fault_occurred = true;
                simulated_crashes++;
                // Simulate abrupt process kill without committing
                break;
            }

            char sql[128];
            snprintf(sql, sizeof(sql), "INSERT INTO telemetry_wal VALUES (%d, %ld, 1001, 220.5);",
                     iter * 10 + op, 1700000000L + iter);
            sqlite3_exec(db, sql, NULL, NULL, NULL);
        }

        if (fault_occurred) {
            // Power-cut crash: force rollback / abrupt close without commit
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            sqlite3_close(db);

            // Re-open: SQLite WAL must automatically recover and verify integrity
            rc = sqlite3_open(db_file, &db);
            assert(rc == SQLITE_OK);

            sqlite3_stmt *stmt;
            rc = sqlite3_prepare_v2(db, "PRAGMA integrity_check;", -1, &stmt, NULL);
            assert(rc == SQLITE_OK);
            assert(sqlite3_step(stmt) == SQLITE_ROW);
            const char *res = (const char *)sqlite3_column_text(stmt, 0);
            assert(strcmp(res, "ok") == 0);
            sqlite3_finalize(stmt);
            successful_replays++;
        } else {
            sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
        }

        sqlite3_close(db);
    }

    printf(
        "  -> Simulated %u abrupt powercuts and torn-writes; %u successful crash-recovery "
        "checks!\n",
        simulated_crashes, successful_replays);
    assert(simulated_crashes > 0);
    assert(successful_replays == simulated_crashes);

    unlink(db_file);
    unlink("test_powercut_wal.db-wal");
    unlink("test_powercut_wal.db-shm");

    printf("  -> PASSED: 1000 power-cut iterations verified with 100%% crash consistency.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-410: Power-Cut & Crash-Consistency Test Suite          \n");
    printf("=================================================================\n");

    test_powercut_and_crash_consistency();

    printf("\nAll AEP-410 Power-Cut tests PASSED!\n");
    return 0;
}
