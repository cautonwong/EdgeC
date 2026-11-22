/**
 * @file test_dcu_north_source.c
 * @brief DCU 北向取数端口（ADR-0011 族 A / N2-3）
 *
 * 这个端口存在的理由：北向驱动只做 payload 编码，**不认识 698 OAD**。
 * 值从 datahub 影子取、名字由产品给——两条都必须走端口，不能让驱动自己反查
 * （那是 ADR-002 删掉的 `read_shadow` / `get_tag_name`）。
 *
 * 这里锁住的是"名字映射"这一半：映射错了，云端看到的字段名就是错的，
 * 而值本身是对的——这种错最难发现。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "dcu_698_var.h"
#include "dcu_north_source.h"
#include "dcu_router.h"

static void test_name_maps_realtime_oads(void** state) {
    (void)state;
    const edge_north_source_ops_t* ops = dcu_north_source_ops();
    assert_non_null(ops);
    assert_non_null(ops->name);
    assert_non_null(ops->peek);

    /* 三个相电压必须各自有名字——只给一个 VOLTAGE 会让云端三相互相覆盖 */
    assert_string_equal(ops->name(NULL, OAD_VOLTAGE), "VOLTAGE_A");
    assert_string_equal(ops->name(NULL, TAG_VOLTAGE_B), "VOLTAGE_B");
    assert_string_equal(ops->name(NULL, TAG_VOLTAGE_C), "VOLTAGE_C");

    assert_string_equal(ops->name(NULL, OAD_CURRENT), "CURRENT_A");
    assert_string_equal(ops->name(NULL, OAD_ACTIVE_POWER_TOTAL), "ACTIVE_POWER_TOTAL");
    assert_string_equal(ops->name(NULL, TAG_ACTIVE_POWER_A), "ACTIVE_POWER_A");
    assert_string_equal(ops->name(NULL, TAG_ACTIVE_POWER_B), "ACTIVE_POWER_B");
    assert_string_equal(ops->name(NULL, TAG_ACTIVE_POWER_C), "ACTIVE_POWER_C");
    assert_string_equal(ops->name(NULL, OAD_GRID_FREQUENCY), "GRID_FREQUENCY");
    assert_string_equal(ops->name(NULL, OAD_ACTIVE_ENERGY), "ACTIVE_ENERGY");
    assert_string_equal(ops->name(NULL, OAD_REACTIVE_ENERGY), "REACTIVE_ENERGY");
}

static void test_name_unknown_returns_null(void** state) {
    (void)state;
    const edge_north_source_ops_t* ops = dcu_north_source_ops();

    /* 档案/任务/控制类 OI 不走推送通道，没有 metric 名。
     * 返回 NULL 让驱动回落成数字标识，而不是编出一个假名字。 */
    assert_null(ops->name(NULL, (uint32_t)OI_METER_ARCHIVE << 16));
    assert_null(ops->name(NULL, (uint32_t)OI_REMOTE_CONTROL << 16));
    assert_null(ops->name(NULL, 0xDEADBEEF));
}

static const dcu_698_object_t* object_for_oi(uint16_t oi) {
    for (size_t i = 0; i < dcu_698_object_count(); i++) {
        const dcu_698_object_t* obj = dcu_698_object_at(i);
        if (obj->oi == oi) return obj;
    }
    return NULL;
}

/* 名字必须从变量点表派生；任何一处单独改表都会让这条测试变红。 */
static void test_names_derive_from_point_model(void** state) {
    (void)state;
    const edge_north_source_ops_t* ops = dcu_north_source_ops();
    uint32_t tags[64];
    size_t n = dcu_698_var_poll_tags(tags, sizeof(tags) / sizeof(tags[0]));
    assert_true(n > 0);

    for (size_t i = 0; i < n; i++) {
        const char* name = ops->name(NULL, tags[i]);
        assert_non_null(name);

        uint32_t back = 0;
        assert_int_equal(ops->resolve_name(NULL, name, &back), 0);
        assert_int_equal(back, tags[i]);

        const dcu_698_object_t* obj = object_for_oi((uint16_t)(tags[i] >> 16));
        assert_non_null(obj);
        assert_true((obj->attr_mask[0] & 0x04) != 0); /* 属性 2 已广告 */

        for (size_t j = 0; j < i; j++) assert_string_not_equal(name, ops->name(NULL, tags[j]));
    }
}

static void test_names_exclude_points_absent_from_object_table(void** state) {
    (void)state;
    const edge_north_source_ops_t* ops = dcu_north_source_ops();
    assert_null(ops->name(NULL, OAD_ZERO_SEQ_CURRENT));
    assert_null(ops->name(NULL, OAD_REACTIVE_POWER_TOTAL));
}

static void test_peek_rejects_null_ctx(void** state) {
    (void)state;
    const edge_north_source_ops_t* ops = dcu_north_source_ops();
    pipe_msg_t msg;

    /* 没给 datahub 就别假装能取到值 */
    assert_int_not_equal(ops->peek(NULL, OAD_VOLTAGE, &msg), 0);
    assert_int_not_equal(ops->peek(NULL, OAD_VOLTAGE, NULL), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_name_maps_realtime_oads),
        cmocka_unit_test(test_name_unknown_returns_null),
        cmocka_unit_test(test_names_derive_from_point_model),
        cmocka_unit_test(test_names_exclude_points_absent_from_object_table),
        cmocka_unit_test(test_peek_rejects_null_ctx),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
