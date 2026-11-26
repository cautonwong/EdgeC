/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_northbound_downlink_e2e.c
 * @brief P4-03c/P4-03c1: {p,v} 下行从传输回调到控制写、accepted 与结果回执。
 *
 * 链路：mock transport 的 subscribe 回调 → 真 northbound_mgr → 真 IEMS
 * `_on_downlink` → 点位表解析 + 路由/提交桩 → accepted 编码/发布；
 * 提交桩跨线程触发完成回调 → 真 northbound_mgr 结果队列 → 北向 loop → 结果回执。
 *
 * 钉住的语义：
 * - 合法帧只提交一次，bus/slave/protocol 与 route 一致，tag/value 与点表一致；
 * - accepted 回执 `<name>.ack` 只表示 submit_control 的返回值，不是设备执行结果；
 * - 结果回执 `<name>.result`：完成回调（任意线程）→ 值 1=OK / 0=FAILED / 2=TIMEOUT；
 * - 坏帧/未知点名/无唯一路由/submit 失败都不写设备，accepted 为 false；
 * - {p,v} 没有命令 ID：同一帧收到两次就写两次、回两次 accepted，幂等由主站负责；
 * - 结果上下文池满只丢结果回执（accepted 照发），reset/stop 后迟到回调安全丢弃。
 */

#include <cmocka.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "iems_driver_adapter.h"
#include "iems_north_adapter.h"
#include "northbound_mgr.h"
#include "thread_port_uv.h"

#define E2E_TEMPLATE "spBv1.0/{group}/{kind}/{node}"
#define E2E_NCMD_TOPIC "spBv1.0/G/NCMD/N1"
#define E2E_NDATA_TOPIC "spBv1.0/G/NDATA/N1"

typedef void (*nb_on_msg_fn)(void *user, const char *topic, const uint8_t *payload, size_t len);

/* ---- mock 传输：记录订阅与每次 publish 的主题/载荷 ------------------------ */

typedef struct {
    pthread_mutex_t mu;
    int subs;
    char sub_topic[128];
    nb_on_msg_fn on_msg;
    void *on_msg_user;
    int publishes;
    char pub_topics[32][128];
    char pub_payloads[32][128];
    pthread_t pub_threads[32];
} fake_transport_t;

static int fake_publish(void *ctx, const char *topic, const uint8_t *payload, size_t len) {
    fake_transport_t *t = (fake_transport_t *)ctx;
    pthread_mutex_lock(&t->mu);
    int i = t->publishes;
    if (i < 32) {
        snprintf(t->pub_topics[i], sizeof(t->pub_topics[i]), "%s", topic ? topic : "");
        size_t n = len < sizeof(t->pub_payloads[i]) - 1 ? len : sizeof(t->pub_payloads[i]) - 1;
        if (payload && n) memcpy(t->pub_payloads[i], payload, n);
        t->pub_payloads[i][n] = '\0';
        t->pub_threads[i] = pthread_self();
    }
    t->publishes++;
    pthread_mutex_unlock(&t->mu);
    return 0;
}

static int fake_subscribe(void *ctx, const char *topic, nb_on_msg_fn on_msg, void *user) {
    fake_transport_t *t = (fake_transport_t *)ctx;
    pthread_mutex_lock(&t->mu);
    t->subs++;
    snprintf(t->sub_topic, sizeof(t->sub_topic), "%s", topic ? topic : "");
    t->on_msg = on_msg;
    t->on_msg_user = user;
    pthread_mutex_unlock(&t->mu);
    return 0;
}

static bool fake_is_connected(void *ctx) {
    (void)ctx;
    return true;
}

/* ---- mock 驱动端口：把 (name, value) 编成可断言的 "name=value" ---------- */

static int g_driver_marker;

typedef struct {
    int values;
    uint32_t last_tag;
    double last_value;
    char last_name[96];
    char frame[128];
} fake_driver_t;

static int fake_attach(void *ctx, const char *protocol, north_driver_t **out_driver) {
    (void)ctx;
    (void)protocol;
    *out_driver = (north_driver_t *)(void *)&g_driver_marker;
    return 0;
}

