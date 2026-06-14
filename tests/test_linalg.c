#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "edge_linalg.h"

void test_matrix_basic() {
    printf("[Test] Running Linalg Basic Test...\n");
    edge_matrix_t* a = edge_matrix_alloc(2, 2);
    edge_matrix_t* b = edge_matrix_alloc(2, 2);
    edge_matrix_t* c = edge_matrix_alloc(2, 2);

    edge_matrix_set(a, 0, 0, 1.0);
    edge_matrix_set(a, 0, 1, 2.0);
    edge_matrix_set(a, 1, 0, 3.0);
    edge_matrix_set(a, 1, 1, 4.0);

    edge_matrix_set(b, 0, 0, 5.0);
    edge_matrix_set(b, 0, 1, 6.0);
    edge_matrix_set(b, 1, 0, 7.0);
    edge_matrix_set(b, 1, 1, 8.0);

    // Test Addition
    edge_matrix_add(a, b, c);
    assert(edge_matrix_get(c, 0, 0) == 6.0);
    assert(edge_matrix_get(c, 1, 1) == 12.0);
    printf("  - Addition OK\n");

    // Test Multiplication
    // [1 2] * [5 6] = [19 22]
    // [3 4]   [7 8]   [43 50]
    edge_matrix_mul(a, b, c);
    assert(edge_matrix_get(c, 0, 0) == 19.0);
    assert(edge_matrix_get(c, 0, 1) == 22.0);
    assert(edge_matrix_get(c, 1, 0) == 43.0);
    assert(edge_matrix_get(c, 1, 1) == 50.0);
    printf("  - Multiplication OK\n");

    edge_matrix_free(a);
    edge_matrix_free(b);
    edge_matrix_free(c);
}

void test_matrix_inversion() {
    printf("[Test] Running Linalg Inversion Test...\n");
    edge_matrix_t* m = edge_matrix_alloc(2, 2);
    edge_matrix_t* inv = edge_matrix_alloc(2, 2);
    edge_matrix_t* id = edge_matrix_alloc(2, 2);

    // [4 7]
    // [2 6]
    // det = 4*6 - 7*2 = 24 - 14 = 10
    // inv = 1/10 * [6 -7] = [0.6 -0.7]
    //              [-2 4]   [-0.2 0.4]
    edge_matrix_set(m, 0, 0, 4.0);
    edge_matrix_set(m, 0, 1, 7.0);
    edge_matrix_set(m, 1, 0, 2.0);
    edge_matrix_set(m, 1, 1, 6.0);

    int ret = edge_matrix_inv(m, inv);
    assert(ret == 0);

    assert(fabs(edge_matrix_get(inv, 0, 0) - 0.6) < 1e-9);
    assert(fabs(edge_matrix_get(inv, 0, 1) + 0.7) < 1e-9);
    assert(fabs(edge_matrix_get(inv, 1, 0) + 0.2) < 1e-9);
    assert(fabs(edge_matrix_get(inv, 1, 1) - 0.4) < 1e-9);

    // Verify inv(M)*M = I
    edge_matrix_mul(m, inv, id);
    assert(fabs(edge_matrix_get(id, 0, 0) - 1.0) < 1e-9);
    assert(fabs(edge_matrix_get(id, 0, 1)) < 1e-9);
    assert(fabs(edge_matrix_get(id, 1, 0)) < 1e-9);
    assert(fabs(edge_matrix_get(id, 1, 1) - 1.0) < 1e-9);

    printf("  - Inversion OK\n");

    edge_matrix_free(m);
    edge_matrix_free(inv);
    edge_matrix_free(id);
}

int main() {
    test_matrix_basic();
    test_matrix_inversion();
    printf("[Test] All Linalg tests passed!\n");
    return 0;
}
