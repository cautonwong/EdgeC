#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "edge_log.h"
#include "snf_manager.h"
#include "sqlite_service.h"

static int g_publish_count = 0;

static int mock_publish(void* user_data, const PipeMsg* msg) {
    (void)user_data;
    (void)msg;
    g_publish_count++;
    return 0;
}

static void test_snf_offline_buffering(void** state) {
    (void)state;
    unlink("snf_resilience.db");

    uv_loop_t* loop = uv_default_loop();

    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE,
                              .db_path = "snf_resilience.db",
                              .batch_size = 0,
                              .flush_interval_ms = 0};
    sqlite_svc_t* db = sqlite_svc_create(loop, &db_cfg);
    assert_non_null(db);

    snf_config_t snf_cfg = {.db = db,
                            .loop = loop,
                            .publish_fn = mock_publish,
                            .sync_batch_size = 10,
                            .sync_interval_ms = 100};
    snf_manager_t* mgr = snf_manager_create(&snf_cfg);
    assert_non_null(mgr);

    // 1. Go Offline
    snf_manager_set_online(mgr, false);

    // 2. Buffer data
    LOG_INFO(LOG_CAT_CORE, "[TEST] Buffering 5 records...");
    for (int i = 0; i < 5; i++) {
        PipeMsg msg = {.ts = (uint64_t)i + 2000, .tag_id = 1001, .payload.val.f = (double)i};
        msg.type = PIPEMSG_TYPE_SCALAR;
        int rc = snf_manager_buffer_data(mgr, &msg);
        assert_int_equal(rc, 0);
    }

    // Ensure no publishing while offline
    for (int i = 0; i < 50; i++) uv_run(loop, UV_RUN_NOWAIT);
    assert_int_equal(g_publish_count, 0);

    // [Nexus] Verify DB content before going online
    sqlite3* sqlite = (sqlite3*)sqlite_svc_get_db_handle(db);
    sqlite3_stmt* stmt;
    int db_count = 0;
    if (sqlite3_prepare_v2(sqlite, "SELECT count(*) FROM telemetry WHERE synced = 0;", -1, &stmt,
                           NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            db_count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    printf("[TEST] Unsynced records in DB: %d\n", db_count);
    assert_int_equal(db_count, 5);

    // 3. Go Online
    LOG_INFO(LOG_CAT_CORE, "[TEST] Going ONLINE...");
    snf_manager_set_online(mgr, true);

    // 4. Wait for synchronization
    int found = -1;
    for (int retry = 0; retry < 100; retry++) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (g_publish_count >= 5) {
            found = 0;
            break;
        }
        usleep(10000);
    }

    assert_int_equal(found, 0);
    LOG_INFO(LOG_CAT_CORE, "[TEST] Successfully synchronized %d records.", g_publish_count);

    snf_manager_destroy(mgr);
    sqlite_svc_destroy(db);
    // unlink("snf_resilience.db"); // Keep for analysis
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_snf_offline_buffering),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
