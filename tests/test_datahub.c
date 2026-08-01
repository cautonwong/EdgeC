#include <cmocka.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    int received;
    uv_timer_t timeout;
} test_ctx_t;

static void on_timeout(uv_timer_t* handle) {
    test_ctx_t* ctx = (test_ctx_t*)handle->data;
    fprintf(stderr, "[FATAL] Test timed out! Received: %d/100\n", ctx->received);
    uv_stop(&ctx->loop);
    exit(1);  // Force fail
}

static void* producer_thread(void* arg) {
    test_ctx_t* ctx = (test_ctx_t*)arg;
    for (int i = 0; i < 100; i++) {
        pipe_msg_t msg = {0};
        msg.type = PIPEMSG_TYPE_SCALAR;
        msg.payload.val.i = (int64_t)i;
        datahub_publish(&ctx->hub, &msg);

        // 关键修复：在 QEMU/ARM64 环境下，强制让渡 CPU 时间片，
        // 否则生产者可能饿死消费者线程，导致 uv_async 无法及时处理
        uv_sleep(1);
    }
    return NULL;
}

static void on_hub_data(datahub_t* hub, void* user_data) {
    test_ctx_t* ctx = (test_ctx_t*)user_data;
    pipe_msg_t msg;
    static uint64_t my_seq = 0;
    while (shm_ring_read(&hub->ring, &my_seq, &msg) == 0) {
        ctx->received++;
    }
    if (ctx->received >= 100) {
        uv_timer_stop(&ctx->timeout);
        uv_stop(&ctx->loop);
    }
}

static void test_datahub_notify(void** state) {
    test_ctx_t* ctx = (test_ctx_t*)*state;

    // 安全网：5秒超时
    uv_timer_init(&ctx->loop, &ctx->timeout);
    ctx->timeout.data = ctx;
    uv_timer_start(&ctx->timeout, on_timeout, 5000, 0);

    pthread_t th;
    pthread_create(&th, NULL, producer_thread, ctx);

    uv_run(&ctx->loop, UV_RUN_DEFAULT);

    pthread_join(th, NULL);
    assert_int_equal(ctx->received, 100);
}

static int setup(void** state) {
    test_ctx_t* ctx = calloc(1, sizeof(*ctx));
    uv_loop_init(&ctx->loop);
    shm_unlink("/test_hub");

    // 增加返回值断言
    if (datahub_init(&ctx->hub, &ctx->loop, "/test_hub", 128, 1024, NULL, 0, NULL, NULL, NULL) !=
        0) {
        fprintf(stderr, "[FATAL] datahub_init failed (check /dev/shm permissions?)\n");
        free(ctx);
        return -1;
    }

    datahub_subscribe(&ctx->hub, on_hub_data, ctx);

    *state = ctx;
    return 0;
}

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int teardown(void** state) {
    test_ctx_t* ctx = (test_ctx_t*)*state;
    datahub_destroy(&ctx->hub);

    // 关键修复：关闭所有句柄 (包括 timeout timer)
    uv_walk(&ctx->loop, on_walk_close, NULL);

    // 运行循环直到所有关闭回调完成
    while (uv_run(&ctx->loop, UV_RUN_DEFAULT) != 0);

    if (uv_loop_close(&ctx->loop) != 0) {
        fprintf(stderr, "[WARN] Loop close failed (leak?)\n");
    }

    free(ctx);
    return 0;
}

static void* wal_writer_thread(void* arg) {
    datahub_t* hub = (datahub_t*)arg;
    for (int i = 0; i < 200; i++) {
        pipe_msg_t msg = {0};
        msg.tag_id = (uint32_t)(i % 50 + 1);
        msg.type = PIPEMSG_TYPE_SCALAR;
        msg.payload.val.f = (double)(i + 100);
        datahub_publish(hub, &msg);
        uv_sleep(1);
    }
    return NULL;
}

static void* wal_reader_thread(void* arg) {
    datahub_t* hub = (datahub_t*)arg;
    pipe_msg_t msg;
    for (int i = 0; i < 150; i++) {
        for (uint32_t tag = 1; tag <= 50; tag++) {
            datahub_read_shadow(hub, tag, &msg);
        }
        uv_sleep(1);
    }
    return NULL;
}

static void test_datahub_wal_concurrency(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_wal_hub");

    assert_int_equal(datahub_init(&hub, &loop, "/test_wal_hub", 256, 128, NULL, 0, NULL, NULL, NULL), 0);

    const char* wal_path = "/tmp/test_datahub_wal_t103.log";
    const char* snap_path = "/tmp/test_datahub_snap_t103.bin";
    unlink(wal_path);
    unlink(snap_path);

    assert_int_equal(datahub_enable_wal(&hub, wal_path), 0);

    pthread_t w1, w2, r1;
    pthread_create(&w1, NULL, wal_writer_thread, &hub);
    pthread_create(&w2, NULL, wal_writer_thread, &hub);
    pthread_create(&r1, NULL, wal_reader_thread, &hub);

    // Concurrently trigger snapshot compaction
    uv_sleep(50);
    hub.snapshot_path[0] = '\0';
    strncpy(hub.snapshot_path, snap_path, sizeof(hub.snapshot_path) - 1);
    datahub_compact_now(&hub);

    pthread_join(w1, NULL);
    pthread_join(w2, NULL);
    pthread_join(r1, NULL);

    // Save final snapshot and test WAL replay into a fresh hub
    datahub_save_snapshot(&hub, snap_path);
    datahub_destroy(&hub);

    datahub_t hub2;
    uv_loop_t loop2;
    uv_loop_init(&loop2);
    shm_unlink("/test_wal_hub2");
    assert_int_equal(datahub_init(&hub2, &loop2, "/test_wal_hub2", 256, 128, NULL, 0, NULL, NULL, NULL), 0);

    assert_int_equal(datahub_load_snapshot(&hub2, snap_path), 0);
    strncpy(hub2.wal_path, wal_path, sizeof(hub2.wal_path) - 1);
    assert_int_equal(datahub_replay_wal(&hub2), 0);

    // Verify tag 1 shadow read succeeds
    pipe_msg_t msg_out;
    assert_int_equal(datahub_read_shadow(&hub2, 1, &msg_out), 0);
    assert_int_equal(msg_out.type, PIPEMSG_TYPE_SCALAR);

    datahub_destroy(&hub2);
    while (uv_run(&loop, UV_RUN_NOWAIT));
    uv_loop_close(&loop);
    while (uv_run(&loop2, UV_RUN_NOWAIT));
    uv_loop_close(&loop2);

    unlink(wal_path);
    unlink(snap_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_datahub_notify, setup, teardown),
        cmocka_unit_test(test_datahub_wal_concurrency),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}