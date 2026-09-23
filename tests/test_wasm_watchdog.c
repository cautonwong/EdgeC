#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "wasm_sandbox_watchdog.h"

static void test_wasm_sandbox_watchdog_limits(void** state) {
    (void)state;

    wasm_sandbox_watchdog_t wd;
    wasm_sandbox_watchdog_init(&wd, 8 * 1024 * 1024, 30);  // 8MB Max, 30ms Max CPU

    // 1. Normal execution check
    assert_true(wasm_sandbox_watchdog_check(&wd, 2 * 1024 * 1024, 10));  // 2MB, 10ms
    assert_false(wd.terminated);

    // 2. CPU Timeout check
    assert_false(wasm_sandbox_watchdog_check(&wd, 2 * 1024 * 1024, 25));  // Total 35ms > 30ms limit
    assert_true(wd.terminated);

    printf("🔥 [SUCCESS] WAMR Sandbox CPU/Memory Resource Watchdog test passed!\n");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wasm_sandbox_watchdog_limits),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
