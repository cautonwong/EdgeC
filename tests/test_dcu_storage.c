#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uv.h>

#include "dcu_router.h"
#include "dcu_storage.h"

// 1. 验证双缓冲与批量事务落盘 (Flash 寿命保障)
static void test_storage_buffering(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    dcu_storage_t storage;

    // 初始化 SQLite，并在内存中创建临时数据库以便单元测试快速收敛
    int ret = dcu_storage_init(&storage, loop, ":memory:");
    assert_int_equal(ret, 0);

    // 写入 5 条最新实时遥测数据
    dcu_storage_write_telemetry(&storage, "10000001", 1, OAD_VOLTAGE, 220.4);
    dcu_storage_write_telemetry(&storage, "10000001", 1, OAD_CURRENT, 5.02);
    dcu_storage_write_telemetry(&storage, "10000002", 2, OAD_VOLTAGE, 221.1);
    dcu_storage_write_telemetry(&storage, "10000002", 2, OAD_CURRENT, 4.98);
    dcu_storage_write_telemetry(&storage, "10000003", 3, OAD_VOLTAGE, 219.8);

    // 未进行 flush 刷写前，由于双缓冲保护，临时数据库中记录行数应当为 0
    sqlite3_stmt* stmt;
    ret = sqlite3_prepare_v2(storage.db, "SELECT count(*) FROM dcu_telemetry_latest", -1, &stmt,
                             NULL);
    assert_int_equal(ret, SQLITE_OK);
    ret = sqlite3_step(stmt);
    assert_int_equal(ret, SQLITE_ROW);
    int count = sqlite3_column_int(stmt, 0);
    assert_int_equal(count, 0);  // 缓冲起效，未落盘
    sqlite3_finalize(stmt);

    // 显式触发事务合并刷盘
    ret = dcu_storage_flush(&storage);
    assert_int_equal(ret, 0);

    // 刷盘后，数据应当以单个批量事务原子化存盘
    ret = sqlite3_prepare_v2(storage.db, "SELECT count(*) FROM dcu_telemetry_latest", -1, &stmt,
                             NULL);
    assert_int_equal(ret, SQLITE_OK);
    ret = sqlite3_step(stmt);
    assert_int_equal(ret, SQLITE_ROW);
    count = sqlite3_column_int(stmt, 0);
    assert_int_equal(count, 5);  // 5 条数据全部安全原子存盘
    sqlite3_finalize(stmt);

    dcu_storage_destroy(&storage);
}

// 2. 验证 Class 9 历史断面冻结数据时空快照写入与带 RSD 时间区间的高性能过滤查询
static void test_storage_freeze_rsd_query(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    dcu_storage_t storage;

    dcu_storage_init(&storage, loop, ":memory:");

    // 模拟写入一系列日冻结数据快照 (OAD_DAILY_FREEZE)
    // 昨天 (时间戳 100000)，正向有功电量快照为 12543.10
    dcu_storage_write_freeze(&storage, "10000001", OAD_DAILY_FREEZE, 100000, OAD_ACTIVE_ENERGY,
                             12543.10);
    // 今天 (时间戳 200000)，正向有功电量快照为 12544.15
    dcu_storage_write_freeze(&storage, "10000001", OAD_DAILY_FREEZE, 200000, OAD_ACTIVE_ENERGY,
                             12544.15);
    // 明天 (时间戳 300000)，正向有功电量快照为 12545.20
    dcu_storage_write_freeze(&storage, "10000001", OAD_DAILY_FREEZE, 300000, OAD_ACTIVE_ENERGY,
                             12545.20);

    // 执行 RSD 范围段过滤检索 (模拟北向 GET-Request-Record，只查昨天到今天的区间：100000 到 250000)
    dcu_freeze_record_t results[10];
    uint16_t out_count = 0;

    int ret = dcu_storage_query_freeze(&storage, "10000001", OAD_DAILY_FREEZE, 100000, 250000,
                                       results, 10, &out_count);
    assert_int_equal(ret, 0);

    // 断言结果：应当只筛选出昨天和今天的 2 条记录，明天的被自动过滤
    assert_int_equal(out_count, 2);

    assert_string_equal(results[0].meter_address, "10000001");
    assert_int_equal(results[0].freeze_time, 100000);
    assert_int_equal(results[0].source_oad, OAD_ACTIVE_ENERGY);
    assert_double_equal(results[0].frozen_value, 12543.10, 0.01);

    assert_int_equal(results[1].freeze_time, 200000);
    assert_double_equal(results[1].frozen_value, 12544.15, 0.01);

    dcu_storage_destroy(&storage);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_storage_buffering),
        cmocka_unit_test(test_storage_freeze_rsd_query),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
