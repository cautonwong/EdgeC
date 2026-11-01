/**
 * @file test_northbound_event_driven.c
 * @brief 事件驱动取数（ADR-0011 N6-7）
 *
 * ## 这条测试在证什么
 *
 * 原来北向靠 100ms 定时轮询 ring，端到端延迟下限就是 100ms，空转也耗 CPU。
 * 改成：source 一有数据就 `uv_async_send` 唤醒，轮询退化成 500ms 兜底。
 *
 * **光看代码看不出差别**——两种实现的输出完全一样，只是快慢不同。
 * 所以这里量时间：在 t≈0 触发唤醒，如果 400ms 内没发出去，
 * 说明它还在等 500ms 的兜底轮询。
 *
 * 400ms 这个阈值是刻意的：比兜底轮询（500ms）小，比正常唤醒延迟（<10ms）大得多，
 * 两边都有余量，不会因为 CI 抖动而假红。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "edge_north_plane.h"
#include "northbound_mgr.h"

/* ---- 假数据源：手工塞消息，手工触发唤醒 ---- */

#define FAKE_QUEUE 8

typedef struct {
    pipe_msg_t msgs[FAKE_QUEUE];
    int head;
    int tail;
    void (*notify)(void*);
    void* notify_user;
} fake_source_t;

static int fs_read_next(void* ctx, uint64_t* seq, pipe_msg_t* out) {
    fake_source_t* fs = (fake_source_t*)ctx;
    if (fs->head == fs->tail) return 1; /* 无数据 */
    *out = fs->msgs[fs->head];
    fs->head = (fs->head + 1) % FAKE_QUEUE;
    if (seq) (*seq)++;
    return 0;
}

static int fs_subscribe_notify(void* ctx, void (*on_data)(void*), void* user) {
    fake_source_t* fs = (fake_source_t*)ctx;
    fs->notify = on_data;
    fs->notify_user = user;
    return 0;
}

static void fs_push(fake_source_t* fs, uint32_t tag, double val) {
    pipe_msg_t m;
    memset(&m, 0, sizeof(m));
    m.tag_id = tag;
    m.ts = 1000;
    m.payload.val.f = val;
    m.type = PIPEMSG_TYPE_SCALAR;
    fs->msgs[fs->tail] = m;
    fs->tail = (fs->tail + 1) % FAKE_QUEUE;
}

/* ---- 假传输：只记第一次 publish 的时刻 ---- */

typedef struct {
    int published;
    char address[128];
} fake_transport_t;

static struct timespec g_t0;

static double _ms_since_start(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - g_t0.tv_sec) * 1000.0 +
           (double)(now.tv_nsec - g_t0.tv_nsec) / 1e6;
}

static int ft_publish(void* ctx, const char* topic, const uint8_t* p, size_t l) {
    (void)p;
    (void)l;
    fake_transport_t* ft = (fake_transport_t*)ctx;
    if (!ft->published) {
        ft->published = 1;
        snprintf(ft->address, sizeof(ft->address), "%s", topic ? topic : "");
    }
    return 0;
}

static bool ft_connected(void* ctx) {
    (void)ctx;
    return true;
}

static void test_notify_beats_polling(void** state) {
    (void)state;
    fake_source_t fs;
    memset(&fs, 0, sizeof(fs));
    fake_transport_t ft;
    memset(&ft, 0, sizeof(ft));

    northbound_mgr_t mgr;
    nb_source_ops_t src = {
        .ctx = &fs,
        .read_next = fs_read_next,
        .subscribe_notify = fs_subscribe_notify,
    };
    nb_transport_ops_t t = {.ctx = &ft, .publish = ft_publish, .is_connected = ft_connected};

    assert_int_equal(northbound_mgr_init_ex(&mgr, src, "test", NULL, t), 0);
    northbound_mgr_set_address(&mgr, "edge/{node}/{kind}", "G", "N1");

    clock_gettime(CLOCK_MONOTONIC, &g_t0);
    assert_int_equal(northbound_mgr_start(&mgr), 0);

    /* 等唤醒回调注册上（在线程入口里做） */
    for (int i = 0; i < 200 && !fs.notify; i++) usleep(10000);
    assert_non_null(fs.notify);

    /* 塞一条数据 + 触发唤醒 */
    fs_push(&fs, 1001, 42.0);
    fs.notify(fs.notify_user);

    /* 兜底轮询是 500ms；若 400ms 内没发出去，说明唤醒没生效 */
    for (int i = 0; i < 200 && !ft.published; i++) usleep(2000);

    assert_int_equal(ft.published, 1);
    double latency = _ms_since_start();
    assert_true(latency < 400.0);
    assert_string_equal(ft.address, "edge/N1/NDATA");

    northbound_mgr_stop(&mgr);
    northbound_mgr_destroy(&mgr);
}

/** 没有 subscribe_notify 的 source 不能崩，只是退化成轮询。 */
static void test_polling_fallback_still_works(void** state) {
    (void)state;
    fake_source_t fs;
    memset(&fs, 0, sizeof(fs));
    fake_transport_t ft;
    memset(&ft, 0, sizeof(ft));

    northbound_mgr_t mgr;
    nb_source_ops_t src = {.ctx = &fs, .read_next = fs_read_next}; /* 故意不给 notify */
    nb_transport_ops_t t = {.ctx = &ft, .publish = ft_publish, .is_connected = ft_connected};

    assert_int_equal(northbound_mgr_init_ex(&mgr, src, "test", NULL, t), 0);
    northbound_mgr_set_address(&mgr, "edge/{node}/{kind}", "G", "N1");
    assert_int_equal(northbound_mgr_start(&mgr), 0);

    fs_push(&fs, 1002, 7.0);

    /* 靠 500ms 兜底轮询发出去 */
    for (int i = 0; i < 400 && !ft.published; i++) usleep(5000);
    assert_int_equal(ft.published, 1);

    northbound_mgr_stop(&mgr);
    northbound_mgr_destroy(&mgr);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_notify_beats_polling),
        cmocka_unit_test(test_polling_fallback_still_works),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
