#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "datahub.h"
#include "datahub_priv.h"

/* P1-06 的容量基线：一个 DataHub 的订阅表只有 DATAHUB_MAX_SUBSCRIBERS 个全局
 * 槽位，按 hub 共享，不是按应用。两个 40 点应用只能拿到 40+24，第 65 个订阅
 * 直接失败；三应用 30+20+20 在第三个应用的第 15 个订阅失败，五应用 16×5 让
 * 第五个应用拿到 0 个。这里把失败点钉住，扩容或按应用配额改变行为时先改测试
 * 与迁移决策。 */

typedef struct {
    int calls;
} cb_ctx_t;

static void on_event(datahub_t *hub, const pipe_msg_t *msg, void *user_ctx) {
    (void)hub;
    (void)msg;
    cb_ctx_t *ctx = (cb_ctx_t *)user_ctx;
    if (ctx) ctx->calls++;
}

static int publish(datahub_t *hub, uint32_t tag) {
    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.tag_id = tag;
    msg.device_id = 0;
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.payload.val.f = (double)tag;
    return datahub_write_shadow(hub, &msg);
}

static void init_hub(datahub_t *hub, const char *prefix) {
    char name[64];
    snprintf(name, sizeof(name), "%s_%d", prefix, (int)getpid());
    assert_int_equal(datahub_init(hub, NULL, name, 64, 1024, NULL, 0, NULL, NULL, NULL), 0);
}

/* 按 tag 顺序注册，返回成功数；失败的下标记录负返回码，便于断言首个失败点。 */
static int subscribe_range(datahub_t *hub, uint32_t base, int count, cb_ctx_t *ctx, int *ids) {
    int ok = 0;
    for (int i = 0; i < count; i++) {
        ids[i] = datahub_subscribe_tag(hub, base + (uint32_t)i, on_event, &ctx[i]);
        if (ids[i] > 0) ok++;
    }
    return ok;
}

static int call_count(const cb_ctx_t *ctx, int count) {
    int calls = 0;
    for (int i = 0; i < count; i++) calls += ctx[i].calls;
    return calls;
}

static void test_subscription_capacity_is_64_per_hub(void **state) {
    (void)state;
    char name[64];
    snprintf(name, sizeof(name), "test_sub_capacity_%d", (int)getpid());
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, NULL, name, 64, 1024, NULL, 0, NULL, NULL, NULL), 0);

    cb_ctx_t ctx[DATAHUB_MAX_SUBSCRIBERS];
    memset(ctx, 0, sizeof(ctx));
    int ids[DATAHUB_MAX_SUBSCRIBERS];
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) {
        ids[i] = datahub_subscribe_tag(&hub, 41, on_event, &ctx[i]);
        assert_true(ids[i] > 0);
    }

    cb_ctx_t overflow;
    memset(&overflow, 0, sizeof(overflow));
    assert_true(datahub_subscribe_tag(&hub, 41, on_event, &overflow) <= 0);

    assert_int_equal(publish(&hub, 41), 0);
    int calls = 0;
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) calls += ctx[i].calls;
    assert_int_equal(calls, DATAHUB_MAX_SUBSCRIBERS);
    assert_int_equal(overflow.calls, 0);

    /* 释放一个槽位后新订阅立即成功，容量是槽位而不是永久拒绝 */
    assert_int_equal(datahub_unsubscribe(&hub, ids[0]), 0);
    cb_ctx_t late;
    memset(&late, 0, sizeof(late));
    assert_true(datahub_subscribe_tag(&hub, 41, on_event, &late) > 0);

    datahub_destroy(&hub);
}

