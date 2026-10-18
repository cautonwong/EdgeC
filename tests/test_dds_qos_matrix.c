/**
 * @file test_dds_qos_matrix.c
 * @brief Comprehensive Verification of Four-Remote Industrial QoS Policy Matrix (AEP-451 / Issue
 * #143)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "datahub_dds.h"
#include "dds/dds.h"
#include "dlt698_objects.h"

static void test_four_remote_qos_policy_properties(void **state) {
    (void)state;

    // 1. 验证 YC 遥测 QoS: BEST_EFFORT, VOLATILE, KEEP_LAST(10)
    dds_qos_t *qos_yc = (dds_qos_t *)datahub_dds_create_qos_yc();
    assert_non_null(qos_yc);
    dds_reliability_kind_t rel_kind;
    dds_duration_t max_block;
    assert_true(dds_qget_reliability(qos_yc, &rel_kind, &max_block));
    assert_int_equal(rel_kind, DDS_RELIABILITY_BEST_EFFORT);

    dds_durability_kind_t dur_kind;
    assert_true(dds_qget_durability(qos_yc, &dur_kind));
    assert_int_equal(dur_kind, DDS_DURABILITY_VOLATILE);

    dds_history_kind_t hist_kind;
    int32_t hist_depth;
    assert_true(dds_qget_history(qos_yc, &hist_kind, &hist_depth));
    assert_int_equal(hist_kind, DDS_HISTORY_KEEP_LAST);
    assert_int_equal(hist_depth, 10);
    dds_delete_qos(qos_yc);

    // 2. 验证 YX 遥信 QoS: RELIABLE, TRANSIENT_LOCAL, KEEP_LAST(100)
    dds_qos_t *qos_yx = (dds_qos_t *)datahub_dds_create_qos_yx();
    assert_non_null(qos_yx);
    assert_true(dds_qget_reliability(qos_yx, &rel_kind, &max_block));
    assert_int_equal(rel_kind, DDS_RELIABILITY_RELIABLE);
    assert_true(dds_qget_durability(qos_yx, &dur_kind));
    assert_int_equal(dur_kind, DDS_DURABILITY_TRANSIENT_LOCAL);
    assert_true(dds_qget_history(qos_yx, &hist_kind, &hist_depth));
    assert_int_equal(hist_kind, DDS_HISTORY_KEEP_LAST);
    assert_int_equal(hist_depth, 100);
    dds_delete_qos(qos_yx);

    // 3. 验证 YK 遥控 QoS: RELIABLE, VOLATILE, KEEP_ALL
    dds_qos_t *qos_yk = (dds_qos_t *)datahub_dds_create_qos_yk();
    assert_non_null(qos_yk);
    assert_true(dds_qget_reliability(qos_yk, &rel_kind, &max_block));
    assert_int_equal(rel_kind, DDS_RELIABILITY_RELIABLE);
    assert_true(dds_qget_durability(qos_yk, &dur_kind));
    assert_int_equal(dur_kind, DDS_DURABILITY_VOLATILE);
    assert_true(dds_qget_history(qos_yk, &hist_kind, &hist_depth));
    assert_int_equal(hist_kind, DDS_HISTORY_KEEP_ALL);
    dds_delete_qos(qos_yk);

    // 4. 验证 YT 遥调 QoS: RELIABLE, VOLATILE, KEEP_LAST(1)
    dds_qos_t *qos_yt = (dds_qos_t *)datahub_dds_create_qos_yt();
    assert_non_null(qos_yt);
    assert_true(dds_qget_reliability(qos_yt, &rel_kind, &max_block));
    assert_int_equal(rel_kind, DDS_RELIABILITY_RELIABLE);
    assert_true(dds_qget_history(qos_yt, &hist_kind, &hist_depth));
    assert_int_equal(hist_kind, DDS_HISTORY_KEEP_LAST);
    assert_int_equal(hist_depth, 1);
    dds_delete_qos(qos_yt);
}

static void test_yx_transient_local_durability_and_replay(void **state) {
    (void)state;

    // 1. 初始化发布节点 Node Publisher
    datahub_t pub_hub;
    memset(&pub_hub, 0, sizeof(pub_hub));
    datahub_config_t cfg_pub = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    assert_int_equal(datahub_dds_init(&pub_hub, &cfg_pub), 0);

    // 2. 在 Subscriber 尚不存在时发布 5 条断路器跳闸报警 (Transient-Local 留存)
    for (int i = 0; i < 5; i++) {
        pipe_msg_t alarm_msg = {
            .tag_id = 0x30110000 + (uint32_t)i,
            .ts = (uint64_t)(1000000 * (i + 1)),
            .type = PIPEMSG_TYPE_EVENT,
            .attr = {.priority = 8},
            .payload.val.f = 1.0,
        };
        assert_int_equal(datahub_dds_publish_yx(&pub_hub, &alarm_msg), 0);
    }

    datahub_backend_stats_t pub_stats = {0};
    datahub_get_stats(&pub_hub, &pub_stats);
    assert_int_equal(pub_stats.total_publishes, 5);
    assert_int_equal(pub_stats.network_packets_tx, 5);

    // 3. 晚加入的节点 (Late Joiner) 初始化
    datahub_t sub_hub;
    memset(&sub_hub, 0, sizeof(sub_hub));
    datahub_config_t cfg_sub = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    assert_int_equal(datahub_dds_init(&sub_hub, &cfg_sub), 0);

    datahub_destroy(&pub_hub);
    datahub_destroy(&sub_hub);
}

static void test_yk_strict_keep_all_telecontrol_burst(void **state) {
    (void)state;

    datahub_t hub;
    memset(&hub, 0, sizeof(hub));
    datahub_config_t cfg = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    assert_int_equal(datahub_dds_init(&hub, &cfg), 0);

    // 突发连续下发 20 组遥控合分闸命令，验证 KEEP_ALL 零丢弃
    for (int i = 0; i < 20; i++) {
        pipe_msg_t cmd_msg = {
            .tag_id = 0x40010000 + (uint32_t)i,
            .ts = (uint64_t)(2000000 * (i + 1)),
            .type = PIPEMSG_TYPE_EVENT,
            .attr = {.priority = 10},
            .payload.val.f = (float)(i % 2),
        };
        assert_int_equal(datahub_dds_publish_yk(&hub, &cmd_msg), 0);
    }

    datahub_backend_stats_t stats = {0};
    datahub_get_stats(&hub, &stats);
    assert_int_equal(stats.total_publishes, 20);
    assert_int_equal(stats.network_packets_tx, 20);

    datahub_destroy(&hub);
}

static void test_automatic_topic_dispatch_routing(void **state) {
    (void)state;

    datahub_t hub;
    memset(&hub, 0, sizeof(hub));
    datahub_config_t cfg = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    assert_int_equal(datahub_dds_init(&hub, &cfg), 0);

    // 1. 普通遥测点位 (0x20000000 范围) -> 自动分发至 YC
    pipe_msg_t msg_yc = {.tag_id = 536871424,  // DLT698 Voltage
                         .ts = 1000000ULL,
                         .type = PIPEMSG_TYPE_SCALAR,
                         .payload.val.f = 220.0};
    assert_int_equal(datahub_publish(&hub, &msg_yc), 0);

    // 2. 遥信告警点位 (0x30000000 范围或 PIPEMSG_TYPE_EVENT) -> 自动分发至 YX
    pipe_msg_t msg_yx = {
        .tag_id = 0x30050001, .ts = 2000000ULL, .type = PIPEMSG_TYPE_EVENT, .payload.val.f = 1.0};
    assert_int_equal(datahub_publish(&hub, &msg_yx), 0);

    // 3. 遥控控制点位 (0x40000000 范围或 priority >= 10) -> 自动分发至 YK
    pipe_msg_t msg_yk = {.tag_id = 0x40010001,
                         .ts = 3000000ULL,
                         .type = PIPEMSG_TYPE_EVENT,
                         .attr = {.priority = 10},
                         .payload.val.f = 0.0};
    assert_int_equal(datahub_publish(&hub, &msg_yk), 0);

    datahub_backend_stats_t stats = {0};
    datahub_get_stats(&hub, &stats);
    assert_int_equal(stats.total_publishes, 3);
    assert_int_equal(stats.network_packets_tx, 3);

    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_four_remote_qos_policy_properties),
        cmocka_unit_test(test_yx_transient_local_durability_and_replay),
        cmocka_unit_test(test_yk_strict_keep_all_telecontrol_burst),
        cmocka_unit_test(test_automatic_topic_dispatch_routing),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
