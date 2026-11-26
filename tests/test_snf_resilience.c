#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"
#include "snf_manager.h"
#include "snf_sqlite_adapter.h"
#include "sqlite_service.h"

static int g_publish_count;

static uv_loop_t* g_loop;
static sqlite_svc_t* g_db;
static snf_manager_t* g_mgr;
static snf_store_port_t g_store;
static const char* g_db_path = "snf_resilience.db";

static int mock_publish(void* user_data, const PipeMsg* msg, uint64_t wall_ts_us) {
    (void)user_data;
    (void)msg;
    (void)wall_ts_us;
    g_publish_count++;
    return 0;
}

static void remove_db_files(const char* path) {
    if (!path) return;
    unlink(path);
    char sidecar[256];
    snprintf(sidecar, sizeof(sidecar), "%s-wal", path);
    unlink(sidecar);
    snprintf(sidecar, sizeof(sidecar), "%s-shm", path);
    unlink(sidecar);
}

static int fixture_setup(void** state) {
    (void)state;
    g_loop = uv_default_loop();
    g_db = NULL;
    g_mgr = NULL;
    g_publish_count = 0;
    remove_db_files(g_db_path);
    return 0;
}

/* 断言失败时仍然销毁句柄并删库，避免残留定时器与 DB 文件。 */
static int fixture_teardown(void** state) {
    (void)state;
    if (g_mgr) {
        snf_manager_destroy(g_mgr);
        for (int i = 0; i < 10; i++) {
            uv_run(g_loop, UV_RUN_NOWAIT);
            usleep(5000);
        }
        g_mgr = NULL;
    }
    if (g_db) {
        sqlite_svc_destroy(g_db);
        g_db = NULL;
    }
    remove_db_files(g_db_path);
    return 0;
}

static void test_snf_offline_buffering(void** state) {
    (void)state;
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE,
                              .db_path = g_db_path,
                              .batch_size = 0,
                              .flush_interval_ms = 0};
    g_db = sqlite_svc_create(g_loop, &db_cfg);
    assert_non_null(g_db);

    g_store = snf_sqlite_store_port(g_db);
    snf_config_t snf_cfg = {.store = &g_store,
                            .loop = g_loop,
                            .publish_fn = mock_publish,
                            .sync_batch_size = 10,
                            .sync_interval_ms = 100};
    g_mgr = snf_manager_create(&snf_cfg);
    assert_non_null(g_mgr);

    // 1. Go Offline
    snf_manager_set_online(g_mgr, false);

    // 2. Buffer data
    LOG_INFO(LOG_CAT_CORE, "[TEST] Buffering 5 records...");
    for (int i = 0; i < 5; i++) {
        PipeMsg msg = {.ts = (uint64_t)i + 2000, .tag_id = 1001, .payload.val.f = (double)i};
        msg.type = PIPEMSG_TYPE_SCALAR;
        int rc = snf_manager_buffer_data(g_mgr, &msg);
        assert_int_equal(rc, 0);
    }

    // Ensure no publishing while offline
    for (int i = 0; i < 50; i++) uv_run(g_loop, UV_RUN_NOWAIT);
    assert_int_equal(g_publish_count, 0);

    // [Nexus] Verify DB content before going online
    sqlite3* sqlite = (sqlite3*)sqlite_svc_get_db_handle(g_db);
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
    snf_manager_set_online(g_mgr, true);

    // 4. Wait for synchronization
    int found = -1;
    for (int retry = 0; retry < 100; retry++) {
        uv_run(g_loop, UV_RUN_NOWAIT);
        if (g_publish_count >= 5) {
            found = 0;
            break;
        }
        usleep(10000);
    }

    assert_int_equal(found, 0);
    LOG_INFO(LOG_CAT_CORE, "[TEST] Successfully synchronized %d records.", g_publish_count);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_snf_offline_buffering, fixture_setup,
                                        fixture_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
