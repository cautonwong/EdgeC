#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "contract_data_types.h"
#include "dcu_698_rules.h"
#include "dcu_driver_adapter.h"
#include "dcu_router.h"
#include "dcu_storage.h"

typedef struct {
    uv_loop_t loop;
    dcu_storage_t storage;
    dcu_rule_cache_t cache;
    char path[64];
} rules_bench_t;

typedef struct {
    uint8_t protocol;
    uint32_t oad;
    uint32_t protocol_id;
    float scale;
    uint8_t length;
} expected_rule_t;

static rules_bench_t g_bench;

static const expected_rule_t k_expected[] = {
    {DCU_RULE_DLT645_2007, OAD_VOLTAGE, 0x02010100, 0.1f, 2},
    {DCU_RULE_DLT645_2007, TAG_VOLTAGE_B, 0x02010200, 0.1f, 2},
    {DCU_RULE_DLT645_2007, TAG_VOLTAGE_C, 0x02010300, 0.1f, 2},
    {DCU_RULE_DLT645_2007, OAD_CURRENT, 0x02020100, 0.001f, 3},
    {DCU_RULE_DLT645_2007, TAG_CURRENT_B, 0x02020200, 0.001f, 3},
    {DCU_RULE_DLT645_2007, TAG_CURRENT_C, 0x02020300, 0.001f, 3},
    {DCU_RULE_DLT645_2007, OAD_ACTIVE_ENERGY, 0x00010000, 0.01f, 4},
    {DCU_RULE_DLT645_2007, OAD_REACTIVE_ENERGY, 0x00020000, 0.01f, 4},
    {DCU_RULE_DLT645_2007, OAD_ACTIVE_POWER_TOTAL, 0x02030000, 0.0001f, 3},
    {DCU_RULE_DLT645_2007, TAG_ACTIVE_POWER_A, 0x02030100, 0.0001f, 3},
    {DCU_RULE_DLT645_2007, TAG_ACTIVE_POWER_B, 0x02030200, 0.0001f, 3},
    {DCU_RULE_DLT645_2007, TAG_ACTIVE_POWER_C, 0x02030300, 0.0001f, 3},
    {DCU_RULE_DLT645_2007, OAD_REACTIVE_POWER_TOTAL, 0x02040000, 0.0001f, 3},
    {DCU_RULE_DLT645_2007, OAD_POWER_FACTOR_TOTAL, 0x02060000, 0.001f, 2},
    {DCU_RULE_DLT645_2007, OAD_GRID_FREQUENCY, 0x02800002, 0.01f, 2},
    {DCU_RULE_DLT698, OAD_VOLTAGE, OAD_VOLTAGE, 0.1f, 2},
    {DCU_RULE_DLT698, TAG_VOLTAGE_B, OAD_VOLTAGE_B_ELEM, 0.1f, 2},
    {DCU_RULE_DLT698, TAG_VOLTAGE_C, OAD_VOLTAGE_C_ELEM, 0.1f, 2},
    {DCU_RULE_DLT698, OAD_CURRENT, OAD_CURRENT, 0.01f, 3},
    {DCU_RULE_DLT698, TAG_CURRENT_B, OAD_CURRENT_B_ELEM, 0.01f, 3},
    {DCU_RULE_DLT698, TAG_CURRENT_C, OAD_CURRENT_C_ELEM, 0.01f, 3},
    {DCU_RULE_DLT698, OAD_ACTIVE_ENERGY, OAD_ACTIVE_ENERGY, 0.01f, 4},
    {DCU_RULE_DLT698, OAD_REACTIVE_ENERGY, OAD_REACTIVE_ENERGY, 0.01f, 4},
    {DCU_RULE_DLT698, OAD_ACTIVE_POWER_TOTAL, OAD_ACTIVE_POWER_TOTAL, 0.0001f, 3},
    {DCU_RULE_DLT698, TAG_ACTIVE_POWER_A, OAD_ACTIVE_POWER_A_ELEM, 0.0001f, 3},
    {DCU_RULE_DLT698, TAG_ACTIVE_POWER_B, OAD_ACTIVE_POWER_B_ELEM, 0.0001f, 3},
    {DCU_RULE_DLT698, TAG_ACTIVE_POWER_C, OAD_ACTIVE_POWER_C_ELEM, 0.0001f, 3},
    {DCU_RULE_DLT698, OAD_REACTIVE_POWER_TOTAL, OAD_REACTIVE_POWER_TOTAL, 0.0001f, 3},
    {DCU_RULE_DLT698, OAD_POWER_FACTOR_TOTAL, OAD_POWER_FACTOR_TOTAL, 0.001f, 2},
    {DCU_RULE_DLT698, OAD_GRID_FREQUENCY, OAD_GRID_FREQUENCY, 0.01f, 2},
    {DCU_RULE_DLT698, OAD_ZERO_SEQ_CURRENT, OAD_ZERO_SEQ_CURRENT, 0.01f, 3},
    {DCU_RULE_MODBUS, OAD_VOLTAGE, 0, 0.1f, 1},
    {DCU_RULE_MODBUS, TAG_VOLTAGE_B, 1, 0.1f, 1},
    {DCU_RULE_MODBUS, TAG_VOLTAGE_C, 2, 0.1f, 1},
    {DCU_RULE_MODBUS, OAD_CURRENT, 6, 0.01f, 2},
    {DCU_RULE_MODBUS, TAG_CURRENT_B, 7, 0.01f, 2},
    {DCU_RULE_MODBUS, TAG_CURRENT_C, 8, 0.01f, 2},
    {DCU_RULE_MODBUS, OAD_ACTIVE_POWER_TOTAL, 12, 0.001f, 2},
    {DCU_RULE_MODBUS, TAG_ACTIVE_POWER_A, 13, 0.001f, 2},
    {DCU_RULE_MODBUS, TAG_ACTIVE_POWER_B, 14, 0.001f, 2},
    {DCU_RULE_MODBUS, TAG_ACTIVE_POWER_C, 15, 0.001f, 2},
    {DCU_RULE_MODBUS, OAD_REACTIVE_POWER_TOTAL, 18, 0.001f, 2},
    {DCU_RULE_MODBUS, OAD_POWER_FACTOR_TOTAL, 24, 0.001f, 1},
    {DCU_RULE_MODBUS, OAD_GRID_FREQUENCY, 28, 0.01f, 1},
    {DCU_RULE_MODBUS, OAD_ACTIVE_ENERGY, 52, 0.01f, 2},
    {DCU_RULE_MODBUS, OAD_REACTIVE_ENERGY, 56, 0.01f, 2},
};