static int fake_encode_value(void *ctx, north_driver_t *driver, uint32_t tag_id, double value,
                             uint64_t ts_us, const char *name, const uint8_t **out_data,
                             size_t *out_len) {
    (void)driver;
    (void)ts_us;
    fake_driver_t *d = (fake_driver_t *)ctx;
    d->values++;
    d->last_tag = tag_id;
    d->last_value = value;
    snprintf(d->last_name, sizeof(d->last_name), "%s", name ? name : "");
    int n = snprintf(d->frame, sizeof(d->frame), "%s=%g", name ? name : "", value);
    if (n < 0 || (size_t)n >= sizeof(d->frame)) return -1;
    *out_data = (const uint8_t *)d->frame;
    *out_len = (size_t)n;
    return 0;
}

/* ---- rig：真 mgr + 真 IEMS 下行适配器 + 可控路由/提交 ------------------- */

#define RIG_MAX_CB 32

typedef struct {
    fake_transport_t bus;
    fake_driver_t drv;
    northbound_mgr_t mgr;
    iems_north_ctx_t ctx;
    int route_calls;
    uint32_t route_tag;
    int route_rc;
    iems_device_route_t route;
    int submit_calls;
    uint32_t submit_tag;
    double submit_value;
    iems_device_route_t submit_route;
    int submit_rc;
    iems_write_done_fn submit_cb[RIG_MAX_CB];
    void *submit_cb_user[RIG_MAX_CB];
    int submit_cb_count;
    nb_on_msg_fn downlink;
    void *downlink_user;
} rig_t;

static rig_t g_rig;
static bool g_started;

static int rig_route(void *ctx, uint32_t tag_id, iems_device_route_t *route) {
    rig_t *r = (rig_t *)ctx;
    r->route_calls++;
    r->route_tag = tag_id;
    if (r->route_rc != 0) return r->route_rc;
    *route = r->route;
    return 0;
}

static int rig_submit(void *ctx, const iems_device_route_t *route, uint32_t tag_id, double value,
                      iems_write_done_fn on_done, void *on_done_user) {
    rig_t *r = (rig_t *)ctx;
    if (r->submit_cb_count < RIG_MAX_CB) {
        r->submit_cb[r->submit_cb_count] = on_done;
        r->submit_cb_user[r->submit_cb_count] = on_done_user;
        r->submit_cb_count++;
    }
    r->submit_calls++;
    r->submit_tag = tag_id;
    r->submit_value = value;
    if (route) r->submit_route = *route;
    return r->submit_rc;
}

static int fixture_setup(void **state) {
    (void)state;
    memset(&g_rig, 0, sizeof(g_rig));
    pthread_mutex_init(&g_rig.bus.mu, NULL);
    g_started = false;

    g_rig.route.bus_id = 3;
    g_rig.route.slave_id = 7;
    snprintf(g_rig.route.protocol, sizeof(g_rig.route.protocol), "%s", "ModbusRTU");
    g_rig.route_rc = 0;
    g_rig.submit_rc = 0;

    nb_source_ops_t source = {0};
    nb_transport_ops_t transport = {
        .ctx = &g_rig.bus,
        .publish = fake_publish,
        .subscribe = fake_subscribe,
        .is_connected = fake_is_connected,
    };
    assert_int_equal(northbound_mgr_init_ex(&g_rig.mgr, source, "downlink_e2e", NULL, transport),
                     0);

    north_driver_port_t port = {
        .ctx = &g_rig.drv,
        .attach = fake_attach,
        .encode_value = fake_encode_value,
    };
    northbound_mgr_set_driver_port(&g_rig.mgr, port);
    northbound_mgr_set_address(&g_rig.mgr, E2E_TEMPLATE, "G", "N1");

    g_rig.ctx.north = &g_rig.mgr;
    g_rig.ctx.submit_control = rig_submit;
    g_rig.ctx.submit_ctx = &g_rig;
    g_rig.ctx.route_for_tag = rig_route;
    g_rig.ctx.route_ctx = &g_rig;
    iems_north_results_reset(&g_rig.ctx); /* 初始化结果上下文池（清零后第一次认领前） */
    g_rig.ctx.north = &g_rig.mgr;
    northbound_mgr_set_north_source(&g_rig.mgr, iems_north_source_ops(), &g_rig.ctx);
    return 0;
}

