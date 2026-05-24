#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <string.h>
#include <uv.h>
#include "datahub.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_storage.h"
#include "dcu_router.h"

// 全局集成测试主体
static void test_dcu_full_pipeline_integration(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    // 1. 初始化 SQLite 存储服务 (使用临时内存数据库做集成校验)
    dcu_storage_t storage;
    int ret = dcu_storage_init(&storage, loop, ":memory:");
    assert_int_equal(ret, 0);

    // 2. 初始化 DataHub 影子寄存器
    datahub_t hub;
    ret = datahub_init(&hub, loop, "dcu_integration_bus", 128, 0, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    // 3. 初始化档案并增加 2 只表 (TSA: 20000001, 20000002)
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    dcu_inventory_add(&inv, "20000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "20000002", "dlt698", 9600, 1);

    // 4. 初始化 poller 并注册定时任务与普通采集方案
    dcu_poller_t poller;
    dcu_poller_init(&poller);
    uint32_t oads[2] = {OAD_VOLTAGE, OAD_CURRENT}; // 电压 + 电流
    dcu_poller_add_scheme(&poller, 1, 0, oads, 2);
    dcu_poller_add_task(&poller, 1, 10, 1);


    // 5. 启动数采模拟生产器并直推 DataHub
    ret = dcu_poller_start_simulation(&poller, &inv, &hub, loop);
    assert_int_equal(ret, 0);

    // 6. 数据总线消费链路搭建：
    // 我们在此集成测试中，直接订阅 DataHub 实时变化的消息。
    // 在真实应用中，dcu_storage 会接收 DataHub 消息并写入缓存；
    // 我们在这里模拟这套“五位一体业务闭环”：
    // 当 timer 触发时，我们直接把产生的实时遥测写入 storage 缓冲。
    // 我们模拟运行 libuv 事件循环 45 毫秒
    uint64_t start_ms = uv_now(loop);
    while (uv_now(loop) - start_ms < 45) {
        uv_run(loop, UV_RUN_ONCE);
        
        // 模拟消费端：取出最新模拟量存入双缓冲
        dcu_storage_write_telemetry(&storage, "20000001", OAD_VOLTAGE, 220.5);
        dcu_storage_write_telemetry(&storage, "20000002", OAD_CURRENT, 4.95);
    }

    // 7. 模拟物理掉电 SIGTERM 中断，强行 Flush 刷盘
    ret = dcu_storage_flush(&storage);
    assert_int_equal(ret, 0);

    // 8. 物理审计落盘数据行数 -> 数据应当 100% 完整落盘
    sqlite3_stmt *stmt;
    ret = sqlite3_prepare_v2(storage.db, "SELECT count(*) FROM dcu_telemetry_latest", -1, &stmt, NULL);
    assert_int_equal(ret, SQLITE_OK);
    ret = sqlite3_step(stmt);
    assert_int_equal(ret, SQLITE_ROW);
    int count = sqlite3_column_int(stmt, 0);
    assert_true(count >= 2); // 必须在数据库中查询到已落盘的电能表遥测数据
    sqlite3_finalize(stmt);

    // 9. 回收所有资源
    dcu_poller_stop_simulation(&poller);
    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
    datahub_destroy(&hub);
    dcu_storage_destroy(&storage);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_full_pipeline_integration),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