static void test_two_apps_share_the_same_64_slots(void **state) {
    (void)state;
    char name[64];
    snprintf(name, sizeof(name), "test_sub_two_apps_%d", (int)getpid());
    datahub_t hub;
    assert_int_equal(datahub_init(&hub, NULL, name, 64, 1024, NULL, 0, NULL, NULL, NULL), 0);

    cb_ctx_t app_a[40];
    cb_ctx_t app_b[40];
    memset(app_a, 0, sizeof(app_a));
    memset(app_b, 0, sizeof(app_b));

    int a_ok = 0;
    for (int i = 0; i < 40; i++) {
        int id = datahub_subscribe_tag(&hub, (uint32_t)(100 + i), on_event, &app_a[i]);
        if (id > 0) a_ok++;
    }
    assert_int_equal(a_ok, 40);

    int b_ok = 0;
    for (int i = 0; i < 40; i++) {
        int id = datahub_subscribe_tag(&hub, (uint32_t)(200 + i), on_event, &app_b[i]);
        if (id > 0) b_ok++;
    }
    /* 40 + 24 = 64：第二个应用只能拿到剩余 24 个槽位 */
    assert_int_equal(b_ok, DATAHUB_MAX_SUBSCRIBERS - 40);

    /* 点位隔离仍然成立：A 的点只回调 A，B 的点只回调已订阅到的 B */
    assert_int_equal(publish(&hub, 100), 0);
    assert_int_equal(app_a[0].calls, 1);
    int b_calls = 0;
    for (int i = 0; i < 40; i++) b_calls += app_b[i].calls;
    assert_int_equal(b_calls, 0);

    assert_int_equal(publish(&hub, 200), 0);
    assert_int_equal(app_b[0].calls, 1);
    assert_int_equal(app_a[0].calls, 1);

    datahub_destroy(&hub);
}

static void test_three_apps_split_exhausts_the_table(void **state) {
    (void)state;
    datahub_t hub;
    init_hub(&hub, "test_sub_three_apps");

    cb_ctx_t app_a[30];
    cb_ctx_t app_b[20];
    cb_ctx_t app_c[20];
    int a_ids[30];
    int b_ids[20];
    int c_ids[20];
    memset(app_a, 0, sizeof(app_a));
    memset(app_b, 0, sizeof(app_b));
    memset(app_c, 0, sizeof(app_c));

    assert_int_equal(subscribe_range(&hub, 300, 30, app_a, a_ids), 30);
    assert_int_equal(subscribe_range(&hub, 400, 20, app_b, b_ids), 20);
    /* 30 + 20 = 50：第三个应用只拿到剩余 14 个槽，第 15 个（tag 514）失败 */
    assert_int_equal(subscribe_range(&hub, 500, 20, app_c, c_ids), DATAHUB_MAX_SUBSCRIBERS - 50);
    assert_true(c_ids[13] > 0);
    assert_true(c_ids[14] <= 0);

    assert_int_equal(publish(&hub, 300), 0);
    assert_int_equal(publish(&hub, 400), 0);
    assert_int_equal(publish(&hub, 500), 0);
    assert_int_equal(publish(&hub, 513), 0);
    assert_int_equal(publish(&hub, 514), 0);

    /* 先注册的订阅照常收数，失败的那个既不占槽也不回调 */
    assert_int_equal(app_a[0].calls, 1);
    assert_int_equal(app_b[0].calls, 1);
    assert_int_equal(app_c[0].calls, 1);
    assert_int_equal(app_c[13].calls, 1);
    assert_int_equal(app_c[14].calls, 0);
    assert_int_equal(call_count(app_c, 20), 2);

    datahub_destroy(&hub);
}

static void test_five_apps_leave_the_last_app_with_zero_slots(void **state) {
    (void)state;
    datahub_t hub;
    init_hub(&hub, "test_sub_five_apps");

    cb_ctx_t app[5][16];
    int ids[5][16];
    memset(app, 0, sizeof(app));

    for (int i = 0; i < 4; i++) {
        assert_int_equal(subscribe_range(&hub, (uint32_t)(600 + 100 * i), 16, app[i], ids[i]), 16);
    }
    /* 4 × 16 = 64：第五个应用一个槽也拿不到，首个订阅（tag 1000）即失败 */
    assert_int_equal(subscribe_range(&hub, 1000, 16, app[4], ids[4]), DATAHUB_MAX_SUBSCRIBERS - 64);
    assert_true(ids[4][0] <= 0);

    assert_int_equal(publish(&hub, 600), 0);
    assert_int_equal(publish(&hub, 900), 0);
    assert_int_equal(publish(&hub, 1000), 0);

    /* 前四个应用不受第五个应用失败的影响 */
    assert_int_equal(app[0][0].calls, 1);
    assert_int_equal(app[3][0].calls, 1);
    assert_int_equal(app[4][0].calls, 0);

    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_subscription_capacity_is_64_per_hub),
        cmocka_unit_test(test_two_apps_share_the_same_64_slots),
        cmocka_unit_test(test_three_apps_split_exhausts_the_table),
        cmocka_unit_test(test_five_apps_leave_the_last_app_with_zero_slots),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
