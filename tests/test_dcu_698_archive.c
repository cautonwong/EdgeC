#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include "dcu_698_object_internal.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "edge_slave_plane.h"

#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

static dcu_inventory_t g_inv;
static dcu_poller_t g_poller;
static sqlite3 *g_db;
static char g_db_path[64];

static int _save(void *ctx, const production_meter_t *m) {
    return dcu_inventory_save_meter((sqlite3 *)ctx, m);
}

static int _delete(void *ctx, const char *address) {
    return dcu_inventory_delete_meter((sqlite3 *)ctx, address);
}

static int _clear(void *ctx) { return dcu_inventory_clear_db((sqlite3 *)ctx); }

static int _load_count(void) {
    dcu_inventory_t probe;
    dcu_inventory_init(&probe);
    int loaded = dcu_inventory_load_from_db(&probe, g_db_path);
    return loaded;
}

static dcu_archive_unit_t _unit(const char *addr, uint8_t port) {
    dcu_archive_unit_t u = {0};
    strncpy(u.address, addr, sizeof(u.address) - 1);
    u.baud_code = 6;      /* 9600 */
    u.protocol_code = 2;  /* DL/T645-2007 */
    u.port = port;
    u.tariff_count = 1;
    u.wiring_mode = 1;
    u.rated_voltage = 2200;
    u.rated_current = 500;
    return u;
}

static void _init_model(const router_sink_ops_t *sink) {
    dcu_inventory_init(&g_inv);
    dcu_poller_init(&g_poller);
    router_bus_ops_t bus = {0};
    assert_int_equal(dcu_698_object_model_init(&g_inv, &g_poller, *sink, NULL, NULL, NULL, NULL,
                                               bus),
                     0);
}

static int _call(uint8_t method, dcu_archive_op_t *op) {
    return dcu_698_call(MK_OMD(OI_METER_ARCHIVE, method), op, NULL);
}

static void test_add_delete_clear_round_trip(void **state) {
    (void)state;
    router_sink_ops_t sink = {.ctx = g_db, .archive_save = _save, .archive_delete = _delete,
                              .archive_clear = _clear};
    _init_model(&sink);

    dcu_archive_op_t op = {0};
    op.method = 127;
    op.unit_count = 1;
    op.units[0] = _unit("21000011", 22);
    assert_int_equal(_call(127, &op), 0);
    assert_int_equal(g_inv.count, 1);
    assert_int_equal(g_inv.meters[0].config_seq, 1);
    assert_int_equal(_load_count(), 1);

    dcu_inventory_t probe;
    dcu_inventory_init(&probe);
    assert_int_equal(dcu_inventory_load_from_db(&probe, g_db_path), 1);
    assert_string_equal(probe.meters[0].address, "21000011");
    assert_int_equal(probe.meters[0].baud_rate, 9600);
    assert_int_equal(probe.meters[0].channel_id, 22);
    assert_int_equal(probe.meters[0].config_seq, 1);
    assert_int_equal(probe.meters[0].rated_voltage, 2200);

    dcu_archive_op_t del = {0};
    del.method = 131;
    del.config_seq = 1;
    assert_int_equal(_call(131, &del), 0);
    assert_int_equal(g_inv.count, 0);
    assert_int_equal(_load_count(), 0);

    op.units[0] = _unit("21000012", 23);
    assert_int_equal(_call(127, &op), 0);
    dcu_archive_op_t clear = {0};
    clear.method = 134;
    assert_int_equal(_call(134, &clear), 0);
    assert_int_equal(g_inv.count, 0);
    assert_int_equal(_load_count(), 0);

    dcu_698_object_model_reset();
}