#define EXPECTED_COUNT (sizeof(k_expected) / sizeof(k_expected[0]))

static int _bench_init(rules_bench_t *b) {
    memset(b, 0, sizeof(*b));
    strcpy(b->path, "/tmp/dcu_698_rules_XXXXXX");
    int fd = mkstemp(b->path);
    if (fd < 0) return -1;
    close(fd);
    if (uv_loop_init(&b->loop) != 0) return -1;
    if (dcu_storage_init(&b->storage, &b->loop, b->path) != 0) return -1;
    return 0;
}

static void _bench_free(rules_bench_t *b) {
    dcu_storage_destroy(&b->storage);
    uv_run(&b->loop, UV_RUN_NOWAIT);
    uv_loop_close(&b->loop);
    unlink(b->path);
    char side[80];
    snprintf(side, sizeof(side), "%s-wal", b->path);
    unlink(side);
    snprintf(side, sizeof(side), "%s-shm", b->path);
    unlink(side);
}

static int _bench_reopen(rules_bench_t *b) {
    dcu_storage_destroy(&b->storage);
    uv_run(&b->loop, UV_RUN_NOWAIT);
    return dcu_storage_init(&b->storage, &b->loop, b->path);
}

static void _expect_row_matches(const dcu_rule_row_t *row, const expected_rule_t *want) {
    assert_int_equal(row->oad, want->oad);
    assert_int_equal(row->protocol_id, want->protocol_id);
    assert_int_equal(row->length, want->length);
    assert_true(row->scale > want->scale - 1e-6f && row->scale < want->scale + 1e-6f);
}

static void test_seed_lookup_before_init(void **state) {
    (void)state;
    dcu_rule_row_t row;
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_VOLTAGE, &row), 0);
    assert_int_equal(row.protocol_id, 0x02010100);
    assert_int_equal(row.length, 2);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT698, OAD_CURRENT, &row), 0);
    assert_int_equal(row.protocol_id, OAD_CURRENT);
    assert_int_equal(row.length, 3);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_MODBUS, TAG_VOLTAGE_B, &row), 0);
    assert_int_equal(row.protocol_id, 1);
    assert_int_equal(dcu_698_rules_find_by_proto(DCU_RULE_DLT645_2007, 0x02020100, &row), 0);
    assert_int_equal(row.oad, OAD_CURRENT);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, 0xDEADBEEF, &row), -1);
    assert_int_equal(dcu_698_rules_find_by_proto(DCU_RULE_DLT698, 0xDEADBEEF, &row), -1);
}

