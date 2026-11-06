#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "iems_node.h"

typedef struct {
    int calls;
    int bus_id;
    uint32_t tag_id;
    double value;
} fake_exec_t;

static int fake_write(void *ctx, int bus_id, uint32_t tag_id, double value, const char *proto) {
    fake_exec_t *exec = ctx;
    (void)proto;
    exec->calls++;
    exec->bus_id = bus_id;
    exec->tag_id = tag_id;
    exec->value = value;
    return 0;
}

static void test_write_goes_through_injected_port(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "IEMS_EXEC_TEST_%d", (int)getpid());
    iems_node_config_t cfg = {
        .product_name = "IEMS_TEST",
        .shm_name = shm_name,
        .datahub_capacity = 128,
    };
    iems_node_t *node = iems_node_create(&loop, &cfg);
    assert_non_null(node);

    fake_exec_t exec = {0};
    assert_int_equal(iems_node_set_execution_port(
                         node, (iems_execution_port_t){.ctx = &exec, .write_tag = fake_write}),
                     0);

    assert_int_equal(iems_node_write_tag(node, 3, 0x1234u, 42.5, "ModbusRTU"), 0);
    assert_int_equal(exec.calls, 1);
    assert_int_equal(exec.bus_id, 3);
    assert_int_equal(exec.tag_id, 0x1234u);
    assert_true(exec.value == 42.5);

    iems_node_destroy(node);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_write_goes_through_injected_port),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
