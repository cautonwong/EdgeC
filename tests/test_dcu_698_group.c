#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_group.h"
#include "dlt698_codec.h"

typedef struct {
    int64_t p[32];
    int64_t q[32];
    int64_t pae[32];
    int64_t nae[32];
    int64_t pre[32];
    int64_t nre[32];
    bool fail[32];
} mock_meter_bank_t;

static int _mock_read(void *ctx, const dcu_group_member_t *member,
                      dcu_group_member_reading_t *out) {
    const mock_meter_bank_t *bank = (const mock_meter_bank_t *)ctx;
    uint32_t idx = member->slot_id;
    if (idx >= 32 || bank->fail[idx]) return -1;
    out->active_power = bank->p[idx];
    out->reactive_power = bank->q[idx];
    out->pos_active_energy = bank->pae[idx];
    out->neg_active_energy = bank->nae[idx];
    out->pos_reactive_energy = bank->pre[idx];
    out->neg_reactive_energy = bank->nre[idx];
    return 0;
}

static int64_t _decode_be64(const uint8_t *p) {
    uint64_t u = 0;
    for (int i = 0; i < 8; i++) {
        u = (u << 8) | p[i];
    }
    return (int64_t)u;
}

static void test_member_crud_and_bounds(void **state) {
    (void)state;
    dcu_698_group_init();

    assert_null(dcu_698_group_get(DCU_GROUP_COUNT));
    assert_int_equal(dcu_698_group_clear(DCU_GROUP_COUNT), -1);

    dcu_group_member_t m;
    memset(&m, 0, sizeof(m));
    m.slot_id = 1;
    m.add_sub_flag = 0;
    m.calc_flag = 1;
    assert_int_equal(dcu_698_group_add_member(DCU_GROUP_COUNT, &m), -1);
    assert_int_equal(dcu_698_group_add_member(0, NULL), -1);

    m.add_sub_flag = 2;
    assert_int_equal(dcu_698_group_add_member(0, &m), -1);
    m.add_sub_flag = 0;
    m.calc_flag = 2;
    assert_int_equal(dcu_698_group_add_member(0, &m), -1);
    m.calc_flag = 1;
    m.tsa_len = 1;
    assert_int_equal(dcu_698_group_add_member(0, &m), -1);
    m.tsa_len = 0;

    /* 满 16 个成员后第 17 个返回 -1 */
    for (uint32_t i = 0; i < DCU_GROUP_MAX_MEMBERS; i++) {
        m.slot_id = i + 1;
        assert_int_equal(dcu_698_group_add_member(0, &m), 0);
    }
    const dcu_group_state_t *st = dcu_698_group_get(0);
    assert_non_null(st);
    assert_int_equal(st->member_count, DCU_GROUP_MAX_MEMBERS);

    m.slot_id = 99;
    assert_int_equal(dcu_698_group_add_member(0, &m), -1);

    /* 同 slot_id 更新覆盖不增加数量 */
    m.slot_id = 5;
    m.add_sub_flag = 1;
    assert_int_equal(dcu_698_group_add_member(0, &m), 0);
    assert_int_equal(st->member_count, DCU_GROUP_MAX_MEMBERS);
    assert_int_equal(st->members[4].add_sub_flag, 1);

    /* 删除存在的成员、再删不存在的返回 -1 */
    assert_int_equal(dcu_698_group_delete_member(0, 5, NULL, 0), 0);
    assert_int_equal(st->member_count, DCU_GROUP_MAX_MEMBERS - 1);
    assert_int_equal(dcu_698_group_delete_member(0, 5, NULL, 0), -1);

    /* TSA 匹配添加与删除 */
    dcu_group_member_t tsa_m;
    memset(&tsa_m, 0, sizeof(tsa_m));
    tsa_m.slot_id = 20;
    tsa_m.tsa_len = 7;
    tsa_m.tsa[0] = 0x05;
    tsa_m.tsa[6] = 0x77;
    tsa_m.calc_flag = 1;
    assert_int_equal(dcu_698_group_add_member(0, &tsa_m), 0);
    assert_int_equal(st->member_count, DCU_GROUP_MAX_MEMBERS);
    assert_int_equal(dcu_698_group_delete_member(0, 0, tsa_m.tsa, tsa_m.tsa_len), 0);
    assert_int_equal(st->member_count, DCU_GROUP_MAX_MEMBERS - 1);

    assert_int_equal(dcu_698_group_clear(0), 0);
    assert_int_equal(st->member_count, 0);
}