static void test_store_load_matches_old_tables(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);
    assert_int_equal(b->cache.count, EXPECTED_COUNT);

    for (size_t i = 0; i < EXPECTED_COUNT; i++) {
        const expected_rule_t *want = &k_expected[i];
        dcu_rule_row_t row;
        assert_int_equal(dcu_698_rules_find_by_oad(want->protocol, want->oad, &row), 0);
        _expect_row_matches(&row, want);
        assert_int_equal(dcu_698_rules_find_by_proto(want->protocol, want->protocol_id, &row), 0);
        assert_int_equal(row.oad, want->oad);
    }
    _bench_free(b);
}

static void test_cross_protocol_physical_equivalence(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);

    dcu_rule_row_t r645, r698;
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_CURRENT, &r645), 0);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT698, OAD_CURRENT, &r698), 0);
    double phys645 = 5000.0 * (double)r645.scale;
    double phys698 = 500.0 * (double)r698.scale;
    assert_true(phys645 > phys698 - 1e-6 && phys645 < phys698 + 1e-6);

    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_ACTIVE_POWER_TOTAL, &r645),
                     0);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT698, OAD_ACTIVE_POWER_TOTAL, &r698), 0);
    phys645 = 12345.0 * (double)r645.scale;
    phys698 = 12345.0 * (double)r698.scale;
    assert_true(phys645 > phys698 - 1e-6 && phys645 < phys698 + 1e-6);
    _bench_free(b);
}

static void test_adapter_reads_rules_source(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);

    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT645", OAD_VOLTAGE), 0x02010100);
    assert_int_equal(dcu_driver_adapter_proto_to_oad("DLT645", 0x02020100), OAD_CURRENT);
    assert_true(dcu_driver_adapter_scale("DLT645", 0x02020100) > 0.0009);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT698", OAD_CURRENT), OAD_CURRENT);
    uint32_t mb = dcu_driver_adapter_oad_to_proto("ModbusRTU", OAD_CURRENT);
    assert_int_equal(mb & 0xFFFFu, 6);
    assert_int_equal(mb >> 16, 2);
    assert_int_equal(dcu_driver_adapter_proto_to_oad("ModbusRTU", mb), OAD_CURRENT);
    _bench_free(b);
}

static void test_reopen_keeps_single_seed_set(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);
    assert_int_equal(b->cache.count, EXPECTED_COUNT);
    assert_int_equal(_bench_reopen(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);
    assert_int_equal(b->cache.count, EXPECTED_COUNT);
    dcu_rule_row_t row;
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_VOLTAGE, &row), 0);
    assert_int_equal(row.protocol_id, 0x02010100);
    _bench_free(b);
}


static void _op_rule(dcu_rule_op_t *op, uint8_t method) {
    memset(op, 0, sizeof(*op));
    op->method = method;
}

static void _op_unit(dcu_rule_op_t *op, uint32_t oad, uint32_t main_di) {
    dcu_rule_unit_t *u = &op->units[op->unit_count++];
    u->oad = oad;
    u->has_2007_main = 1;
    u->di_2007_main = main_di;
}

static void test_projection_only_standard_rows(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);
    uint16_t count = 0;
    assert_int_equal(dcu_698_rules_projected_count(&count), 0);
    assert_int_equal(count, 15);
    uint32_t oad = 0;
    assert_int_equal(dcu_698_rules_projected_oad(0, &oad), 0);
    assert_int_equal(oad, OAD_ACTIVE_ENERGY);
    assert_int_equal(dcu_698_rules_projected_oad(15, &oad), -1);
    _bench_free(b);
}

static void test_apply_refreshes_cache_and_keeps_internal(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);

    dcu_rule_op_t op;
    _op_rule(&op, 127);
    _op_unit(&op, OAD_VOLTAGE, 0x02010107);
    assert_int_equal(dcu_698_rules_apply(&op), 0);
    dcu_rule_row_t row;
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_VOLTAGE, &row), 0);
    assert_int_equal(row.protocol_id, 0x02010107);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT698, OAD_VOLTAGE, &row), 0);

    _op_rule(&op, 128);
    op.csd_count = 1;
    op.csds[0] = OAD_VOLTAGE;
    assert_int_equal(dcu_698_rules_apply(&op), 0);
    uint16_t count = 0;
    assert_int_equal(dcu_698_rules_projected_count(&count), 0);
    assert_int_equal(count, 14);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_VOLTAGE, &row), -1);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT698, OAD_VOLTAGE, &row), 0);
    _bench_free(b);
}

