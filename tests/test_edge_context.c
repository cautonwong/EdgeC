#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "core/edge_context.h"

// Test that the context can be initialized and shut down without crashing.
static void test_edge_ctx_init_and_shutdown(void **state) {
    (void)state; // Unused

    edge_ctx_t ctx;

    // Initialize the context
    int r = edge_ctx_init(&ctx);
    assert_int_equal(r, 0);
    assert_non_null(ctx.arbiter);

    // Shut down the context
    // In a real app, edge_ctx_start() would be called here.
    // For this test, we just want to ensure the shutdown logic is sound.
    edge_ctx_stop(&ctx);

    // The final check is that the test completes without a crash.
    // More detailed tests would mock sub-modules to check call order, etc.
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_edge_ctx_init_and_shutdown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