static int fixture_teardown(void **state) {
    (void)state;
    /* 与组合根 destroy 同序：先断结果上下文，再停/销毁管理器。 */
    iems_north_results_reset(&g_rig.ctx);
    if (g_started) {
        northbound_mgr_stop(&g_rig.mgr);
        g_started = false;
    }
    northbound_mgr_destroy(&g_rig.mgr);
    pthread_mutex_destroy(&g_rig.bus.mu);
    return 0;
}

/* 下行订阅在 loop 的 500ms 兜底 poll 里惰性建立。 */
static void rig_start(rig_t *r) {
    assert_int_equal(northbound_mgr_start(&r->mgr), 0);
    g_started = true;

    bool subscribed = false;
    for (int i = 0; i < 250 && !subscribed; i++) {
        pthread_mutex_lock(&r->bus.mu);
        subscribed = r->bus.subs == 1;
        pthread_mutex_unlock(&r->bus.mu);
        if (!subscribed) usleep(20000);
    }
    assert_true(subscribed);

    char topic[128];
    pthread_mutex_lock(&r->bus.mu);
    snprintf(topic, sizeof(topic), "%s", r->bus.sub_topic);
    r->downlink = r->bus.on_msg;
    r->downlink_user = r->bus.on_msg_user;
    pthread_mutex_unlock(&r->bus.mu);
    assert_non_null(r->downlink);
    assert_string_equal(topic, E2E_NCMD_TOPIC);
}

static void rig_arm(rig_t *r, int route_rc, int submit_rc) {
    r->route_calls = 0;
    r->route_tag = 0;
    r->route_rc = route_rc;
    r->submit_calls = 0;
    r->submit_tag = 0;
    r->submit_value = 0.0;
    memset(&r->submit_route, 0, sizeof(r->submit_route));
    r->submit_rc = submit_rc;
    memset(r->submit_cb, 0, sizeof(r->submit_cb));
    memset(r->submit_cb_user, 0, sizeof(r->submit_cb_user));
    r->submit_cb_count = 0;
    memset(&r->drv, 0, sizeof(r->drv));
    pthread_mutex_lock(&r->bus.mu);
    r->bus.publishes = 0;
    memset(r->bus.pub_topics, 0, sizeof(r->bus.pub_topics));
    memset(r->bus.pub_payloads, 0, sizeof(r->bus.pub_payloads));
    pthread_mutex_unlock(&r->bus.mu);
}

/* 直接调用捕获到的传输回调，与 Paho 回调线程投递下行帧的语义一致。 */
static void rig_inject(rig_t *r, const char *body) {
    assert_non_null(r->downlink);
    r->downlink(r->downlink_user, E2E_NCMD_TOPIC, (const uint8_t *)body, strlen(body));
}

typedef struct {
    int count;
    char topics[32][128];
    char payloads[32][128];
    pthread_t threads[32];
} publish_snapshot_t;

static publish_snapshot_t rig_snapshot(rig_t *r) {
    publish_snapshot_t s;
    memset(&s, 0, sizeof(s));
    pthread_mutex_lock(&r->bus.mu);
    s.count = r->bus.publishes;
    memcpy(s.topics, r->bus.pub_topics, sizeof(s.topics));
    memcpy(s.payloads, r->bus.pub_payloads, sizeof(s.payloads));
    memcpy(s.threads, r->bus.pub_threads, sizeof(s.threads));
    pthread_mutex_unlock(&r->bus.mu);
    return s;
}

static bool rig_snapshot_has(const publish_snapshot_t *s, const char *payload) {
    int n = s->count < 32 ? s->count : 32;
    for (int i = 0; i < n; i++) {
        if (strcmp(s->payloads[i], payload) == 0) return true;
    }
    return false;
}

/* 编码/发布只允许发生在北向 loop 线程：回执不该由注入线程（本测试线程）发布。 */
static bool rig_snapshot_off_thread(const publish_snapshot_t *s, pthread_t thread) {
    int n = s->count < 32 ? s->count : 32;
    for (int i = 0; i < n; i++) {
        if (pthread_equal(s->threads[i], thread)) return false;
    }
    return true;
}

