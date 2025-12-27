#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <uv.h>
#include <pthread.h>
#include <unistd.h>

#include "datahub.h"

/* ---------------- context ---------------- */

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    int received;
} test_ctx_t;

/* ---------------- producer thread ---------------- */

static void *producer_thread(void *arg)
{
    test_ctx_t *ctx = (test_ctx_t *)arg;

    for (int i = 0; i < 100; i++) {
        pipe_msg_t msg = {0};
        msg.type = PIPEMSG_SCALAR_I64;
        msg.payload.val_i = i;
        
        while (datahub_publish(&ctx->hub, &msg) != 0) {
            uv_sleep(1000);
        }
    }
    return NULL;
}

/* ---------------- async callback ---------------- */

static void on_hub_data(datahub_t *hub, void *user_data)
{
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    pipe_msg_t msg;
    uint64_t last_seq = 0; // In real app, keep state

    // Note: datahub doesn't track sequence for you in on_data, 
    // you must read from its ring.
    // For this test, we just count how many we can read.
    
    static uint64_t my_seq = 0;
    while (shm_ring_read(&hub->ring, &my_seq, &msg) == 0) {
        ctx->received++;
    }

    if (ctx->received >= 100) {
        uv_stop(&ctx->loop);
    }
}

/* ---------------- test ---------------- */

static void test_datahub_notify(void **state)
{
    test_ctx_t *ctx = (test_ctx_t *)*state;

    pthread_t th;
    pthread_create(&th, NULL, producer_thread, ctx);

    uv_run(&ctx->loop, UV_RUN_DEFAULT);

    pthread_join(th, NULL);

    assert_int_equal(ctx->received, 100);
}

/* ---------------- setup / teardown ---------------- */

static int setup(void **state)
{
    test_ctx_t *ctx = calloc(1, sizeof(*ctx));
    assert_non_null(ctx);

    uv_loop_init(&ctx->loop);
    
    shm_unlink("/test_hub");
    datahub_init(&ctx->hub, &ctx->loop, "/test_hub", 128, on_hub_data, ctx);

    *state = ctx;
    return 0;
}

static int teardown(void **state)
{
    test_ctx_t *ctx = (test_ctx_t *)*state;

    datahub_destroy(&ctx->hub);
    uv_loop_close(&ctx->loop);
    free(ctx);

    return 0;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(
            test_datahub_notify,
            setup,
            teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}