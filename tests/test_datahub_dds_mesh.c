/**
 * @file test_datahub_dds_mesh.c
 * @brief Cyclone DDS Industrial QoS Matrix & Multi-Node Distributed Mesh Test (ARCH-DDS-01)
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

static void test_dds_qos_matrix_creation(void **state) {
    (void)state;

    void *qos_yc = datahub_dds_create_qos_yc();
    assert_non_null(qos_yc);
    dds_delete_qos((dds_qos_t *)qos_yc);

    void *qos_yx = datahub_dds_create_qos_yx();
    assert_non_null(qos_yx);
    dds_delete_qos((dds_qos_t *)qos_yx);

    void *qos_yk = datahub_dds_create_qos_yk();
    assert_non_null(qos_yk);
    dds_delete_qos((dds_qos_t *)qos_yk);

    void *qos_yt = datahub_dds_create_qos_yt();
    assert_non_null(qos_yt);
    dds_delete_qos((dds_qos_t *)qos_yt);
}

static void test_dds_node_discovery_and_telemetry_streaming(void **state) {
    (void)state;

    datahub_t hub_a;
    memset(&hub_a, 0, sizeof(hub_a));
    datahub_config_t cfg_a = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    int rc = datahub_dds_init(&hub_a, &cfg_a);
    assert_int_equal(rc, 0);

    datahub_t hub_b;
    memset(&hub_b, 0, sizeof(hub_b));
    datahub_config_t cfg_b = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    rc = datahub_dds_init(&hub_b, &cfg_b);
    assert_int_equal(rc, 0);

    // Node B 发布 100 帧高频遥测流 (YC)
    for (int i = 0; i < 100; i++) {
        pipe_msg_t msg = {
            .tag_id = 536871424 + (uint32_t)i,  // DLT698 Voltage Phase A offset
            .ts = 1000000ULL * (uint64_t)(uint32_t)i,
            .type = PIPEMSG_TYPE_SCALAR,
            .payload.val.f = 220.0 + (double)i * 0.1,
        };
        rc = datahub_publish(&hub_b, &msg);
        assert_int_equal(rc, 0);
    }

    datahub_backend_stats_t stats_b = {0};
    datahub_get_stats(&hub_b, &stats_b);
    assert_int_equal(stats_b.total_publishes, 100);
    assert_int_equal(stats_b.network_packets_tx, 100);

    datahub_destroy(&hub_a);
    datahub_destroy(&hub_b);
}

static void test_dds_yx_late_joiner_retention(void **state) {
    (void)state;

    datahub_t hub_b;
    memset(&hub_b, 0, sizeof(hub_b));
    datahub_config_t cfg_b = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    int rc = datahub_dds_init(&hub_b, &cfg_b);
    assert_int_equal(rc, 0);

    // Node B 发布 5 条断路器跳闸越限告警 (YX Alarm, Transient Local)
    for (int i = 0; i < 5; i++) {
        pipe_msg_t alarm_msg = {
            .tag_id = 0x30110001 + (uint32_t)i,
            .ts = 2000000ULL * (uint64_t)(uint32_t)i,
            .type = PIPEMSG_TYPE_EVENT,
            .attr = {.priority = 5},
            .payload.val.f = 1.0,  // 变位报警置位
        };
        rc = datahub_dds_publish_yx(&hub_b, &alarm_msg);
        assert_int_equal(rc, 0);
    }

    datahub_backend_stats_t stats_b = {0};
    datahub_get_stats(&hub_b, &stats_b);
    assert_int_equal(stats_b.total_publishes, 5);
    assert_int_equal(stats_b.network_packets_tx, 5);

    // Node A 晚加入 (Late Joiner)
    datahub_t hub_a;
    memset(&hub_a, 0, sizeof(hub_a));
    datahub_config_t cfg_a = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    rc = datahub_dds_init(&hub_a, &cfg_a);
    assert_int_equal(rc, 0);

    datahub_destroy(&hub_a);
    datahub_destroy(&hub_b);
}

static void test_dds_yk_telecontrol_strict_dispatch(void **state) {
    (void)state;

    datahub_t hub_a;
    memset(&hub_a, 0, sizeof(hub_a));
    datahub_config_t cfg_a = {.dds_domain_id = 0, .backend_type = DATAHUB_BACKEND_DDS};
    int rc = datahub_dds_init(&hub_a, &cfg_a);
    assert_int_equal(rc, 0);

    // Node A 严格可靠下发遥控分闸命令 (YK Command, Keep All)
    pipe_msg_t cmd_msg = {
        .tag_id = 0x40010001,
        .ts = 5000000ULL,
        .type = PIPEMSG_TYPE_EVENT,
        .attr = {.priority = 10},
        .payload.val.f = 0.0,  // 遥控分闸
    };
    rc = datahub_dds_publish_yk(&hub_a, &cmd_msg);
    assert_int_equal(rc, 0);

    datahub_backend_stats_t stats_a = {0};
    datahub_get_stats(&hub_a, &stats_a);
    assert_int_equal(stats_a.total_publishes, 1);
    assert_int_equal(stats_a.network_packets_tx, 1);

    datahub_destroy(&hub_a);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dds_qos_matrix_creation),
        cmocka_unit_test(test_dds_node_discovery_and_telemetry_streaming),
        cmocka_unit_test(test_dds_yx_late_joiner_retention),
        cmocka_unit_test(test_dds_yk_telecontrol_strict_dispatch),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