/* 结果回执由北向 loop 线程异步发布：等到预期条数再断言。 */
static bool rig_wait_publishes(rig_t *r, int want) {
    for (int i = 0; i < 500; i++) {
        pthread_mutex_lock(&r->bus.mu);
        int count = r->bus.publishes;
        pthread_mutex_unlock(&r->bus.mu);
        if (count >= want) return true;
        usleep(10000);
    }
    return false;
}

/* 在独立线程触发执行完成回调，模拟总线/IO 线程（真实回调不在北向 loop）。 */
typedef struct {
    iems_write_done_fn fn;
    void *user;
    iems_write_done_status_t status;
} done_trigger_t;

static void *trigger_done_thread(void *arg) {
    done_trigger_t *t = (done_trigger_t *)arg;
    t->fn(t->user, t->status);
    return NULL;
}

static void rig_fire_done(rig_t *r, int index, iems_write_done_status_t status) {
    assert_true(index >= 0 && index < r->submit_cb_count);
    assert_non_null(r->submit_cb[index]);
    done_trigger_t t = {
        .fn = r->submit_cb[index], .user = r->submit_cb_user[index], .status = status};
    pthread_t th;
    assert_int_equal(pthread_create(&th, NULL, trigger_done_thread, &t), 0);
    assert_int_equal(pthread_join(th, NULL), 0);
}

/* ---- 用例 ---------------------------------------------------------------- */

static void test_valid_frame_submits_once_and_acks_accepted(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");

    assert_int_equal(r->route_calls, 1);
    assert_int_equal(r->route_tag, (uint32_t)IEMS_TAG_BATTERY_SOC);
    assert_int_equal(r->submit_calls, 1);
    assert_int_equal(r->submit_tag, (uint32_t)IEMS_TAG_BATTERY_SOC);
    assert_true(r->submit_value == 55.0);
    assert_int_equal(r->submit_route.bus_id, 3);
    assert_int_equal(r->submit_route.slave_id, 7);
    assert_string_equal(r->submit_route.protocol, "ModbusRTU");

    /* accepted 也走异步回执队列：等北向 loop 发布后再断言（编码不在注入线程）。 */
    assert_true(rig_wait_publishes(r, 1));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 1);
    assert_string_equal(s.topics[0], E2E_NDATA_TOPIC);
    assert_string_equal(s.payloads[0], "BATTERY_SOC.ack=1");
    assert_true(rig_snapshot_off_thread(&s, pthread_self()));
    assert_int_equal(r->drv.last_tag, IEMS_TAG_BATTERY_SOC);
}

static void test_bad_frame_rejects_with_ack_0(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "not a {p,v} frame");

    assert_int_equal(r->route_calls, 0);
    assert_int_equal(r->submit_calls, 0);
    assert_true(rig_wait_publishes(r, 1));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 1);
    assert_string_equal(s.topics[0], E2E_NDATA_TOPIC);
    assert_string_equal(s.payloads[0], "ack.0=0");
    assert_true(rig_snapshot_off_thread(&s, pthread_self()));
    assert_int_equal(r->drv.last_tag, 0);
}

static void test_unknown_name_rejects_with_original_name(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "{\"p\":\"NO_SUCH_POINT\",\"v\":1}");

    assert_int_equal(r->route_calls, 0);
    assert_int_equal(r->submit_calls, 0);
    assert_true(rig_wait_publishes(r, 1));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 1);
    assert_string_equal(s.payloads[0], "NO_SUCH_POINT.ack=0");
    assert_true(rig_snapshot_off_thread(&s, pthread_self()));
    assert_int_equal(r->drv.last_tag, 0);
}

static void test_no_unique_route_rejects_with_tag_and_name(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, -1, 0);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");

    assert_int_equal(r->route_calls, 1);
    assert_int_equal(r->route_tag, (uint32_t)IEMS_TAG_BATTERY_SOC);
    assert_int_equal(r->submit_calls, 0);
    assert_true(rig_wait_publishes(r, 1));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 1);
    assert_string_equal(s.payloads[0], "BATTERY_SOC.ack=0");
    assert_true(rig_snapshot_off_thread(&s, pthread_self()));
    assert_int_equal(r->drv.last_tag, IEMS_TAG_BATTERY_SOC);
}

