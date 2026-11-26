#define _GNU_SOURCE

#include <cmocka.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "northbound_mgr.h"
#include "thread_port_uv.h"

/* A5 回归：start 后立刻 stop 的启动竞态、并发 double-stop、驱动建不起来、
 * restart。旧实现先发布 mgr->loop 再初始化 stop_async，且两个 stop 会 join
 * 同一个 uv_thread_t 两次；这里全部必须安全返回。 */

typedef struct {
    pthread_mutex_t mu;
    int publishes;
    int encodes;
    char last_point[128];
    pthread_t last_thread;
} fake_t;

static fake_t g_fake;
static int g_fail_attach;

static void fake_reset(void) {
    pthread_mutex_lock(&g_fake.mu);
    g_fake.publishes = 0;
    g_fake.encodes = 0;
    g_fake.last_point[0] = '\0';
    pthread_mutex_unlock(&g_fake.mu);
}

static int fake_publish(void *ctx, const char *topic, const uint8_t *payload, size_t len) {
    (void)topic;
    (void)payload;
    (void)len;
    fake_t *f = (fake_t *)ctx;
    pthread_mutex_lock(&f->mu);
    f->publishes++;
    f->last_thread = pthread_self();
    pthread_mutex_unlock(&f->mu);
    return 0;
}

static bool fake_is_connected(void *ctx) {
    (void)ctx;
    return true;
}

static int fake_attach(void *ctx, const char *protocol, north_driver_t **out_driver) {
    (void)ctx;
    (void)protocol;
    if (g_fail_attach) return -1;
    static int marker;
    *out_driver = (north_driver_t *)(void *)&marker;
    return 0;
}

static void fake_detach(void *ctx, north_driver_t *driver) {
    (void)ctx;
    (void)driver;
}

static int fake_encode(void *ctx, north_driver_t *driver, uint32_t tag_id, double value,
                       uint64_t ts_us, const char *name, const uint8_t **out_data,
                       size_t *out_len) {
    (void)driver;
    (void)tag_id;
    (void)ts_us;
    static char frame[128];
    fake_t *f = (fake_t *)ctx;
    pthread_mutex_lock(&f->mu);
    f->encodes++;
    snprintf(f->last_point, sizeof(f->last_point), "%s=%g", name ? name : "", value);
    int n = snprintf(frame, sizeof(frame), "%s=%g", name ? name : "", value);
    pthread_mutex_unlock(&f->mu);
    if (n <= 0 || (size_t)n >= sizeof(frame)) return -1;
    *out_data = (const uint8_t *)frame;
    *out_len = (size_t)n;
    return 0;
}

static void mgr_setup(northbound_mgr_t *mgr) {
    pthread_mutex_init(&g_fake.mu, NULL);
    fake_reset();
    nb_source_ops_t source = {0};
    nb_transport_ops_t transport = {
        .ctx = &g_fake,
        .publish = fake_publish,
        .is_connected = fake_is_connected,
    };
    assert_int_equal(northbound_mgr_init_ex(mgr, source, "lifecycle", NULL, transport), 0);
    north_driver_port_t port = {
        .ctx = &g_fake, .attach = fake_attach, .detach = fake_detach, .encode_value = fake_encode};
    northbound_mgr_set_driver_port(mgr, port);
    northbound_mgr_set_address(mgr, "spBv1.0/{group}/{kind}/{node}", "G", "N1");
}

static bool wait_ready(northbound_mgr_t *mgr) {
    for (int i = 0; i < 5000; i++) {
        if (northbound_mgr_publish_result_ack_async(mgr, 1, "READY", NB_RESULT_STATUS_OK) == 0) {
            return true;
        }
        usleep(1000);
    }
    return false;
}

static bool wait_encodes(int want) {
    for (int i = 0; i < 1000; i++) {
        pthread_mutex_lock(&g_fake.mu);
        int n = g_fake.encodes;
        pthread_mutex_unlock(&g_fake.mu);
        if (n >= want) return true;
        usleep(1000);
    }
    return false;
}

/* start 后立刻 stop：stop 可能看到 loop 尚未 ready；线程必须自己收尾。 */
static void test_start_immediate_stop(void **state) {
    (void)state;
    for (int i = 0; i < 100; i++) {
        northbound_mgr_t mgr;
        mgr_setup(&mgr);
        assert_int_equal(northbound_mgr_start(&mgr), 0);
        northbound_mgr_stop(&mgr);
        northbound_mgr_destroy(&mgr);
        pthread_mutex_destroy(&g_fake.mu);
    }
}