static void test_apply_persists_across_reopen(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);
    dcu_rule_op_t op;
    _op_rule(&op, 127);
    _op_unit(&op, OAD_VOLTAGE, 0x02010108);
    assert_int_equal(dcu_698_rules_apply(&op), 0);

    assert_int_equal(_bench_reopen(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);
    dcu_rule_row_t row;
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_VOLTAGE, &row), 0);
    assert_int_equal(row.protocol_id, 0x02010108);
    _bench_free(b);
}


static void test_apply_csd_maps_to_internal_tag(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);

    dcu_rule_op_t op;
    _op_rule(&op, 127);
    op.unit_count = 1;
    op.units[0].csd = OAD_VOLTAGE_B_ELEM;
    op.units[0].has_2007_main = 1;
    op.units[0].di_2007_main = 0x0201020B;
    assert_int_equal(dcu_698_rules_apply(&op), 0);

    dcu_rule_row_t row;
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, TAG_VOLTAGE_B, &row), 0);
    assert_int_equal(row.protocol_id, 0x0201020B);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_VOLTAGE_B_ELEM, &row),
                     -1);

    _op_rule(&op, 128);
    op.csd_count = 1;
    op.csds[0] = OAD_VOLTAGE_B_ELEM;
    assert_int_equal(dcu_698_rules_apply(&op), 0);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, TAG_VOLTAGE_B, &row), -1);
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT698, TAG_VOLTAGE_B, &row), 0);
    _bench_free(b);
}


static void test_retry_decision_one_shot(void **state) {
    (void)state;
    rules_bench_t *b = &g_bench;
    assert_int_equal(_bench_init(b), 0);
    assert_int_equal(dcu_698_rules_init(&b->storage, &b->cache), 0);

    uint32_t spare = 0;
    assert_int_equal(dcu_698_rules_retry_with_spare("DLT645", OAD_VOLTAGE, 0, &spare), 0);

    dcu_rule_op_t op;
    _op_rule(&op, 127);
    op.unit_count = 1;
    op.units[0].csd = OAD_VOLTAGE;
    op.units[0].has_2007_main = 1;
    op.units[0].di_2007_main = 0x02010100;
    op.units[0].has_2007_spare = 1;
    op.units[0].di_2007_spare = 0x02010101;
    assert_int_equal(dcu_698_rules_apply(&op), 0);

    assert_int_equal(dcu_698_rules_retry_with_spare("DLT645", OAD_VOLTAGE, 0, &spare), 1);
    assert_int_equal(spare, 0x02010101);
    assert_int_equal(dcu_698_rules_retry_with_spare("DLT645", OAD_VOLTAGE, 1, &spare), 0);
    assert_int_equal(dcu_698_rules_retry_with_spare("DLT698", OAD_VOLTAGE, 0, &spare), 0);
    assert_int_equal(dcu_698_rules_retry_with_spare("ModbusRTU", OAD_VOLTAGE, 0, &spare), 0);
    assert_int_equal(dcu_698_rules_retry_with_spare("DLT645", OAD_CURRENT, 0, &spare), 0);

    dcu_rule_row_t main_row, spare_row;
    assert_int_equal(dcu_698_rules_find_by_oad(DCU_RULE_DLT645_2007, OAD_VOLTAGE, &main_row), 0);
    assert_int_equal(dcu_698_rules_find_by_proto(DCU_RULE_DLT645_2007, 0x02010101, &spare_row), 0);
    assert_true(main_row.scale > spare_row.scale - 1e-6f && main_row.scale < spare_row.scale + 1e-6f);
    assert_int_equal(main_row.length, spare_row.length);

    int submits = 1;
    int retried = 0;
    if (dcu_698_rules_retry_with_spare("DLT645", OAD_VOLTAGE, retried, &spare)) {
        retried = 1;
        submits++;
    }
    if (dcu_698_rules_retry_with_spare("DLT645", OAD_VOLTAGE, retried, &spare)) submits++;
    assert_int_equal(submits, 2);
    _bench_free(b);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_seed_lookup_before_init),
        cmocka_unit_test(test_store_load_matches_old_tables),
        cmocka_unit_test(test_cross_protocol_physical_equivalence),
        cmocka_unit_test(test_adapter_reads_rules_source),
        cmocka_unit_test(test_reopen_keeps_single_seed_set),
        cmocka_unit_test(test_projection_only_standard_rows),
        cmocka_unit_test(test_apply_refreshes_cache_and_keeps_internal),
        cmocka_unit_test(test_apply_persists_across_reopen),
        cmocka_unit_test(test_apply_csd_maps_to_internal_tag),
        cmocka_unit_test(test_retry_decision_one_shot),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
