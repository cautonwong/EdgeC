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
 * 直接失败。这里把失败点钉住，扩容或按应用配额改变行为时先改测试与迁移决策。 */

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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_subscription_capacity_is_64_per_hub),
        cmocka_unit_test(test_two_apps_share_the_same_64_slots),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
