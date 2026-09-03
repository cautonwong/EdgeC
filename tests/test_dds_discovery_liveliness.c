/**
 * @file test_dds_discovery_liveliness.c
 * @brief Empirical Verification for Cyclone DDS Topology Discovery & Liveliness Self-Healing (AEP-452)
 */

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>

#include "datahub.h"
#include "dds_topology_monitor.h"

static void test_multi_node_dynamic_discovery(void **state) {
    (void)state;

    dds_topology_monitor_t mon;
    int rc = dds_topology_monitor_init(&mon, NULL, 2000);
    assert_int_equal(rc, 0);

    // 并发注册 10 个分布式现场节点 (PCS, BMS, DCU, SmartBox)
    for (uint32_t i = 1; i <= 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "EdgeNode_%u", i);
        rc = dds_topology_monitor_register_node(&mon, i, name, "Bay/Main", 0x10000000 + i * 10, 0x10000000 + i * 10 + 5);
        assert_int_equal(rc, 0);
    }

    assert_int_equal(mon.node_count, 10);
    assert_int_equal(mon.online_nodes_count, 10);

    for (uint32_t i = 1; i <= 10; i++) {
        assert_true(dds_topology_monitor_is_node_online(&mon, i));
    }

    dds_topology_monitor_destroy(&mon);
}

static void test_liveliness_heartbeat_and_lost_dropout(void **state) {
    (void)state;

    dds_topology_monitor_t mon;
    int rc = dds_topology_monitor_init(&mon, NULL, 2000); // 2000ms 租约
    assert_int_equal(rc, 0);

    rc = dds_topology_monitor_register_node(&mon, 1, "PCS_Cluster_1", "Bay/Line1", 0x20010000, 0x20010005);
    assert_int_equal(rc, 0);
    rc = dds_topology_monitor_register_node(&mon, 2, "BMS_Cluster_1", "Bay/Line1", 0x20020000, 0x20020005);
    assert_int_equal(rc, 0);

    // t = 1000ms: 节点 1 刷新心跳, 节点 2 未刷新
    dds_topology_monitor_heartbeat(&mon, 1, 1000);

    // t = 2500ms: 检测失活 (租约 2000ms)
    // 节点 1: 2500 - 1000 = 1500 <= 2000 (在线)
    // 节点 2: 2500 - 0 = 2500 > 2000 (失活离线)
    rc = dds_topology_monitor_check_liveliness(&mon, 2500);
    assert_int_equal(rc, 0);

    assert_true(dds_topology_monitor_is_node_online(&mon, 1));
    assert_false(dds_topology_monitor_is_node_online(&mon, 2));
    assert_int_equal(mon.online_nodes_count, 1);
    assert_int_equal(mon.total_liveliness_lost_events, 1);

    // t = 3000ms: 节点 2 恢复重连上报心跳 -> 自动自愈重置为在线
    rc = dds_topology_monitor_heartbeat(&mon, 2, 3000);
    assert_int_equal(rc, 0);
    assert_true(dds_topology_monitor_is_node_online(&mon, 2));
    assert_int_equal(mon.online_nodes_count, 2);

    dds_topology_monitor_destroy(&mon);
}

static void test_datahub_shadow_automatic_invalidation(void **state) {
    (void)state;

    datahub_t hub;
    int rc = datahub_init(&hub, NULL, "dds_liveliness_hub", 64, 0x40000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    dds_topology_monitor_t mon;
    rc = dds_topology_monitor_init(&mon, &hub, 1000); // 1000ms 租约
    assert_int_equal(rc, 0);

    uint32_t target_tag = 1001;
    rc = dds_topology_monitor_register_node(&mon, 10, "Inverter_Central", "Bay/PV", target_tag, target_tag);
    assert_int_equal(rc, 0);

    // 节点写入正常遥测数据 (Quality = 0x00 VALID)
    pipe_msg_t valid_msg = {
        .tag_id = target_tag,
        .ts = 100000,
        .type = PIPEMSG_TYPE_SCALAR,
        .quality = 0x00,
        .payload.val.f = 220.0
    };
    datahub_write_shadow(&hub, &valid_msg);

    pipe_msg_t check_msg = {0};
    rc = datahub_read_shadow(&hub, target_tag, &check_msg);
    assert_int_equal(rc, 0);
    assert_int_equal(check_msg.quality, 0x00);

    // t = 1500ms: 节点未发送心跳导致 Liveliness Lost -> DataHub 点位自动打标为 QUALITY_INVALID (0x02)
    rc = dds_topology_monitor_check_liveliness(&mon, 1500);
    assert_int_equal(rc, 0);
    assert_false(dds_topology_monitor_is_node_online(&mon, 10));

    rc = datahub_read_shadow(&hub, target_tag, &check_msg);
    assert_int_equal(rc, 0);
    assert_int_equal(check_msg.quality, 0x02); // 0x02 为 QUALITY_INVALID

    dds_topology_monitor_destroy(&mon);
    datahub_destroy(&hub);
}

static void test_partition_qos_isolation(void **state) {
    (void)state;

    // 1. 精确同分区匹配
    assert_true(dds_topology_monitor_match_partition("Bay/Line1", "Bay/Line1"));

    // 2. 跨分区隔离拦截
    assert_false(dds_topology_monitor_match_partition("Bay/Line1", "Bay/Line2"));

    // 3. 通配符广播匹配
    assert_true(dds_topology_monitor_match_partition("*", "Bay/Line1"));
    assert_true(dds_topology_monitor_match_partition("Bay/Line2", "*"));

    // 4. 默认缺省分区匹配
    assert_true(dds_topology_monitor_match_partition("", ""));
    assert_true(dds_topology_monitor_match_partition(NULL, "Bay/Line1"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_multi_node_dynamic_discovery),
        cmocka_unit_test(test_liveliness_heartbeat_and_lost_dropout),
        cmocka_unit_test(test_datahub_shadow_automatic_invalidation),
        cmocka_unit_test(test_partition_qos_isolation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
