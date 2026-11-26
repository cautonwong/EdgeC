#include <cmocka.h>
#include <fcntl.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "datahub_priv.h"
#include "iems_app.h"

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
    while (uv_run(&ctx->loop, UV_RUN_DEFAULT) != 0)
        ;

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

    assert_int_equal(
        datahub_init(&hub, &loop, "/test_wal_hub", 256, 128, NULL, 0, NULL, NULL, NULL), 0);

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
    assert_int_equal(
        datahub_init(&hub2, &loop2, "/test_wal_hub2", 256, 128, NULL, 0, NULL, NULL, NULL), 0);

    assert_int_equal(datahub_load_snapshot(&hub2, snap_path), 0);
    strncpy(hub2.wal_path, wal_path, sizeof(hub2.wal_path) - 1);
    assert_int_equal(datahub_replay_wal(&hub2), 0);

    // Verify tag 1 shadow read succeeds
    pipe_msg_t msg_out;
    assert_int_equal(datahub_read_shadow(&hub2, 1, &msg_out), 0);
    assert_int_equal(msg_out.type, PIPEMSG_TYPE_SCALAR);

    datahub_destroy(&hub2);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
    while (uv_run(&loop2, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop2);

    unlink(wal_path);
    unlink(snap_path);
}

static void write_wal_record(int fd, uint32_t tag_id, double value) {
    pipe_msg_t msg = {0};
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.tag_id = tag_id;
    msg.payload.val.f = value;
    assert_int_equal(write(fd, &msg, sizeof(msg)), (ssize_t)sizeof(msg));
}

static void test_datahub_wal_replay_truncates_torn_tail(void** state) {
    (void)state;
    const char* wal_path = "/tmp/test_datahub_wal_tail.log";
    unlink(wal_path);

    int fd = open(wal_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert_true(fd >= 0);
    for (uint32_t tag = 1; tag <= 3; tag++) {
        write_wal_record(fd, tag, (double)tag * 10.0);
    }
    uint8_t garbage[32];
    memset(garbage, 0xAB, sizeof(garbage));
    assert_int_equal(write(fd, garbage, sizeof(garbage)), (ssize_t)sizeof(garbage));
    close(fd);

    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_wal_tail_hub");
    assert_int_equal(
        datahub_init(&hub, &loop, "/test_wal_tail_hub", 256, 128, NULL, 0, NULL, NULL, NULL), 0);
    assert_int_equal(datahub_enable_wal(&hub, wal_path), 0);
    assert_int_equal(datahub_replay_wal(&hub), 0);

    struct stat st;
    assert_int_equal(stat(wal_path, &st), 0);
    assert_int_equal(st.st_size, 3 * (off_t)sizeof(pipe_msg_t));

    pipe_msg_t out;
    for (uint32_t tag = 1; tag <= 3; tag++) {
        assert_int_equal(datahub_read_shadow(&hub, tag, &out), 0);
        assert_int_equal(out.tag_id, tag);
        assert_true(out.payload.val.f == (double)tag * 10.0);
    }

    pipe_msg_t next = {0};
    next.type = PIPEMSG_TYPE_SCALAR;
    next.tag_id = 4;
    next.payload.val.f = 40.0;
    assert_int_equal(datahub_publish(&hub, &next), 0);
    assert_int_equal(stat(wal_path, &st), 0);
    assert_int_equal(st.st_size, 4 * (off_t)sizeof(pipe_msg_t));
    datahub_destroy(&hub);

    datahub_t hub2;
    uv_loop_t loop2;
    uv_loop_init(&loop2);
    shm_unlink("/test_wal_tail_hub2");
    assert_int_equal(
        datahub_init(&hub2, &loop2, "/test_wal_tail_hub2", 256, 128, NULL, 0, NULL, NULL, NULL), 0);
    strncpy(hub2.wal_path, wal_path, sizeof(hub2.wal_path) - 1);
    assert_int_equal(datahub_replay_wal(&hub2), 0);
    assert_int_equal(datahub_read_shadow(&hub2, 4, &out), 0);
    assert_int_equal(out.tag_id, 4);
    assert_true(out.payload.val.f == 40.0);
    datahub_destroy(&hub2);

    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
    while (uv_run(&loop2, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop2);
    unlink(wal_path);
}

static void test_datahub_wal_write_failure_counted(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_wal_full_hub");
    assert_int_equal(
        datahub_init(&hub, &loop, "/test_wal_full_hub", 256, 128, NULL, 0, NULL, NULL, NULL), 0);

    // /dev/full opens fine but every write returns ENOSPC.
    assert_int_equal(datahub_enable_wal(&hub, "/dev/full"), 0);

    pipe_msg_t msg = {0};
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.tag_id = 1;
    msg.payload.val.f = 1.0;
    assert_int_equal(datahub_publish(&hub, &msg), 0);
    assert_int_equal(atomic_load(&hub.wal_write_failures), 1);
    assert_true(hub.wal_fd < 0); /* a failed write disables the WAL */
    assert_int_equal(datahub_publish(&hub, &msg), 0);
    assert_int_equal(atomic_load(&hub.wal_write_failures), 1);

    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
}

static void test_iems_app_wal_start_degrades(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_iems_wal_start_hub");
    assert_int_equal(
        datahub_init(&hub, &loop, "/test_iems_wal_start_hub", 256, 128, NULL, 0, NULL, NULL, NULL),
        0);

    // 打不开的路径：产品明确降级而不是崩溃，WAL 路径随之失效。
    assert_int_equal(iems_app_wal_start(&hub, "/nonexistent-wal-dir/wal.log"), -1);
    assert_int_equal(datahub_replay_wal(&hub), -1);

    const char* wal_path = "/tmp/test_iems_wal_start.log";
    unlink(wal_path);
    assert_int_equal(iems_app_wal_start(&hub, wal_path), 0);

    pipe_msg_t msg = {0};
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.tag_id = 1;
    msg.payload.val.f = 7.0;
    assert_int_equal(datahub_publish(&hub, &msg), 0);

    struct stat st;
    assert_int_equal(stat(wal_path, &st), 0);
    assert_int_equal(st.st_size, (off_t)sizeof(pipe_msg_t));

    // 降级关闭后写入照常进影子表，但不再追加 WAL。
    datahub_disable_wal(&hub);
    assert_int_equal(datahub_publish(&hub, &msg), 0);
    assert_int_equal(stat(wal_path, &st), 0);
    assert_int_equal(st.st_size, (off_t)sizeof(pipe_msg_t));

    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
    unlink(wal_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_datahub_notify, setup, teardown),
        cmocka_unit_test(test_datahub_wal_concurrency),
        cmocka_unit_test(test_datahub_wal_replay_truncates_torn_tail),
        cmocka_unit_test(test_datahub_wal_write_failure_counted),
        cmocka_unit_test(test_iems_app_wal_start_degrades),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}