static void test_submit_failure_acks_false(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, -1);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");

    assert_int_equal(r->route_calls, 1);
    assert_int_equal(r->submit_calls, 1);
    assert_true(rig_wait_publishes(r, 1));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 1);
    assert_string_equal(s.payloads[0], "BATTERY_SOC.ack=0");
    assert_true(rig_snapshot_off_thread(&s, pthread_self()));
    assert_int_equal(r->drv.last_tag, IEMS_TAG_BATTERY_SOC);
}

/* 协议没有命令 ID：每一帧合法下行都执行一次控制写、回一次 ack。 */
static void test_duplicate_frame_writes_and_acks_each_time(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");
    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");

    assert_int_equal(r->route_calls, 2);
    assert_int_equal(r->submit_calls, 2);
    assert_true(rig_wait_publishes(r, 2));
    assert_int_equal(r->drv.values, 2);
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 2);
    assert_string_equal(s.topics[0], E2E_NDATA_TOPIC);
    assert_string_equal(s.topics[1], E2E_NDATA_TOPIC);
    assert_string_equal(s.payloads[0], "BATTERY_SOC.ack=1");
    assert_string_equal(s.payloads[1], "BATTERY_SOC.ack=1");
    assert_true(rig_snapshot_off_thread(&s, pthread_self()));
}

/* ---- 结果回执（P4-03c1）：完成回调在独立线程触发，回投北向 loop 后发布 ---- */

static void test_result_ack_success(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");
    assert_int_equal(r->submit_cb_count, 1);
    assert_true(rig_wait_publishes(r, 1));
    publish_snapshot_t accepted = rig_snapshot(r);
    assert_int_equal(accepted.count, 1);
    assert_string_equal(accepted.payloads[0], "BATTERY_SOC.ack=1");

    rig_fire_done(r, 0, IEMS_WRITE_DONE_OK);
    assert_true(rig_wait_publishes(r, 2));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 2);
    assert_string_equal(s.topics[1], E2E_NDATA_TOPIC);
    assert_string_equal(s.payloads[1], "BATTERY_SOC.result=1");
    assert_true(rig_snapshot_off_thread(&s, pthread_self()));
    assert_int_equal(r->drv.last_tag, IEMS_TAG_BATTERY_SOC);
    assert_string_equal(r->drv.last_name, "BATTERY_SOC.result");
}

static void test_result_ack_failure(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");
    assert_int_equal(r->submit_cb_count, 1);
    assert_true(rig_wait_publishes(r, 1));

    rig_fire_done(r, 0, IEMS_WRITE_DONE_FAILED);
    assert_true(rig_wait_publishes(r, 2));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 2);
    assert_string_equal(s.topics[1], E2E_NDATA_TOPIC);
    assert_string_equal(s.payloads[1], "BATTERY_SOC.result=0");
}

static void test_result_ack_timeout(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");
    assert_int_equal(r->submit_cb_count, 1);
    assert_true(rig_wait_publishes(r, 1));

    rig_fire_done(r, 0, IEMS_WRITE_DONE_TIMEOUT);
    assert_true(rig_wait_publishes(r, 2));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 2);
    assert_string_equal(s.topics[1], E2E_NDATA_TOPIC);
    assert_string_equal(s.payloads[1], "BATTERY_SOC.result=2");
}

/* 两条不同点位的命令各持自己的上下文：反序完成也要落到各自的名字上。 */
static void test_two_commands_results_do_not_cross(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");
    rig_inject(r, "{\"p\":\"GRID_POWER\",\"v\":12}");
    assert_int_equal(r->submit_cb_count, 2);
    assert_int_equal(r->submit_tag, (uint32_t)IEMS_TAG_GRID_POWER);

    assert_true(rig_wait_publishes(r, 2));
    publish_snapshot_t accepted = rig_snapshot(r);
    assert_int_equal(accepted.count, 2);
    assert_string_equal(accepted.payloads[0], "BATTERY_SOC.ack=1");
    assert_string_equal(accepted.payloads[1], "GRID_POWER.ack=1");

    /* 后提交的先完成（成功），先提交的后完成（超时） */
    rig_fire_done(r, 1, IEMS_WRITE_DONE_OK);
    rig_fire_done(r, 0, IEMS_WRITE_DONE_TIMEOUT);
    assert_true(rig_wait_publishes(r, 4));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 4);
    assert_true(rig_snapshot_has(&s, "BATTERY_SOC.result=2"));
    assert_true(rig_snapshot_has(&s, "GRID_POWER.result=1"));
    assert_true(rig_snapshot_off_thread(&s, pthread_self()));
}

