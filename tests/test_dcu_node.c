/**
 * @file test_dcu_node.c
 * @brief Unit tests for DCU Node Aggregate Root (Issue #375)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "dcu_node.h"

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int setup(void **state) {
    uv_loop_t *loop = (uv_loop_t *)malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    uv_walk(loop, on_walk_close, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    free(loop);
    return 0;
}

static void test_dcu_node_lifecycle(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "/tmp/test_dcu_node.db";
    unlink(db_path);

    dcu_node_config_t cfg = {
        .product_name = "DCU_Test",
        .db_path = db_path,
        .datahub_capacity = 1024,
        .poll_interval_ms = 100
    };

    dcu_node_t *node = dcu_node_create(loop, &cfg);
    assert_non_null(node);

    assert_non_null(dcu_node_get_inventory(node));
    assert_non_null(dcu_node_get_poller(node));
    assert_non_null(dcu_node_get_storage(node));
    assert_non_null(dcu_node_get_datahub(node));
    assert_non_null(dcu_node_get_container(node));

    /* Start the node polling loop */
    int rc = dcu_node_start(node);
    assert_int_equal(rc, 0);

    /* Pump active poller */
    rc = dcu_node_pump(node);
    assert_int_equal(rc, 0);

    /* Stop the node */
    rc = dcu_node_stop(node);
    assert_int_equal(rc, 0);

    /* Clean destruction */
    dcu_node_destroy(node);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dcu_node_lifecycle, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
