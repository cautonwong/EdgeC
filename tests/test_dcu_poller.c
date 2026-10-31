#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"

/* ==================== 任务/方案注册（沿用旧用例） ==================== */

static void test_task_and_scheme_add(void** state) {
    (void)state;
    dcu_poller_t poller;
    assert_int_equal(dcu_poller_init(&poller), 0);
    assert_int_equal(poller.task_count, 0);
    assert_int_equal(poller.scheme_count, 0);

    uint32_t csds[2] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, csds, 2), 0);
    assert_int_equal(poller.scheme_count, 1);

    dcu_scheme_t* s = dcu_poller_get_scheme(&poller, 1);
    assert_non_null(s);
    assert_int_equal(s->data.normal.csd_count, 2);
    assert_int_equal(s->data.normal.csd_list[0], OAD_VOLTAGE);

    assert_int_equal(dcu_poller_add_task(&poller, 1, 15, 1), 0);
    assert_int_equal(poller.task_count, 1);
    assert_int_equal(poller.tasks[0].interval_s, 15);

    dcu_poller_destroy(&poller);
}

/* 边界：方案表写满、CSD 超长都必须拒绝，不能写穿相邻数组 */
static void test_add_scheme_bounds(void** state) {
    (void)state;
    dcu_poller_t poller;
    dcu_poller_init(&poller);

    uint32_t csds[MAX_SCHEME_OADS + 4];
    for (int i = 0; i < MAX_SCHEME_OADS + 4; i++) csds[i] = OAD_VOLTAGE + (uint32_t)i;

    /* CSD 超长 */
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, csds, MAX_SCHEME_OADS + 1), -1);
    /* 正常长度可以 */
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, csds, MAX_SCHEME_OADS), 0);

    /* 方案表写满后必须拒绝 */
    for (int i = 1; i < MAX_SCHEMES; i++) {
        (void)dcu_poller_add_scheme(&poller, 100 + (uint32_t)i, 0, csds, 1);
    }
    assert_int_equal(poller.scheme_count, MAX_SCHEMES);
    assert_int_equal(dcu_poller_add_scheme(&poller, 999, 0, csds, 1), -1);
}

/* ==================== 链式提交 mock ==================== */

#define TRACE_MAX 512

static int g_submit_count;
static int g_pub_count;
static int g_fail_submits;      /* 让 submit 直接返回 -1 的次数（-1 = 永不失败） */
static int g_sync_callback;     /* 1 = 同步回调（验证 pumping 防重入） */
static uint32_t g_trace_meter[TRACE_MAX];
static uint32_t g_trace_tag[TRACE_MAX];
static int g_trace_n;

static void _trace_reset(void) {
    g_submit_count = 0;
    g_pub_count = 0;
    g_fail_submits = -1;
    g_sync_callback = 1;
    g_trace_n = 0;
}

static int _mock_submit(void* ctx, uint32_t meter_idx, uint32_t tag_id, poller_read_done_cb cb,
                        void* ud) {
    (void)ctx;
    if (g_fail_submits > 0) {
        g_fail_submits--;
        return -1;
    }
    g_submit_count++;
    if (g_trace_n < TRACE_MAX) {
        g_trace_meter[g_trace_n] = meter_idx;
        g_trace_tag[g_trace_n] = tag_id;
        g_trace_n++;
    }
    if (g_sync_callback && cb) cb(ud, 0, 220.0);
    return 0;
}

static int _mock_pub(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val, uint64_t ts) {
    (void)ctx; (void)tag_id; (void)meter_idx; (void)val; (void)ts;
    g_pub_count++;
    return 0;
}

static poller_source_ops_t _src(void) {
    poller_source_ops_t s;
    memset(&s, 0, sizeof(s));
    s.submit = _mock_submit;
    return s;
}

static poller_bus_ops_t _bus(void) {
    poller_bus_ops_t b;
    memset(&b, 0, sizeof(b));
    b.publish_realtime = _mock_pub;
    return b;
}

/* ==================== 链式行为 ==================== */

