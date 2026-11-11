/**
 * @file test_northbound_address.c
 * @brief 北向地址模板渲染（ADR-0011 / N4-1）
 *
 * ## 为什么要专门测这个
 *
 * 地址模板写错属于"看起来正常但不对"：主题拼出来了，publish 也返回成功，
 * 但主站那边收不到（订阅的是另一个主题）。没有报错、没有日志、没有异常，
 * 只有"云端一直没数据"。
 *
 * 所以这里逐条锁住：默认模板、全部占位符、未知占位符、缺身份、缓冲区不够。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "northbound_mgr.h"
#include "north_driver_test_port.h"

static void test_default_template(void** state) {
    (void)state;
    northbound_mgr_t mgr;
    memset(&mgr, 0, sizeof(mgr));

    /* 不给模板 = 用 Sparkplug 缺省；group/node 取自环境变量，这里显式设 */
    northbound_mgr_set_address(&mgr, NULL, "EdgeOS", "DCU_Node_A");

    char out[128];
    assert_int_equal(northbound_mgr_render_address(&mgr, "NDATA", "VOLTAGE_A", out, sizeof(out)), 0);
    assert_string_equal(out, "spBv1.0/EdgeOS/NDATA/DCU_Node_A");

    /* NBIRTH 不带点位，主题形态一致 */
    assert_int_equal(northbound_mgr_render_address(&mgr, "NBIRTH", NULL, out, sizeof(out)), 0);
    assert_string_equal(out, "spBv1.0/EdgeOS/NBIRTH/DCU_Node_A");
}

static void test_custom_template_all_placeholders(void** state) {
    (void)state;
    northbound_mgr_t mgr;
    memset(&mgr, 0, sizeof(mgr));

    northbound_mgr_set_address(&mgr, "edge/{node}/{point}", "G", "N1");

    char out[128];
    assert_int_equal(northbound_mgr_render_address(&mgr, "NDATA", "SOC", out, sizeof(out)), 0);
    assert_string_equal(out, "edge/N1/SOC");
}

static void test_unknown_placeholder_is_preserved(void** state) {
    (void)state;
    northbound_mgr_t mgr;
    memset(&mgr, 0, sizeof(mgr));

    /* 拼错占位符必须原样保留 —— 让主站看到一个显眼的 {ponit}，
     * 而不是静默渲染成空串拼出一个"看起来正常"的错误主题。 */
    northbound_mgr_set_address(&mgr, "edge/{node}/{ponit}", NULL, "N1");

    char out[128];
    assert_int_equal(northbound_mgr_render_address(&mgr, "NDATA", "SOC", out, sizeof(out)), 0);
    assert_string_equal(out, "edge/N1/{ponit}");
}

static void test_missing_identity_falls_back(void** state) {
    (void)state;
    northbound_mgr_t mgr;
    memset(&mgr, 0, sizeof(mgr));

    /* 身份没配不能渲染成 "//" —— 那是合法但无意义的主题 */
    northbound_mgr_set_address(&mgr, "spBv1.0/{group}/{kind}/{node}", "", "");

    char out[128];
    assert_int_equal(northbound_mgr_render_address(&mgr, "NDATA", "V", out, sizeof(out)), 0);
    assert_string_equal(out, "spBv1.0/unknown/NDATA/unknown");
}

static void test_small_buffer_does_not_overflow(void** state) {
    (void)state;
    northbound_mgr_t mgr;
    memset(&mgr, 0, sizeof(mgr));

    northbound_mgr_set_address(&mgr, "spBv1.0/{group}/{kind}/{node}", "EdgeOS", "Node_A");

    char out[16];
    memset(out, 'X', sizeof(out));
    assert_int_equal(northbound_mgr_render_address(&mgr, "NDATA", "VOLTAGE_A", out, sizeof(out)), 0);

    /* 必须 NUL 结尾且不越界 */
    assert_int_equal(out[sizeof(out) - 1], '\0');
    assert_true(strlen(out) <= sizeof(out) - 1);
}

static void test_rejects_bad_args(void** state) {
    (void)state;
    northbound_mgr_t mgr;
    memset(&mgr, 0, sizeof(mgr));
    char out[32];

    assert_int_not_equal(northbound_mgr_render_address(NULL, "NDATA", "V", out, sizeof(out)), 0);
    assert_int_not_equal(northbound_mgr_render_address(&mgr, "NDATA", "V", NULL, sizeof(out)), 0);
    assert_int_not_equal(northbound_mgr_render_address(&mgr, "NDATA", "V", out, 0), 0);
}

static void test_protocol_selection(void** state) {
    (void)state;
    northbound_mgr_t mgr;
    memset(&mgr, 0, sizeof(mgr));

    /* 注册表里必须真有第二个协议，否则"可插拔"是没验证过的说法 */
    assert_int_equal(northbound_mgr_set_protocol(&mgr, "SparkplugB"), 0);
    assert_int_equal(northbound_mgr_set_protocol(&mgr, "MqttJson"), 0);
    assert_string_equal(mgr.protocol_name, "MqttJson");

    /* 未注册的名字必须被拒，而不是默默用缺省 */
    assert_int_not_equal(northbound_mgr_set_protocol(&mgr, "NoSuchProtocol"), 0);
    assert_string_equal(mgr.protocol_name, "MqttJson"); /* 失败不该改掉已选的 */
    assert_int_not_equal(northbound_mgr_set_protocol(&mgr, ""), 0);
    assert_int_not_equal(northbound_mgr_set_protocol(NULL, "MqttJson"), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_default_template),
        cmocka_unit_test(test_custom_template_all_placeholders),
        cmocka_unit_test(test_unknown_placeholder_is_preserved),
        cmocka_unit_test(test_missing_identity_falls_back),
        cmocka_unit_test(test_small_buffer_does_not_overflow),
        cmocka_unit_test(test_rejects_bad_args),
        cmocka_unit_test(test_protocol_selection),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