static void test_add_batch_is_all_or_nothing(void **state) {
    (void)state;
    router_sink_ops_t sink = {.ctx = g_db, .archive_save = _save, .archive_delete = _delete,
                              .archive_clear = _clear};
    _init_model(&sink);

    dcu_archive_op_t one = {0};
    one.unit_count = 1;
    one.units[0] = _unit("21000021", 22);
    assert_int_equal(_call(127, &one), 0);

    dcu_archive_op_t batch = {0};
    batch.method = 128;
    batch.unit_count = 2;
    batch.units[0] = _unit("21000022", 22);
    batch.units[1] = _unit("21000021", 22);
    assert_int_equal(_call(128, &batch), EDGE_SLAVE_SRC_DENIED);
    assert_int_equal(g_inv.count, 1);
    assert_null(dcu_inventory_get(&g_inv, "21000022"));
    assert_int_equal(_load_count(), 1);

    dcu_698_object_model_reset();
}

static void test_update_and_delete_forms(void **state) {
    (void)state;
    router_sink_ops_t sink = {.ctx = g_db, .archive_save = _save, .archive_delete = _delete,
                              .archive_clear = _clear};
    _init_model(&sink);

    dcu_archive_op_t add = {0};
    add.unit_count = 1;
    add.units[0] = _unit("21000031", 22);
    assert_int_equal(_call(127, &add), 0);

    /* 129：按配置序号改基本信息，地址跟着换主键。 */
    dcu_archive_op_t upd = {0};
    upd.config_seq = 1;
    upd.unit_count = 1;
    upd.units[0] = _unit("21000032", 23);
    assert_int_equal(_call(129, &upd), 0);
    assert_non_null(dcu_inventory_get(&g_inv, "21000032"));
    assert_null(dcu_inventory_get(&g_inv, "21000031"));
    assert_int_equal(_load_count(), 1);

    /* 132：按基本信息删除。 */
    dcu_archive_op_t del_basic = {0};
    del_basic.unit_count = 1;
    del_basic.units[0] = _unit("21000032", 23);
    assert_int_equal(_call(132, &del_basic), 0);
    assert_int_equal(g_inv.count, 0);

    /* 133：按通信地址+端口删除。 */
    add.units[0] = _unit("21000033", 22);
    assert_int_equal(_call(127, &add), 0);
    dcu_archive_op_t del_port = {0};
    del_port.unit_count = 1;
    del_port.units[0] = _unit("21000033", 22);
    assert_int_equal(_call(133, &del_port), 0);
    assert_int_equal(g_inv.count, 0);

    dcu_698_object_model_reset();
}