/* 两个线程同时 stop：只有认领者 join，另一个立即返回，不许挂死或双 join。 */
static void *stop_thread(void *arg) {
    northbound_mgr_stop((northbound_mgr_t *)arg);
    return NULL;
}

static void test_concurrent_double_stop(void **state) {
    (void)state;
    for (int i = 0; i < 20; i++) {
        northbound_mgr_t mgr;
        mgr_setup(&mgr);
        assert_int_equal(northbound_mgr_start(&mgr), 0);
        assert_true(wait_ready(&mgr));
        assert_true(wait_encodes(1));

        pthread_t t1, t2;
        assert_int_equal(pthread_create(&t1, NULL, stop_thread, &mgr), 0);
        assert_int_equal(pthread_create(&t2, NULL, stop_thread, &mgr), 0);
        assert_int_equal(pthread_join(t1, NULL), 0);
        assert_int_equal(pthread_join(t2, NULL), 0);

        assert_int_equal(
            northbound_mgr_publish_result_ack_async(&mgr, 2, "LATE", NB_RESULT_STATUS_OK), -1);
        northbound_mgr_destroy(&mgr);
        pthread_mutex_destroy(&g_fake.mu);
    }
}

/* stop 在 start 之前是 no-op；顺序 double-stop 幂等；重复 start 返回 -1。 */
static void test_stop_before_start_and_double_stop(void **state) {
    (void)state;
    northbound_mgr_t mgr;
    mgr_setup(&mgr);

    northbound_mgr_stop(&mgr); /* start 之前：no-op */
    assert_int_equal(
        northbound_mgr_publish_result_ack_async(&mgr, 1, "PRE", NB_RESULT_STATUS_OK), -1);

    assert_int_equal(northbound_mgr_start(&mgr), 0);
    assert_int_equal(northbound_mgr_start(&mgr), -1); /* 重复 start */
    assert_true(wait_ready(&mgr));
    northbound_mgr_stop(&mgr);
    northbound_mgr_stop(&mgr); /* 顺序 double-stop：第二次直接返回 */
    assert_int_equal(
        northbound_mgr_publish_result_ack_async(&mgr, 2, "POST", NB_RESULT_STATUS_OK), -1);

    northbound_mgr_destroy(&mgr);
    pthread_mutex_destroy(&g_fake.mu);
}

/* 驱动建不起来：线程提前退出、loop 永不 ready；stop 仍要能 join，投递返回 -1。 */
static void test_driver_attach_failure_still_stops(void **state) {
    (void)state;
    g_fail_attach = 1;
    northbound_mgr_t mgr;
    mgr_setup(&mgr);
    assert_int_equal(northbound_mgr_start(&mgr), 0);
    usleep(20000);
    assert_int_equal(
        northbound_mgr_publish_result_ack_async(&mgr, 1, "ND", NB_RESULT_STATUS_OK), -1);
    northbound_mgr_stop(&mgr);
    northbound_mgr_destroy(&mgr);
    pthread_mutex_destroy(&g_fake.mu);
    g_fail_attach = 0;
}

/* 停后重启：lifecycle 标志复位，回执通道重新可用，旧条目不得重放。 */
static void test_restart_after_stop(void **state) {
    (void)state;
    northbound_mgr_t mgr;
    mgr_setup(&mgr);

    assert_int_equal(northbound_mgr_start(&mgr), 0);
    assert_true(wait_ready(&mgr));
    assert_true(wait_encodes(1));
    northbound_mgr_stop(&mgr);

    int before = 0;
    pthread_mutex_lock(&g_fake.mu);
    before = g_fake.encodes;
    pthread_mutex_unlock(&g_fake.mu);

    assert_int_equal(northbound_mgr_start(&mgr), 0);
    assert_true(wait_ready(&mgr));
    assert_true(wait_encodes(before + 1));
    usleep(100000);
    pthread_mutex_lock(&g_fake.mu);
    int after = g_fake.encodes;
    pthread_mutex_unlock(&g_fake.mu);
    /* 只允许新 READY 一条：stop 时未 drain 的条目在重启时被丢弃。 */
    assert_true(after <= before + 2);

    northbound_mgr_stop(&mgr);
    northbound_mgr_destroy(&mgr);
    pthread_mutex_destroy(&g_fake.mu);
}

int main(void) {
    edge_thread_port_install_uv();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_start_immediate_stop),
        cmocka_unit_test(test_concurrent_double_stop),
        cmocka_unit_test(test_stop_before_start_and_double_stop),
        cmocka_unit_test(test_driver_attach_failure_still_stops),
        cmocka_unit_test(test_restart_after_stop),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