static void test_add_sub_and_calc_flag_aggregation(void **state) {
    (void)state;
    dcu_698_group_init();

    mock_meter_bank_t bank;
    memset(&bank, 0, sizeof(bank));
    bank.p[1] = 1000;
    bank.q[1] = 200;
    bank.pae[1] = 50000;
    bank.nae[1] = 1000;
    bank.pre[1] = 8000;
    bank.nre[1] = 500;

    bank.p[2] = 300;
    bank.q[2] = 50;
    bank.pae[2] = 12000;
    bank.nae[2] = 200;
    bank.pre[2] = 2000;
    bank.nre[2] = 100;

    bank.p[3] = 500;
    bank.q[3] = 999;
    bank.pae[3] = 99999;

    dcu_group_member_t ma = {.slot_id = 1, .add_sub_flag = 0, .calc_flag = 1};
    dcu_group_member_t mb = {.slot_id = 2, .add_sub_flag = 1, .calc_flag = 1};
    dcu_group_member_t mc = {.slot_id = 3, .add_sub_flag = 0, .calc_flag = 0};

    assert_int_equal(dcu_698_group_add_member(1, &ma), 0);
    assert_int_equal(dcu_698_group_add_member(1, &mb), 0);
    assert_int_equal(dcu_698_group_add_member(1, &mc), 0);

    assert_int_equal(dcu_698_group_sample(1, _mock_read, &bank, 100), 0);

    const dcu_group_state_t *st = dcu_698_group_get(1);
    assert_non_null(st);
    assert_true(st->has_sample);
    assert_int_equal(st->active_power, 700);
    assert_int_equal(st->reactive_power, 150);
    assert_int_equal(st->pos_active_energy, 38000);
    assert_int_equal(st->neg_active_energy, 800);
    assert_int_equal(st->pos_reactive_energy, 6000);
    assert_int_equal(st->neg_reactive_energy, 400);
    assert_int_equal(st->slip_active_power, 700);
    assert_int_equal(st->slip_reactive_power, 150);
}

static void test_no_valid_sample_does_not_forge_or_divide_by_zero(void **state) {
    (void)state;
    dcu_698_group_init();

    mock_meter_bank_t bank;
    memset(&bank, 0, sizeof(bank));
    bank.p[1] = 500;

    /* 空组采样失败 */
    assert_int_equal(dcu_698_group_sample(2, _mock_read, &bank, 10), -1);
    assert_int_equal(dcu_698_group_sample(DCU_GROUP_COUNT, _mock_read, &bank, 10), -1);
    assert_int_equal(dcu_698_group_sample(2, NULL, &bank, 10), -1);

    /* 仅存在 calc_flag=0 成员时采样失败，不写入滑差 */
    dcu_group_member_t mc = {.slot_id = 1, .add_sub_flag = 0, .calc_flag = 0};
    assert_int_equal(dcu_698_group_add_member(2, &mc), 0);
    assert_int_equal(dcu_698_group_sample(2, _mock_read, &bank, 20), -1);

    const dcu_group_state_t *st = dcu_698_group_get(2);
    assert_false(st->has_sample);
    assert_int_equal(st->slip_count, 0);

    uint8_t buf[32];
    size_t out_len = 0;
    for (uint8_t attr = 3; attr <= 10; attr++) {
        assert_int_equal(dcu_698_group_encode_attr(2, attr, buf, sizeof(buf), &out_len), -1);
    }
}

static void test_slip_circular_window_eviction(void **state) {
    (void)state;
    dcu_698_group_init();

    mock_meter_bank_t bank;
    memset(&bank, 0, sizeof(bank));
    dcu_group_member_t ma = {.slot_id = 1, .add_sub_flag = 0, .calc_flag = 1};
    assert_int_equal(dcu_698_group_add_member(3, &ma), 0);

    /* 连续注入 20 次样本：100, 200, ..., 2000 */
    for (int i = 1; i <= 20; i++) {
        bank.p[1] = (int64_t)i * 100;
        bank.q[1] = (int64_t)i * 10;
        assert_int_equal(dcu_698_group_sample(3, _mock_read, &bank, (uint64_t)i * 60u), 0);
    }

    const dcu_group_state_t *st = dcu_698_group_get(3);
    assert_int_equal(st->slip_count, DCU_GROUP_SLIP_WINDOW);
    /* 最近 15 个样本为 600..2000，平均为 1300 */
    assert_int_equal(st->slip_active_power, 1300);
    assert_int_equal(st->slip_reactive_power, 130);
}

