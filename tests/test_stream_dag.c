#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmocka.h>
#include "stream_engine.h"

static int g_stage1_run = 0;
static int g_stage2_run = 0;

static stage_ret_t mock_stage1_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)stage;
    (void)ev;
    g_stage1_run++;
    return STAGE_OK;
}

static stage_ret_t mock_stage2_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)stage;
    (void)ev;
    g_stage2_run++;
    return STAGE_OK;
}

static void test_dag_acyclic_and_execution(void **state) {
    (void)state;

    pipeline_stage_t s1 = {.name = "Stage_EKF", .process = mock_stage1_proc};
    pipeline_stage_t s2 = {.name = "Stage_FFT", .process = mock_stage2_proc};

    dag_graph_t graph = {0};
    graph.node_count = 2;

    // Node 0 -> Node 1
    graph.nodes[0].stage = &s1;
    graph.nodes[0].in_degree = 0;
    graph.nodes[0].child_count = 1;
    graph.nodes[0].children[0] = &graph.nodes[1];

    graph.nodes[1].stage = &s2;
    graph.nodes[1].in_degree = 1;
    graph.nodes[1].child_count = 0;

    int ret = dag_validate_acyclic(&graph);
    assert_int_equal(ret, 0);
    assert_true(graph.is_acyclic);

    pipe_msg_t msg = {.tag_id = 1001, .payload = {.val = {.f = 220.0}}};
    ret = dag_execute_parallel(NULL, &graph, &msg);
    assert_int_equal(ret, 0);

    assert_int_equal(g_stage1_run, 1);
    assert_int_equal(g_stage2_run, 1);

    // 构造带环图形 Node 0 -> Node 1 -> Node 0
    graph.nodes[1].child_count = 1;
    graph.nodes[1].children[0] = &graph.nodes[0];

    ret = dag_validate_acyclic(&graph);
    assert_int_equal(ret, -1);
    assert_false(graph.is_acyclic);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dag_acyclic_and_execution),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
