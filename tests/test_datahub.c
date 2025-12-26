#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <unistd.h> // For usleep

#include "datahub.h"

// Test context to hold state
typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    
    int callback_count;
    pipe_msg_t received_msg;
    uint64_t last_seq;
} test_datahub_ctx_t;

// The callback function that the consumer (main thread) provides to the datahub
static void consumer_on_data_cb(datahub_t *hub, void *user_data) {
    test_datahub_ctx_t *ctx = (test_datahub_ctx_t *)user_data;
    
    // In a real app, we would loop until the ring is empty
    if (shm_ring_read(&hub->ring, &ctx->last_seq, &ctx->received_msg) == 0) {
        ctx->callback_count++;
    }
}

// The producer thread's entry point
static void producer_thread_entry(void *arg) {
    datahub_t *hub = (datahub_t *)arg;

    // Wait a moment to ensure the consumer loop is running
    usleep(10000); // 10ms

    // Create and publish a message
    pipe_msg_t msg_to_send;
    msg_to_send.type = PIPE_SCALAR_F64;
    msg_to_send.payload.val_f = 99.5;

    datahub_publish(hub, &msg_to_send);
}

// The main test case
static void test_datahub_publish_notifies_consumer(void **state) {
    (void)state;

    // 1. Setup
    test_datahub_ctx_t ctx = {0};
    uv_loop_init(&ctx.loop);

    const char *ring_name = "/test_datahub_ring";
    int r = datahub_init(&ctx.hub, &ctx.loop, ring_name, 8, consumer_on_data_cb, &ctx);
    assert_int_equal(r, 0);

    // 2. Start producer thread
    uv_thread_t producer_thread;
    uv_thread_create(&producer_thread, producer_thread_entry, &ctx.hub);

    // 3. Run consumer loop
    // This will block until the producer sends an async notification
    // or until all active handles are closed. We expect the async notification.
    printf("Consumer loop running...\n");
    uv_run(&ctx.loop, UV_RUN_DEFAULT);
    printf("Consumer loop stopped.\n");
    
    // The producer calls publish, which sends the async signal.
    // The loop wakes up, calls our on_async_notification -> consumer_on_data_cb.
    // However, the loop doesn't stop on its own. We need to stop it.
    // A better way is to stop it from the callback. Let's modify the callback.

    // Let's re-run the loop briefly to ensure all events are processed before we assert.
    uv_run(&ctx.loop, UV_RUN_NOWAIT);

    // 4. Join producer thread
    uv_thread_join(&producer_thread);
    
    // 5. Assertions
    assert_int_equal(ctx.callback_count, 1);
    assert_int_equal(ctx.received_msg.type, PIPE_SCALAR_F64);
    assert_float_equal(ctx.received_msg.payload.val_f, 99.5, 0.01);

    // 6. Teardown
    datahub_destroy(&ctx.hub);
    
    // Run loop once more to process close handles
    uv_run(&ctx.loop, UV_RUN_DEFAULT);
    uv_loop_close(&ctx.loop);
}


// A revised callback that stops the loop for the test
static void consumer_on_data_and_stop_cb(datahub_t *hub, void *user_data) {
    test_datahub_ctx_t *ctx = (test_datahub_ctx_t *)user_data;
    
    while (shm_ring_read(&hub->ring, &ctx->last_seq, &ctx->received_msg) == 0) {
        ctx->callback_count++;
    }
    
    // Stop the loop so the test can proceed to assertions
    uv_stop(&ctx->loop);
}

static void test_datahub_with_stop(void **state) {
    (void)state;

    // 1. Setup
    test_datahub_ctx_t ctx = {0};
    uv_loop_init(&ctx.loop);

    const char *ring_name = "/test_datahub_stop_ring";
    int r = datahub_init(&ctx.hub, &ctx.loop, ring_name, 8, consumer_on_data_and_stop_cb, &ctx);
    assert_int_equal(r, 0);

    // 2. Start producer thread
    uv_thread_t producer_thread;
    uv_thread_create(&producer_thread, producer_thread_entry, &ctx.hub);

    // 3. Run consumer loop. It will be stopped by the callback.
    printf("Consumer loop running (will be stopped by callback)...\n");
    uv_run(&ctx.loop, UV_RUN_DEFAULT);
    printf("Consumer loop stopped.\n");
    
    // 4. Join producer thread
    uv_thread_join(&producer_thread);
    
    // 5. Assertions
    assert_int_equal(ctx.callback_count, 1);
    assert_int_equal(ctx.received_msg.type, PIPE_SCALAR_F64);
    assert_float_equal(ctx.received_msg.payload.val_f, 99.5, 0.01);

    // 6. Teardown
    datahub_destroy(&ctx.hub);
    
    // Run loop once more to process close handles
    uv_run(&ctx.loop, UV_RUN_DEFAULT);
    assert_int_equal(uv_loop_close(&ctx.loop), 0);
}


int main(void) {
    const struct CMUnitTest tests[] = {
        // This test is flaky because uv_run might exit early if there are no active handles
        // before the producer thread can send the notification.
        // cmocka_unit_test(test_datahub_publish_notifies_consumer), 
        
        // This version is more robust for testing.
        cmocka_unit_test(test_datahub_with_stop),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