static void test_axdr_encoding_a2_to_a10_and_cap_bounds(void **state) {
    (void)state;
    dcu_698_group_init();

    /* 空组 A2 编码为 array(0): 01 00 */
    uint8_t buf[128];
    size_t len = 0;
    assert_int_equal(dcu_698_group_encode_attr(4, 2, buf, sizeof(buf), &len), 0);
    assert_int_equal(len, 2);
    assert_int_equal(buf[0], DLT698_TAG_ARRAY);
    assert_int_equal(buf[1], 0);
    assert_int_equal(dcu_698_group_encode_attr(4, 2, buf, 1, &len), -1);

    /* 添加 1 个带 TSA 成员并采样（含负功率） */
    dcu_group_member_t m;
    memset(&m, 0, sizeof(m));
    m.slot_id = 4;
    m.tsa_len = 7;
    m.tsa[0] = 0x05;
    m.tsa[1] = 0x00;
    m.tsa[2] = 0x00;
    m.tsa[3] = 0x00;
    m.tsa[4] = 0x00;
    m.tsa[5] = 0x00;
    m.tsa[6] = 0x04;
    m.add_sub_flag = 1;
    m.calc_flag = 1;
    assert_int_equal(dcu_698_group_add_member(4, &m), 0);

    mock_meter_bank_t bank;
    memset(&bank, 0, sizeof(bank));
    bank.p[4] = 1234;
    bank.q[4] = 56;
    bank.pae[4] = 7890;
    bank.nae[4] = 111;
    bank.pre[4] = 222;
    bank.nre[4] = 333;
    assert_int_equal(dcu_698_group_sample(4, _mock_read, &bank, 600), 0);

    /* A2: 01 01 02 03 55 07 <7B TSA> 16 01 16 01 */
    assert_int_equal(dcu_698_group_encode_attr(4, 2, buf, sizeof(buf), &len), 0);
    assert_int_equal(len, 17);
    assert_int_equal(buf[0], DLT698_TAG_ARRAY);
    assert_int_equal(buf[1], 1);
    assert_int_equal(buf[2], DLT698_TAG_STRUCTURE);
    assert_int_equal(buf[3], 3);
    assert_int_equal(buf[4], 85);
    assert_int_equal(buf[5], 7);
    assert_memory_equal(&buf[6], m.tsa, 7);
    assert_int_equal(buf[13], DLT698_TAG_ENUM);
    assert_int_equal(buf[14], 1);
    assert_int_equal(buf[15], DLT698_TAG_ENUM);
    assert_int_equal(buf[16], 1);
    assert_int_equal(dcu_698_group_encode_attr(4, 2, buf, 16, &len), -1);

    /* A3..A6: long64 (9 字节) */
    assert_int_equal(dcu_698_group_encode_attr(4, 3, buf, sizeof(buf), &len), 0);
    assert_int_equal(len, 9);
    assert_int_equal(buf[0], DLT698_TAG_LONG64);
    assert_true(_decode_be64(&buf[1]) == -1234);
    assert_int_equal(dcu_698_group_encode_attr(4, 3, buf, 8, &len), -1);

    assert_int_equal(dcu_698_group_encode_attr(4, 5, buf, sizeof(buf), &len), 0);
    assert_int_equal(len, 9);
    assert_true(_decode_be64(&buf[1]) == -1234);

    /* A7..A10: array(1) of long64 (11 字节) */
    assert_int_equal(dcu_698_group_encode_attr(4, 7, buf, sizeof(buf), &len), 0);
    assert_int_equal(len, 11);
    assert_int_equal(buf[0], DLT698_TAG_ARRAY);
    assert_int_equal(buf[1], 1);
    assert_int_equal(buf[2], DLT698_TAG_LONG64);
    assert_true(_decode_be64(&buf[3]) == -7890);
    assert_int_equal(dcu_698_group_encode_attr(4, 7, buf, 10, &len), -1);

    /* 非法属性与越界组 */
    assert_int_equal(dcu_698_group_encode_attr(4, 1, buf, sizeof(buf), &len), -1);
    assert_int_equal(dcu_698_group_encode_attr(4, 11, buf, sizeof(buf), &len), -1);
    assert_int_equal(dcu_698_group_encode_attr(DCU_GROUP_COUNT, 3, buf, sizeof(buf), &len), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_member_crud_and_bounds),
        cmocka_unit_test(test_add_sub_and_calc_flag_aggregation),
        cmocka_unit_test(test_no_valid_sample_does_not_forge_or_divide_by_zero),
        cmocka_unit_test(test_slip_circular_window_eviction),
        cmocka_unit_test(test_axdr_encoding_a2_to_a10_and_cap_bounds),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