/* 结果上下文池满：每条命令仍有 accepted 回执；溢出命令提交时拿到 NULL 回调；
 * 已完成的那条仍能发出结果回执，未完成的不产生任何回执。 */
static void test_result_pool_full_drops_result_only(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    const int overflow = IEMS_NORTH_RESULT_SLOTS + 1;
    for (int i = 0; i < overflow; i++) {
        rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");
        /* 回执队列也是定长 16：逐条等 loop 发布，模拟现场逐条下行，避免测试线程
         * 瞬间灌满队列而丢 accepted（队列满的行为另由 mgr-lifecycle 用例覆盖）。 */
        assert_true(rig_wait_publishes(r, i + 1));
    }
    assert_int_equal(r->submit_calls, overflow);
    assert_int_equal(r->submit_cb_count, overflow);
    for (int i = 0; i < IEMS_NORTH_RESULT_SLOTS; i++) {
        assert_non_null(r->submit_cb[i]);
    }
    assert_null(r->submit_cb[IEMS_NORTH_RESULT_SLOTS]);

    publish_snapshot_t accepted = rig_snapshot(r);
    assert_int_equal(accepted.count, overflow);
    for (int i = 0; i < overflow; i++) {
        assert_string_equal(accepted.payloads[i], "BATTERY_SOC.ack=1");
    }

    rig_fire_done(r, 0, IEMS_WRITE_DONE_OK);
    assert_true(rig_wait_publishes(r, overflow + 1));
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, overflow + 1);
    assert_string_equal(s.payloads[overflow], "BATTERY_SOC.result=1");
}

/* reset（组合根 destroy 路径）后管理器停止，再触发迟到的完成回调：不崩、不再发布。 */
static void test_late_completion_after_reset_is_safe(void **state) {
    (void)state;
    rig_t *r = &g_rig;
    rig_start(r);
    rig_arm(r, 0, 0);

    rig_inject(r, "{\"p\":\"BATTERY_SOC\",\"v\":55}");
    assert_int_equal(r->submit_cb_count, 1);
    assert_true(rig_wait_publishes(r, 1));
    iems_write_done_fn fn = r->submit_cb[0];
    void *user = r->submit_cb_user[0];
    assert_non_null(fn);

    iems_north_results_reset(&r->ctx);
    northbound_mgr_stop(&r->mgr);
    g_started = false;

    /* 停止后的投递必须被拒绝，且不触碰已关闭的 async handle（ASAN 下跑）。 */
    assert_int_equal(northbound_mgr_publish_result_ack_async(&r->mgr, IEMS_TAG_BATTERY_SOC,
                                                             "BATTERY_SOC", IEMS_WRITE_DONE_OK),
                     -1);

    /* 真正走适配器的完成回调（不是只测 API 返回值）：槽已失效，必须安全丢弃。 */
    done_trigger_t t = {.fn = fn, .user = user, .status = IEMS_WRITE_DONE_OK};
    pthread_t th;
    assert_int_equal(pthread_create(&th, NULL, trigger_done_thread, &t), 0);
    assert_int_equal(pthread_join(th, NULL), 0);

    /* 给（本应被丢弃的）异步发布留出时间，确认没有新帧 */
    usleep(100000);
    publish_snapshot_t s = rig_snapshot(r);
    assert_int_equal(s.count, 1);
    assert_string_equal(s.payloads[0], "BATTERY_SOC.ack=1");
}

