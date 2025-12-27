#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <stdio.h>
#include <string.h>

#include "arbiter.h"
#include "shmring.h"

shm_ring_t *global_ring = NULL;

typedef struct {
    uv_loop_t *loop;
    arbiter_t *arb;
    int granted_count;
    char granted_order[10][20]; 
} test_arbiter_ctx_t;

typedef struct {
    test_arbiter_ctx_t *test_ctx;
    const char *name;
} my_request_data_t;

static void on_grant_cb(arbiter_t *arb, arbiter_bus_t bus, void *user) {
    my_request_data_t *req_data = (my_request_data_t *)user;
    test_arbiter_ctx_t *ctx = req_data->test_ctx;
    const char *name = req_data->name;

    assert_non_null(ctx);
    assert_true(ctx->granted_count < 10);
    strncpy(ctx->granted_order[ctx->granted_count], name, 19);
    ctx->granted_order[ctx->granted_count][19] = '\0';
    ctx->granted_count++;

    arbiter_release(arb, bus, req_data);

    if (ctx->granted_count >= 3) {
        arbiter_destroy(arb);
    }
}

static void test_arbiter_scheduling(void **state) {
    (void)state;
    test_arbiter_ctx_t ctx = {0};
    ctx.loop = uv_default_loop();
    ctx.arb = arbiter_create(ctx.loop);

    my_request_data_t polling_data = { .test_ctx = &ctx, .name = "polling" };
    my_request_data_t transparent_data = { .test_ctx = &ctx, .name = "transparent" };
    my_request_data_t ota_data = { .test_ctx = &ctx, .name = "ota" };

    arbiter_request(ctx.arb, BUS_RS485_1,
        ARB_PRIO_LOW, ARB_SHARED, 0,
        on_grant_cb, &polling_data);

    arbiter_request(ctx.arb, BUS_RS485_1,
        ARB_PRIO_HIGH, ARB_SHARED, 0,
        on_grant_cb, &transparent_data);

    arbiter_request(ctx.arb, BUS_RS485_1,
        ARB_PRIO_CRITICAL, ARB_EXCLUSIVE, 0,
        on_grant_cb, &ota_data);

    int i;
    for (i=0; i<500; i++) {
        uv_run(ctx.loop, UV_RUN_NOWAIT);
        if (ctx.granted_count >= 3) break;
        uv_sleep(1); // libuv sleep
    }
    
    uv_run(ctx.loop, UV_RUN_DEFAULT);

    assert_int_equal(ctx.granted_count, 3);
    assert_string_equal(ctx.granted_order[0], "ota");         
    assert_string_equal(ctx.granted_order[1], "transparent"); 
    assert_string_equal(ctx.granted_order[2], "polling");     

    uv_loop_close(ctx.loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_arbiter_scheduling),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}