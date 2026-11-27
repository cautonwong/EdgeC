/**
 * @file test_dcu_runtime_health.c
 * @brief DCU-D0-02：演示行为必须只在 DCU_DEMO_MODE=1 下启用。
 *
 * 生产默认：心跳只刷盘 + 记日志，不冻结、不跳闸、不退出。
 * 台架演示：冻结、随机跳闸、周期自退、遥控触发全部恢复。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "dcu_event_monitor.h"
#include "dcu_heartbeat.h"
#include "dcu_inventory.h"
#include "dcu_node.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "stream_bus_datahub_adapter.h"
#include "stream_engine.h"

static dcu_heartbeat_t g_hb; /* 定时器句柄生命周期需越过单个用例栈帧 */

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int setup(void **state) {
    uv_loop_t *loop = (uv_loop_t *)malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    uv_walk(loop, on_walk_close, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    free(loop);
    return 0;
}

static const char *unique_db_path(char *buf, size_t cap, const char *tag) {
    snprintf(buf, cap, "/tmp/%s_%ld.db", tag, (long)getpid());
    return buf;
}

static void seed_inventory(const char *db_path, int meter_count) {
    unlink(db_path);
    sqlite3 *db = NULL;
    assert_int_equal(sqlite3_open(db_path, &db), SQLITE_OK);
    assert_int_equal(sqlite3_exec(db,
                                  "CREATE TABLE meter_inventory (address TEXT PRIMARY KEY, "
                                  "protocol TEXT, baud INTEGER, channel INTEGER, wiring_mode "
                                  "INTEGER, collector_addr TEXT, asset_no TEXT, pt_ratio "
                                  "INTEGER, ct_ratio INTEGER);",
                                  NULL, NULL, NULL),
                     SQLITE_OK);

    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db,
                                        "INSERT INTO meter_inventory VALUES "
                                        "(?,?,?,?,?,?,?,?,?)",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    for (int i = 0; i < meter_count; i++) {
        char addr[13];
        snprintf(addr, sizeof(addr), "%012d", i + 1);
        sqlite3_bind_text(stmt, 1, addr, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, "DLT645", -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, 2400);
        sqlite3_bind_int(stmt, 4, 22);
        sqlite3_bind_int(stmt, 5, 1);
        sqlite3_bind_text(stmt, 6, "123456789012", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 7, addr, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 8, 1);
        sqlite3_bind_int(stmt, 9, 1);
        assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

typedef struct {
    int calls;
    char meter_addr[MAX_ADDR_LEN];
    uint16_t control_oi;
    bool trip;
} recorded_control_t;

static int record_control(void *ctx, const char *meter_addr, uint16_t control_oi, bool trip) {
    recorded_control_t *rec = (recorded_control_t *)ctx;
    rec->calls++;
    snprintf(rec->meter_addr, sizeof(rec->meter_addr), "%s", meter_addr);
    rec->control_oi = control_oi;
    rec->trip = trip;
    return 0;
}

static void run_demo_control_tick(dcu_node_t *node, dcu_heartbeat_t *hb) {
    memset(hb, 0, sizeof(*hb));
    hb->demo_mode = true;
    hb->node = node;
    dcu_heartbeat_control_tick(hb);
}

static dcu_node_t *create_node(uv_loop_t *loop, const char *db_path, void **mem_out) {
    dcu_node_config_t cfg = {.product_name = "DCU_HealthTest",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};
    size_t size = dcu_node_storage_size(&cfg);
    void *mem = malloc(size);
    assert_non_null(mem);
    dcu_node_t *node = dcu_node_create_in(mem, size, loop, &cfg);
    assert_non_null(node);
    *mem_out = mem;
    return node;
}

static void close_heartbeat(dcu_heartbeat_t *hb) {
    if (!uv_is_closing((uv_handle_t *)&hb->freeze_timer)) {
        uv_close((uv_handle_t *)&hb->freeze_timer, NULL);
    }
    if (!uv_is_closing((uv_handle_t *)&hb->control_timer)) {
        uv_close((uv_handle_t *)&hb->control_timer, NULL);
    }
}

static int count_rows(sqlite3 *db, const char *table) {
    char sql[128];
    snprintf(sql, sizeof(sql), "SELECT count(*) FROM %s;", table);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL), SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    int n = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}

static int _archive_freeze_to_storage(void *ctx, const char *addr, uint32_t freeze_oad,
                                      uint64_t freeze_time, uint32_t src_oad, double value) {
    return dcu_storage_write_freeze((dcu_storage_t *)ctx, addr, freeze_oad, freeze_time, src_oad,
                                    value);
}

static int freeze_rows_for(dcu_storage_t *storage, const char *addr) {
    dcu_freeze_record_t rows[8];
    uint16_t count = 0;
    assert_int_equal(dcu_storage_query_freeze(storage, addr, OAD_DAILY_FREEZE, 0,
                                              0x7FFFFFFFFFFFFFFFULL, rows, 8, &count),
                     0);
    return count;
}

/* 默认生产模式：3 个周期内不冻结、不跳闸、不触发遥控、不退出，但仍刷盘。 */
static void test_production_cycle_has_no_demo_behavior(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_runtime_health_prod");
    unsetenv("DCU_DEMO_MODE");
    unsetenv("DCU_HEARTBEAT_CYCLES");
    unsetenv("DCU_CONTROL_DELAY_MS");

    seed_inventory(db_path, 1);
    void *mem = NULL;
    dcu_node_t *node = create_node(loop, db_path, &mem);
    dcu_storage_t *storage = dcu_node_get_storage(node);
    assert_int_equal(dcu_storage_write_telemetry(storage, "000000000001", 0, OAD_VOLTAGE, 220.0),
                     0);

    dcu_heartbeat_start(&g_hb, loop, node, NULL, NULL);
    assert_false(g_hb.demo_mode);
    assert_int_equal(g_hb.max_cycles, 0);

    dcu_heartbeat_tick(&g_hb);
    dcu_heartbeat_tick(&g_hb);
    dcu_heartbeat_tick(&g_hb);

    assert_int_equal(g_hb.test_cycles, 3);
    assert_int_equal(g_hb.demo_freezes, 0);
    assert_int_equal(g_hb.demo_relays, 0);
    assert_int_equal(g_hb.control_triggers, 0);
    assert_int_equal(g_hb.stop_requested, 0);

    /* 生产心跳仍会刷盘：缓存里的那条遥测已落到 SQLite。 */
    assert_int_equal(count_rows(storage->db, "dcu_telemetry_latest"), 1);

    close_heartbeat(&g_hb);
    uv_run(loop, UV_RUN_NOWAIT);
    dcu_node_destroy(node);
    free(mem);
    unlink(db_path);
}

/* 证明 storage 的创建、写入、刷盘、销毁都发生在同一个线程上：
 * 产品的从站/采集/存储共用 uv 循环线程，cmd/dcu 不创建线程。 */
static void test_node_lifecycle_runs_on_one_thread(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_runtime_health_thread");
    seed_inventory(db_path, 1);

    pthread_t before = pthread_self();
    void *mem = NULL;
    dcu_node_t *node = create_node(loop, db_path, &mem);
    pthread_t after_create = pthread_self();

    dcu_storage_t *storage = dcu_node_get_storage(node);
    assert_int_equal(dcu_storage_write_telemetry(storage, "000000000001", 0, OAD_VOLTAGE, 220.0),
                     0);
    dcu_storage_flush(storage);
    pthread_t after_write = pthread_self();

    dcu_node_destroy(node);
    pthread_t after_destroy = pthread_self();

    assert_true(pthread_equal(before, after_create));
    assert_true(pthread_equal(before, after_write));
    assert_true(pthread_equal(before, after_destroy));

    free(mem);
    unlink(db_path);
}

/* 演示模式：冻结、随机跳闸、遥控、3 周期自退全部按旧行为触发。 */
static void test_demo_mode_restores_bench_behavior(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_runtime_health_demo");
    setenv("DCU_DEMO_MODE", "1", 1);
    setenv("DCU_HEARTBEAT_CYCLES", "3", 1);
    setenv("DCU_CONTROL_DELAY_MS", "0", 1);

    /* 201 块表：跳闸逻辑每 200 块触发一次，i=200 会被命中。 */
    seed_inventory(db_path, 201);
    void *mem = NULL;
    dcu_node_t *node = create_node(loop, db_path, &mem);

    double value = 220.0;
    assert_int_equal(
        dcu_node_record_reading(node, 0, 0x02010100, (const uint8_t *)&value, sizeof(value)), 0);

    dcu_heartbeat_start(&g_hb, loop, node, NULL, NULL);
    assert_true(g_hb.demo_mode);
    assert_int_equal(g_hb.max_cycles, 3);

    dcu_heartbeat_control_tick(&g_hb);
    assert_int_equal(g_hb.control_triggers, 1);

    dcu_heartbeat_tick(&g_hb);
    assert_int_equal(g_hb.demo_freezes, 201);
    assert_int_equal(g_hb.demo_relays, 1);
    assert_int_equal(g_hb.stop_requested, 0);

    dcu_heartbeat_tick(&g_hb);
    dcu_heartbeat_tick(&g_hb);
    assert_int_equal(g_hb.test_cycles, 3);
    assert_int_equal(g_hb.stop_requested, 1); /* 第 3 个周期后自退 */

    close_heartbeat(&g_hb);
    uv_run(loop, UV_RUN_NOWAIT);
    dcu_node_destroy(node);
    free(mem);
    unlink(db_path);

    unsetenv("DCU_DEMO_MODE");
    unsetenv("DCU_HEARTBEAT_CYCLES");
    unsetenv("DCU_CONTROL_DELAY_MS");
}

/* 演示冻结触发必须带表的稳定 slot_id。删除 slot 0 后，末元素 slot 2 顶到下标 0，
 * slot 1 留在下标 1；触发若按下标发，下标 0 的表一条冻结都收不到。 */
static void test_demo_freeze_trigger_uses_slot_identity(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_runtime_health_slot");

    seed_inventory(db_path, 3);
    void *mem = NULL;
    dcu_node_t *node = create_node(loop, db_path, &mem);
    dcu_inventory_t *inv = dcu_node_get_inventory(node);
    dcu_storage_t *storage = dcu_node_get_storage(node);

    /* 删除 slot 0：slot 2 顶到下标 0，slot 1 留在下标 1，slot 与下标分叉。 */
    assert_int_equal(dcu_inventory_remove(inv, "000000000001"), 0);
    assert_int_equal(inv->count, 2);
    assert_string_equal(inv->meters[0].address, "000000000003");
    assert_int_equal(inv->meters[0].slot_id, 2);
    assert_int_equal(inv->meters[1].slot_id, 1);

    /* demo 分支只在“本轮有落库”时进冻结管线。 */
    double value = 220.0;
    assert_int_equal(
        dcu_node_record_reading(node, 2, 0x02010100, (const uint8_t *)&value, sizeof(value)), 0);

    datahub_t *hub = dcu_node_get_datahub(node);
    stream_engine_t *se = stream_engine_create_datahub(hub, loop, NULL, NULL);
    assert_non_null(se);
    pipeline_t *pipeline = stream_engine_add_pipeline(se, "TEST_HEARTBEAT_FREEZE", 0);
    event_sink_ops_t sink = {.ctx = storage, .archive_freeze = _archive_freeze_to_storage};
    pipeline_stage_t *stage = stage_periodic_freeze_create_ex(inv, hub, sink);
    assert_non_null(stage);
    pipeline_add_stage(pipeline, stage->name, stage->process, stage->priv_state,
                       stage->cleanup_state, NULL, NULL);
    free(stage); /* 管线复制 stage 字段，工厂对象只是临时描述符。 */

    dcu_heartbeat_t hb;
    memset(&hb, 0, sizeof(hb));
    hb.demo_mode = true;
    hb.node = node;
    hb.se = se;
    dcu_heartbeat_tick(&hb);

    /* 每只表 3 个冻结源。按下标发触发时 slot 2 一条没有。 */
    assert_int_equal(freeze_rows_for(storage, "000000000002"), 3);
    assert_int_equal(freeze_rows_for(storage, "000000000003"), 3);

    stream_engine_destroy(se);
    dcu_node_destroy(node);
    free(mem);
    unlink(db_path);
}

/* demo 遥控必须发到档案第一只表的稳定 slot；按下标 0 发会打到另一只表（或 NOT_FOUND）。 */
static void test_demo_control_targets_first_meter_slot(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_runtime_health_ctl");

    seed_inventory(db_path, 2);
    void *mem = NULL;
    dcu_node_t *node = create_node(loop, db_path, &mem);
    dcu_inventory_t *inv = dcu_node_get_inventory(node);

    /* 删除 slot 0：000000000002 成为档案第一只表，slot_id = 1，slot 与下标分叉。 */
    assert_int_equal(dcu_inventory_remove(inv, "000000000001"), 0);
    assert_int_equal(inv->count, 1);
    assert_string_equal(inv->meters[0].address, "000000000002");
    assert_int_equal(inv->meters[0].slot_id, 1);

    recorded_control_t rec;
    memset(&rec, 0, sizeof(rec));
    dcu_router_set_control_sink(record_control, &rec);

    dcu_heartbeat_t hb;
    run_demo_control_tick(node, &hb);

    assert_int_equal(hb.control_triggers, 1);
    assert_int_equal(rec.calls, 1);
    assert_string_equal(rec.meter_addr, "000000000002");
    assert_int_equal(rec.control_oi, OI_REMOTE_CONTROL);
    assert_true(rec.trip);

    dcu_node_destroy(node);
    free(mem);
    unlink(db_path);
}

/* 空档案时跳过遥控，绝不回落到 slot 0。 */
static void test_demo_control_skips_empty_inventory(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path =
        unique_db_path(db_file, sizeof(db_file), "test_dcu_runtime_health_ctl_missing");

    seed_inventory(db_path, 1);
    void *mem = NULL;
    dcu_node_t *node = create_node(loop, db_path, &mem);
    dcu_inventory_t *inv = dcu_node_get_inventory(node);
    assert_int_equal(dcu_inventory_remove(inv, "000000000001"), 0);
    assert_int_equal(inv->count, 0);

    recorded_control_t rec;
    memset(&rec, 0, sizeof(rec));
    dcu_router_set_control_sink(record_control, &rec);

    dcu_heartbeat_t hb;
    run_demo_control_tick(node, &hb);

    assert_int_equal(hb.control_triggers, 1);
    assert_int_equal(rec.calls, 0);

    dcu_node_destroy(node);
    free(mem);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_production_cycle_has_no_demo_behavior, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_demo_mode_restores_bench_behavior, setup, teardown),
        cmocka_unit_test_setup_teardown(test_node_lifecycle_runs_on_one_thread, setup, teardown),
        cmocka_unit_test_setup_teardown(test_demo_freeze_trigger_uses_slot_identity, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_demo_control_targets_first_meter_slot, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_demo_control_skips_empty_inventory, setup,
                                        teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
