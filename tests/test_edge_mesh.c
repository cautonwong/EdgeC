#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "datahub.h"
#include "edge_mesh.h"

static void test_mesh_delta_and_merkle_reconcile(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    datahub_t hub_a, hub_b;
    int ret = datahub_init(&hub_a, loop, "mesh_bus_a", 32, 3000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    ret = datahub_init(&hub_b, loop, "mesh_bus_b", 32, 3000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    edge_mesh_t mesh_a, mesh_b;
    ret = edge_mesh_init(&mesh_a, 101, &hub_a);
    assert_int_equal(ret, 0);

    ret = edge_mesh_init(&mesh_b, 102, &hub_b);
    assert_int_equal(ret, 0);

    // Node A 打包 Delta 增量
    mesh_delta_pkt_t pkt;
    ret = edge_mesh_pack_delta(&mesh_a, 2001, 380.5, 1000000, &pkt);
    assert_int_equal(ret, 0);
    assert_int_equal(pkt.node_id, 101);
    assert_int_equal(pkt.tag_id, 2001);

    // Node B 处理来自 A 的 Delta
    ret = edge_mesh_process_remote_delta(&mesh_b, &pkt);
    assert_int_equal(ret, 0);

    // 验证 Node B 影子点位物理对齐
    pipe_msg_t msg_b;
    int idx = datahub_read_shadow(&hub_b, 2001, &msg_b);
    assert_true(idx >= 0);
    assert_double_equal(msg_b.payload.val.f, 380.5, 0.001);

    // 验证两节点 Merkle Root 完全一致
    pipe_msg_t msg_a = {.tag_id = 2001,
                        .ts = 1000000,
                        .type = PIPEMSG_TYPE_SCALAR,
                        .payload = {.val = {.f = 380.5}}};
    datahub_write_shadow(&hub_a, &msg_a);

    uint64_t root_a = edge_mesh_compute_merkle_root(&mesh_a);
    uint64_t root_b = edge_mesh_compute_merkle_root(&mesh_b);
    assert_int_equal(root_a, root_b);

    ret = edge_mesh_reconcile(&mesh_a, root_b);
    assert_int_equal(ret, 0);  // 0 代表完全匹配

    datahub_destroy(&hub_a);
    datahub_destroy(&hub_b);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mesh_delta_and_merkle_reconcile),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
