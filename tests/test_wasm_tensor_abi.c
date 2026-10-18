/**
 * @file test_wasm_tensor_abi.c
 * @brief Unit tests for WASM Runtime Tensor ABI & Sliding Window Pipeline (ADR-0009 / Issue #218,
 * #219)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "wasm_mgr.h"

typedef struct {
    datahub_t hub;
    uv_loop_t *loop;
    wasm_app_t *app;
} test_wasm_abi_fixture_t;

static int setup_wasm_abi(void **state) {
    test_wasm_abi_fixture_t *fix = calloc(1, sizeof(*fix));
    assert_non_null(fix);

    fix->loop = uv_default_loop();
    int rc = datahub_init(&fix->hub, fix->loop, "wasm_tensor_hub", 128, 10000, NULL, 0, NULL, NULL,
                          NULL);
    assert_int_equal(rc, 0);

    wasm_mgr_init(&fix->hub);

    const char *wasm_path = "../../algo/wasm/demo/demo.wasm";
    fix->app = wasm_app_create(wasm_path, 32768, 65536, 100);
    assert_non_null(fix->app);

    *state = fix;
    return 0;
}

static int teardown_wasm_abi(void **state) {
    test_wasm_abi_fixture_t *fix = (test_wasm_abi_fixture_t *)*state;
    if (fix) {
        if (fix->app) {
            wasm_app_destroy(fix->app);
            fix->app = NULL;
        }
        wasm_mgr_cleanup();
        datahub_destroy(&fix->hub);
        if (fix->loop) {
            while (uv_run(fix->loop, UV_RUN_NOWAIT))
                ;
        }
        free(fix);
    }
    return 0;
}

/**
 * @brief Test allocation, zero-copy pointer translation, write, read and free of tensors in WASM
 * memory
 */
static void test_wasm_tensor_allocation_and_rw(void **state) {
    test_wasm_abi_fixture_t *fix = (test_wasm_abi_fixture_t *)*state;
    wasm_app_t *app = fix->app;

    const uint32_t tensor_len = 16;
    const uint32_t tensor_bytes = tensor_len * sizeof(float);
    float host_in_tensor[16];
    for (uint32_t i = 0; i < tensor_len; i++) {
        host_in_tensor[i] = 100.0f + (float)i * 2.5f;
    }

    uint32_t wasm_tensor_ptr = 0;
    int rc = wasm_app_tensor_alloc(app, tensor_bytes, &wasm_tensor_ptr);
    assert_int_equal(rc, 0);
    assert_true(wasm_tensor_ptr > 0);

    rc = wasm_app_tensor_write(app, wasm_tensor_ptr, host_in_tensor, tensor_bytes);
    assert_int_equal(rc, 0);

    float host_out_tensor[16] = {0};
    rc = wasm_app_tensor_read(app, wasm_tensor_ptr, host_out_tensor, tensor_bytes);
    assert_int_equal(rc, 0);

    for (uint32_t i = 0; i < tensor_len; i++) {
        assert_float_equal(host_out_tensor[i], host_in_tensor[i], 0.0001f);
    }

    rc = wasm_app_tensor_free(app, wasm_tensor_ptr);
    assert_int_equal(rc, 0);
}

/**
 * @brief Test boundary checks, NULL safety, and error handling for Tensor ABI
 */
static void test_wasm_tensor_abi_edge_cases(void **state) {
    test_wasm_abi_fixture_t *fix = (test_wasm_abi_fixture_t *)*state;
    wasm_app_t *app = fix->app;

    uint32_t wasm_ptr = 0;
    // NULL app check
    assert_int_equal(wasm_app_tensor_alloc(NULL, 128, &wasm_ptr), -1);
    // Zero size check
    assert_int_equal(wasm_app_tensor_alloc(app, 0, &wasm_ptr), -1);
    // NULL output pointer check
    assert_int_equal(wasm_app_tensor_alloc(app, 128, NULL), -1);

    // Free with invalid wasm_ptr
    assert_int_equal(wasm_app_tensor_free(NULL, 100), -1);
    assert_int_equal(wasm_app_tensor_free(app, 0), -1);

    // Write bounds check
    float buf[4] = {1, 2, 3, 4};
    assert_int_equal(wasm_app_tensor_write(NULL, 100, buf, sizeof(buf)), -1);
    assert_int_equal(wasm_app_tensor_write(app, 0, buf, sizeof(buf)), -1);
    assert_int_equal(wasm_app_tensor_write(app, 100, NULL, sizeof(buf)), -1);
    assert_int_equal(wasm_app_tensor_write(app, 100, buf, 0), -1);
    // Invalid huge address out-of-bounds check
    assert_int_equal(wasm_app_tensor_write(app, 0xFFFFFF00, buf, sizeof(buf)), -1);

    // Read bounds check
    assert_int_equal(wasm_app_tensor_read(NULL, 100, buf, sizeof(buf)), -1);
    assert_int_equal(wasm_app_tensor_read(app, 0, buf, sizeof(buf)), -1);
    assert_int_equal(wasm_app_tensor_read(app, 100, NULL, sizeof(buf)), -1);
    assert_int_equal(wasm_app_tensor_read(app, 100, buf, 0), -1);
    assert_int_equal(wasm_app_tensor_read(app, 0xFFFFFF00, buf, sizeof(buf)), -1);
}

/**
 * @brief Test wasm_app_exec_args for multi-argument function dispatch
 */
static void test_wasm_exec_args_invocation(void **state) {
    test_wasm_abi_fixture_t *fix = (test_wasm_abi_fixture_t *)*state;
    wasm_app_t *app = fix->app;

    // Call on_tick with 0 args using wasm_app_exec_args
    int rc = wasm_app_exec_args(app, "on_tick", 0, NULL);
    assert_int_equal(rc, 0);

    // Non-existent function must return -1
    rc = wasm_app_exec_args(app, "non_existent_func", 0, NULL);
    assert_int_equal(rc, -1);

    // NULL arguments check
    assert_int_equal(wasm_app_exec_args(NULL, "on_tick", 0, NULL), -1);
    assert_int_equal(wasm_app_exec_args(app, NULL, 0, NULL), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_wasm_tensor_allocation_and_rw, setup_wasm_abi,
                                        teardown_wasm_abi),
        cmocka_unit_test_setup_teardown(test_wasm_tensor_abi_edge_cases, setup_wasm_abi,
                                        teardown_wasm_abi),
        cmocka_unit_test_setup_teardown(test_wasm_exec_args_invocation, setup_wasm_abi,
                                        teardown_wasm_abi),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
