/**
 * @file test_ai_graph_pruner.c
 * @brief Unit tests for Neural Network Computation Graph Topological Pruner (AEP-708 / Issue #241)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ai_graph_pruner.h"

static void test_graph_dead_node_and_identity_pruning(void **state) {
    (void)state;

    ai_graph_pruner_storage_t storage;
    ai_graph_pruner_t *pruner = ai_graph_pruner_create_in(&storage);
    assert_non_null(pruner);

    /* Construct a graph with dead branches and identity layers:
     * N0 (Input) -> N1 (Conv1D) -> N2 (Identity) -> N3 (ReLU) -> N4 (Output)
     * N0 -> N5 (Dead Conv) -> N6 (Dead Dropout) [No output connection]
     */
    int32_t n0 = ai_graph_pruner_add_node(pruner, "input", AI_OP_INPUT);
    int32_t n1 = ai_graph_pruner_add_node(pruner, "conv1", AI_OP_CONV1D);
    int32_t n2 = ai_graph_pruner_add_node(pruner, "identity1", AI_OP_IDENTITY);
    int32_t n3 = ai_graph_pruner_add_node(pruner, "relu1", AI_OP_RELU);
    int32_t n4 = ai_graph_pruner_add_node(pruner, "output", AI_OP_OUTPUT);

    int32_t n5 = ai_graph_pruner_add_node(pruner, "dead_conv", AI_OP_CONV1D);
    int32_t n6 = ai_graph_pruner_add_node(pruner, "dead_dropout", AI_OP_DROPOUT);

    assert_true(ai_graph_pruner_add_edge(pruner, (uint32_t)n0, (uint32_t)n1));
    assert_true(ai_graph_pruner_add_edge(pruner, (uint32_t)n1, (uint32_t)n2));
    assert_true(ai_graph_pruner_add_edge(pruner, (uint32_t)n2, (uint32_t)n3));
    assert_true(ai_graph_pruner_add_edge(pruner, (uint32_t)n3, (uint32_t)n4));

    assert_true(ai_graph_pruner_add_edge(pruner, (uint32_t)n0, (uint32_t)n5));
    assert_true(ai_graph_pruner_add_edge(pruner, (uint32_t)n5, (uint32_t)n6));

    /* Optimize graph */
    assert_true(ai_graph_pruner_optimize(pruner));

    ai_graph_stats_t stats;
    assert_true(ai_graph_pruner_get_stats(pruner, &stats));

    /* Dead nodes n5, n6 should be pruned */
    assert_int_equal(stats.pruned_dead_nodes, 2);
    /* Identity node n2 should be folded */
    assert_int_equal(stats.folded_identity_nodes, 1);
    /* DAG should be acyclic and valid */
    assert_true(stats.is_acyclic_dag);

    const ai_graph_node_t *node5 = ai_graph_pruner_get_node(pruner, (uint32_t)n5);
    assert_non_null(node5);
    assert_int_equal(node5->status, AI_NODE_STATUS_PRUNED);

    const ai_graph_node_t *node2 = ai_graph_pruner_get_node(pruner, (uint32_t)n2);
    assert_non_null(node2);
    assert_int_equal(node2->status, AI_NODE_STATUS_FOLDED);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_graph_dead_node_and_identity_pruning),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