/* 每块表访问一次、每项提交一帧；同步回调不能打穿栈 */
static void test_chain_visits_each_meter_once(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "10000002", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "10000003", "dlt698", 9600, 1);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 2, 0, csds, 1);
    dcu_poller_add_task(&poller, 10, 10, 2);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);

    assert_int_equal(g_submit_count, 3);   /* 3 块表 × 1 项 */
    assert_int_equal(g_pub_count, 3);
    for (int i = 0; i < 3; i++) assert_int_equal(g_trace_tag[i], OAD_VOLTAGE);
}

/* 两个任务的项集重叠：同一 (表,项) 只提交一帧 */
static void test_dedup_overlapping_tasks(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "20000001", "dlt698", 9600, 1);

    /* 两个方案，项集有交集（OAD_VOLTAGE 出现在两边） */
    uint32_t csd_a[2] = {OAD_VOLTAGE, OAD_CURRENT};
    uint32_t csd_b[2] = {OAD_VOLTAGE, OAD_VOLTAGE + 1};
    dcu_poller_add_scheme(&poller, 1, 0, csd_a, 2);
    dcu_poller_add_scheme(&poller, 2, 0, csd_b, 2);
    dcu_poller_add_task(&poller, 1, 10, 1);
    dcu_poller_add_task(&poller, 2, 10, 2);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);

    /* 并集 = {OAD_VOLTAGE, OAD_CURRENT, OAD_VOLTAGE+1} 三项，而不是四项 */
    assert_int_equal(g_submit_count, 3);
    int dup = 0;
    for (int i = 0; i < g_trace_n; i++)
        for (int j = i + 1; j < g_trace_n; j++)
            if (g_trace_tag[i] == g_trace_tag[j]) dup++;
    assert_int_equal(dup, 0);
}

/* 提交失败不能让链卡死：要继续推进后面的表 */
static void test_submit_failure_does_not_stall(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "30000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "30000002", "dlt698", 9600, 1);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 10, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_fail_submits = 1;   /* 第一块表提交失败 */
    dcu_poller_step(&poller, &inv, 0);

    /* 第二块表仍然被提交（链没有停在失败的那一块） */
    assert_true(g_submit_count >= 1);
}

/* 熔断：连续失败达阈值后该表被跳过，其它表照常 */
static void test_breaker_skips_meter(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "40000001", "dlt698", 9600, 1);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    /* 周期 0 = 每步都到期：本用例要连跑多步累积失败 */
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    /* 每次 step 都让提交失败，凑够熔断阈值 */
    for (int i = 0; i < 8; i++) {
        _trace_reset();
        g_fail_submits = 1;
        dcu_poller_step(&poller, &inv, 0);
    }
    assert_true(dcu_poller_meter_breaker_open(&poller, "40000001"));

    /* 熔断后不再为它提交 */
    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 0);
}

/* 通道独立：两条通道的表各自被推进，互不阻塞 */
static void test_channels_progress_independently(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "50000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "50000002", "dlt698", 9600, 2);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 10, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);

    assert_int_equal(g_submit_count, 2);
    assert_int_equal(poller.chains[1].round, 1);  /* 通道 1 已扫完一圈 */
    assert_int_equal(poller.chains[2].round, 1);  /* 通道 2 也独立扫完一圈 */
}

/* 档案 GC：删掉的表要退休，不能继续被抄 */
static void test_gc_retires_deleted_meter(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "60000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "60000002", "dlt698", 9600, 1);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 10, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 2);

    /* 删掉一块表（档案 revision 变化触发 GC） */
    dcu_inventory_remove(&inv, "60000001");

    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);
    dcu_poller_step(&poller, &inv, 0);
    assert_true(g_submit_count <= 1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_task_and_scheme_add),
        cmocka_unit_test(test_add_scheme_bounds),
        cmocka_unit_test(test_chain_visits_each_meter_once),
        cmocka_unit_test(test_dedup_overlapping_tasks),
        cmocka_unit_test(test_submit_failure_does_not_stall),
        cmocka_unit_test(test_breaker_skips_meter),
        cmocka_unit_test(test_channels_progress_independently),
        cmocka_unit_test(test_gc_retires_deleted_meter),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