static void test_add_without_persistence_is_refused(void **state) {
    (void)state;
    router_sink_ops_t sink = {0};
    _init_model(&sink);

    dcu_archive_op_t op = {0};
    op.unit_count = 1;
    op.units[0] = _unit("21000041", 22);
    assert_int_equal(_call(127, &op), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(g_inv.count, 0);

    dcu_698_object_model_reset();
}

static void test_decode_add_param_from_axdr(void **state) {
    (void)state;
    /* structure{配置序号 1, Basic{地址 21000051, enum 9600, enum 645-2007, OAD port 22,
     * octets password "", unsigned 1, unsigned 0, enum 1, LU 2200, LU 500},
     * Extended{octets "", octets "", LU 1, LU 1}, array 0} */
    const uint8_t axdr[] = {
        0x02, 0x04, 0x12, 0x00, 0x01,
        0x02, 0x0A, 0x09, 0x08, '2', '1', '0', '0', '0', '0', '5', '1', 0x16, 0x06, 0x16,
        0x02, 0x51, 0x00, 0x00, 0x00, 0x16, 0x09, 0x00, 0x11, 0x01, 0x11, 0x00, 0x16, 0x01,
        0x12, 0x08, 0x98, 0x12, 0x01, 0xF4,
        0x02, 0x04, 0x09, 0x00, 0x09, 0x00, 0x12, 0x00, 0x01, 0x12, 0x00, 0x01,
        0x01, 0x00,
        0x00};
    dcu_archive_op_t op;
    assert_int_equal(dcu_698_decode_archive_op(127, axdr, sizeof(axdr), &op), 0);
    assert_int_equal(op.unit_count, 1);
    assert_int_equal(op.units[0].config_seq, 1);
    assert_string_equal(op.units[0].address, "21000051");
    assert_int_equal(op.units[0].baud_code, 6);
    assert_int_equal(op.units[0].protocol_code, 2);
    assert_int_equal(op.units[0].port, 22);
    assert_int_equal(op.units[0].tariff_count, 1);
    assert_int_equal(op.units[0].wiring_mode, 1);
    assert_int_equal(op.units[0].rated_voltage, 2200);
    assert_int_equal(op.units[0].rated_current, 500);
    assert_false(op.has_annex);
}

static void test_load_assigns_config_seq_from_one(void **state) {
    (void)state;
    /* 组合根只 memset 节点时 next_config_seq 为 0：加载必须从 1 开始补号。 */
    dcu_inventory_t fresh;
    memset(&fresh, 0, sizeof(fresh));
    production_meter_t m = {0};
    strncpy(m.address, "21000061", sizeof(m.address) - 1);
    strncpy(m.protocol, "DLT645", sizeof(m.protocol) - 1);
    m.protocol_code = 2;
    m.baud_code = 6;
    m.baud_rate = 9600;
    m.channel_id = 22;
    assert_int_equal(dcu_inventory_save_meter(g_db, &m), 0);

    assert_int_equal(dcu_inventory_load_from_db(&fresh, g_db_path), 1);
    assert_int_equal(fresh.meters[0].config_seq, 1);
    dcu_inventory_delete_meter(g_db, "21000061");
}

static void test_archive_array_carries_added_unit(void **state) {
    (void)state;
    router_sink_ops_t sink = {.ctx = g_db, .archive_save = _save, .archive_delete = _delete,
                              .archive_clear = _clear};
    _init_model(&sink);

    dcu_archive_op_t op = {0};
    op.unit_count = 1;
    op.units[0] = _unit("21000071", 22);
    assert_int_equal(_call(127, &op), 0);

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(dcu_698_archive_encode_axdr(&g_inv, MK_OMD(OI_METER_ARCHIVE, 2), buf,
                                                 sizeof(buf), &len),
                     0);
    assert_int_equal(buf[0], 0x01); /* array */
    assert_int_equal(buf[1], 1);
    const char* found = NULL;
    for (size_t i = 0; i + 8 <= len; i++) {
        if (memcmp(buf + i, "21000071", 8) == 0) {
            found = (const char*)buf + i;
            break;
        }
    }
    assert_non_null(found);

    dcu_698_object_model_reset();
}

static int setup(void **state) {
    (void)state;
    snprintf(g_db_path, sizeof(g_db_path), "/tmp/test_dcu_698_archive_%d.db", (int)getpid());
    unlink(g_db_path);
    assert_int_equal(sqlite3_open(g_db_path, &g_db), SQLITE_OK);
    dcu_inventory_t probe;
    dcu_inventory_init(&probe);
    assert_int_equal(dcu_inventory_load_from_db(&probe, g_db_path), 0);
    return 0;
}

static int teardown(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    if (g_db) sqlite3_close(g_db);
    g_db = NULL;
    unlink(g_db_path);
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_add_delete_clear_round_trip, setup, teardown),
        cmocka_unit_test_setup_teardown(test_add_batch_is_all_or_nothing, setup, teardown),
        cmocka_unit_test_setup_teardown(test_update_and_delete_forms, setup, teardown),
        cmocka_unit_test_setup_teardown(test_add_without_persistence_is_refused, setup, teardown),
        cmocka_unit_test(test_decode_add_param_from_axdr),
        cmocka_unit_test_setup_teardown(test_load_assigns_config_seq_from_one, setup, teardown),
        cmocka_unit_test_setup_teardown(test_archive_array_carries_added_unit, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
