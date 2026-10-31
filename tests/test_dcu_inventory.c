#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_inventory.h"

// 1. 验证档案初始化与添加电表
static void test_inventory_add_remove(void** state) {
    (void)state;
    dcu_inventory_t inv;

    // 初始化
    int ret = dcu_inventory_init(&inv);
    assert_int_equal(ret, 0);
    assert_int_equal(inv.count, 0);
    assert_int_equal((int)inv.revision, 0); // revision 必须显式归零：poller 靠它察觉档案变更

    // 添加电表表1 (TSA: 10000001, 协议: dlt698, 波特率: 9600, 通道: 1)
    ret = dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1);
    assert_int_equal(ret, 0);
    assert_int_equal(inv.count, 1);
    assert_int_equal((int)inv.revision, 1);

    // 获取表1，验证其各项属性
    production_meter_t* m = dcu_inventory_get(&inv, "10000001");
    assert_non_null(m);
    assert_string_equal(m->address, "10000001");
    assert_string_equal(m->protocol, "dlt698");
    assert_int_equal(m->baud_rate, 9600);
    assert_int_equal(m->channel_id, 1);
    assert_true(m->is_online);           // 初始在线

    // 重复添加同一只表应该报错，保障唯一性
    uint32_t rev_before = inv.revision;
    ret = dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1);
    // 重复添加失败：revision 不得变化（否则 poller 会白做一次退休扫描）
    assert_int_not_equal(ret, 0);
    assert_int_equal((int)inv.revision, (int)rev_before);
    // 添加表2
    ret = dcu_inventory_add(&inv, "10000002", "dlt645", 2400, 2);
    assert_int_equal(ret, 0);
    assert_int_equal(inv.count, 2);

    // 删除表1
    ret = dcu_inventory_remove(&inv, "10000001");
    assert_int_equal(ret, 0);
    assert_int_equal(inv.count, 1);
    assert_int_equal((int)inv.revision, (int)rev_before + 2); // 一次成功添加 + 一次删除

    // 此时获取表1应该返回空
    m = dcu_inventory_get(&inv, "10000001");
    assert_null(m);

    dcu_inventory_destroy(&inv);
}

// 2. 通道协议从档案取, 不按通道号猜
static void test_inventory_channel_protocol(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);

    // 空通道 → NULL
    assert_null(dcu_inventory_channel_protocol(&inv, 22));

    // 同一通道两只 645 表
    dcu_inventory_add(&inv, "10000001", "DLT645", 9600, 22);
    dcu_inventory_add(&inv, "10000002", "DLT645", 9600, 22);
    assert_string_equal(dcu_inventory_channel_protocol(&inv, 22), "DLT645");
    // 其它通道仍然空
    assert_null(dcu_inventory_channel_protocol(&inv, 5));

    // 通道 5 放一只 698 表: 协议必须来自档案, 而不是通道号 <22 就猜 698
    dcu_inventory_add(&inv, "10000003", "dlt698", 9600, 5);
    assert_string_equal(dcu_inventory_channel_protocol(&inv, 5), "dlt698");

    // 混协议: 取首个, 不崩
    dcu_inventory_add(&inv, "10000004", "DLT698", 9600, 22);
    assert_string_equal(dcu_inventory_channel_protocol(&inv, 22), "DLT645");

    dcu_inventory_destroy(&inv);
}

// 3. 装载只按真正入库的行数递增 revision（超容量被丢的行不算变更）
static void test_inventory_load_revision(void** state) {
    (void)state;
    const char* db_path = "/tmp/test_dcu_inv_load.db";
    remove(db_path);

    sqlite3* db = NULL;
    assert_int_equal(sqlite3_open(db_path, &db), SQLITE_OK);
    assert_int_equal(sqlite3_exec(db,
                                  "CREATE TABLE meter_inventory(address TEXT PRIMARY KEY, "
                                  "protocol TEXT, baud INTEGER, channel INTEGER, wiring_mode "
                                  "INTEGER, collector_addr TEXT, asset_no TEXT, pt_ratio INTEGER, "
                                  "ct_ratio INTEGER);",
                                  NULL, NULL, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_exec(db,
                                  "INSERT INTO meter_inventory VALUES"
                                  "('LOAD_A','dlt698',9600,1,0,NULL,NULL,0,0),"
                                  "('LOAD_B','dlt698',9600,1,0,NULL,NULL,0,0);",
                                  NULL, NULL, NULL),
                     SQLITE_OK);
    sqlite3_close(db);

    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    int loaded = dcu_inventory_load_from_db(&inv, db_path);
    assert_int_equal(loaded, 2);
    assert_int_equal(inv.count, 2);
    assert_int_equal((int)inv.revision, 2); // 入库 2 行 → 版本号 +2

    // 只剩 1 个空位时再装：读到 2 行但只入库 1 行，revision 只能 +1
    dcu_inventory_init(&inv);
    inv.count = MAX_METERS - 1;
    loaded = dcu_inventory_load_from_db(&inv, db_path);
    assert_int_equal(loaded, 2);
    assert_int_equal(inv.count, MAX_METERS);
    assert_int_equal((int)inv.revision, 1); // 超出容量的行不算变更

    dcu_inventory_destroy(&inv);
    remove(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_inventory_add_remove),
        cmocka_unit_test(test_inventory_channel_protocol),
        cmocka_unit_test(test_inventory_load_revision),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