/* ---- reset 与在途回调重叠：回调已拷贝槽、停在回执队列锁上时 reset，再撤掉 ctx 内存。
 * 新实现里回调用锁内拷贝的数据投递，reset 后不再解引用 ctx；旧实现会在恢复后写
 * 已释放的槽（SIGSEGV）。子进程跑，父进程只断言干净退出。 ---- */

typedef struct {
    iems_write_done_fn fn;
    void *user;
} uaf_capture_t;

static uaf_capture_t g_uaf_cap;

static int _uaf_route(void *ctx, uint32_t tag_id, iems_device_route_t *route) {
    (void)ctx;
    (void)tag_id;
    memset(route, 0, sizeof(*route));
    route->bus_id = 1;
    route->slave_id = 1;
    snprintf(route->protocol, sizeof(route->protocol), "%s", "ModbusRTU");
    return 0;
}

static int _uaf_submit(void *ctx, const iems_device_route_t *route, uint32_t tag_id,
                       double value, iems_write_done_fn on_done, void *on_done_user) {
    (void)ctx;
    (void)route;
    (void)tag_id;
    (void)value;
    g_uaf_cap.fn = on_done;
    g_uaf_cap.user = on_done_user;
    return 0; /* accepted；完成回调由测试线程触发 */
}

static void _uaf_segv(int sig, siginfo_t *si, void *uc) {
    (void)sig;
    (void)si;
    (void)uc;
    _exit(42); /* 旧实现：回调在 reset 后仍写 ctx 槽 */
}

static void *_uaf_fire(void *arg) {
    uaf_capture_t *cap = (uaf_capture_t *)arg;
    cap->fn(cap->user, IEMS_WRITE_DONE_OK);
    return NULL;
}

static void _uaf_child_run(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = _uaf_segv;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    void *page = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) _exit(2);
    iems_north_ctx_t *ctx = (iems_north_ctx_t *)page;
    memset(ctx, 0, sizeof(*ctx));

    northbound_mgr_t mgr;
    nb_source_ops_t src = {0};
    if (northbound_mgr_init_ex(&mgr, src, "uaf", NULL, (nb_transport_ops_t){0}) != 0) _exit(3);
    iems_north_results_reset(ctx);
    atomic_store(&ctx->north, &mgr);
    ctx->route_for_tag = _uaf_route;
    ctx->submit_control = _uaf_submit;

    const char *frame = "{\"p\":\"BATTERY_SOC\",\"v\":55}";
    if (iems_north_source_ops()->on_downlink(ctx, "spBv1.0/G/NCMD/N1", (const uint8_t *)frame,
                                             strlen(frame)) != 0)
        _exit(4);
    if (!g_uaf_cap.fn) _exit(5);

    /* 回调先拷贝槽并归还，然后停在回执队列锁上；此时 reset 清槽并释放 ctx。 */
    uv_mutex_lock(&mgr.receipts.lock);
    uaf_capture_t cap = g_uaf_cap;
    pthread_t th;
    if (pthread_create(&th, NULL, _uaf_fire, &cap) != 0) _exit(6);
    usleep(100000);

    iems_north_results_reset(ctx);
    if (mprotect(page, 4096, PROT_NONE) != 0) _exit(7);
    uv_mutex_unlock(&mgr.receipts.lock); /* 回调恢复：不得再碰 ctx */
    pthread_join(th, NULL);

    northbound_mgr_destroy(&mgr);
    _exit(0);
}

static void test_reset_races_inflight_callback_safely(void **state) {
    (void)state;
    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) _uaf_child_run();
    int status = 0;
    assert_int_equal(waitpid(pid, &status, 0), pid);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);
}

int main(void) {
    edge_thread_port_install_uv();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_valid_frame_submits_once_and_acks_accepted,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_bad_frame_rejects_with_ack_0, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_unknown_name_rejects_with_original_name, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_no_unique_route_rejects_with_tag_and_name,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_submit_failure_acks_false, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_duplicate_frame_writes_and_acks_each_time,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_result_ack_success, fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_result_ack_failure, fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_result_ack_timeout, fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_two_commands_results_do_not_cross, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_result_pool_full_drops_result_only, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_late_completion_after_reset_is_safe, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test(test_reset_races_inflight_callback_safely),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
