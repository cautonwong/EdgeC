#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "arbiter_types.h"
#include "dcu_bus_source.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "edge_api.h"

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
static int g_fail_forever;      /* 1 = 每次 submit 都返回 -1 */
static int g_attempts;          /* 提交尝试数（含失败） */
static int g_sync_callback;     /* 1 = 同步回调（验证 pumping 防重入） */
static uint32_t g_trace_meter[TRACE_MAX];
static uint32_t g_trace_tag[TRACE_MAX];
static uint32_t g_trace_mask[TRACE_MAX];
static int g_trace_n;
static int g_async_mode;
static poller_read_done_cb g_pending_cb[TRACE_MAX];
static void* g_pending_ud[TRACE_MAX];
static int g_pending_n;
static uint32_t g_pub_tag;
static uint32_t g_pub_dev;
static double g_pub_val;

static void _trace_reset(void) {
    g_submit_count = 0;
    g_pub_count = 0;
    g_fail_submits = -1;
    g_fail_forever = 0;
    g_attempts = 0;
    g_sync_callback = 1;
    g_trace_n = 0;
    g_async_mode = 0;
    g_pending_n = 0;
}

static int _mock_submit(void* ctx, uint32_t meter_idx, uint32_t tag_id, uint32_t task_mask,
                        poller_read_done_cb cb, void* ud) {
    (void)ctx;
    g_attempts++;
    if (g_fail_forever) return -1;
    if (g_fail_submits > 0) {
        g_fail_submits--;
        return -1;
    }
    g_submit_count++;
    if (g_trace_n < TRACE_MAX) {
        g_trace_meter[g_trace_n] = meter_idx;
        g_trace_tag[g_trace_n] = tag_id;
        g_trace_mask[g_trace_n] = task_mask;
        g_trace_n++;
    }
    if (g_async_mode && cb && g_pending_n < TRACE_MAX) {
        g_pending_cb[g_pending_n] = cb;
        g_pending_ud[g_pending_n] = ud;
        g_pending_n++;
    } else if (g_sync_callback && cb) {
        cb(ud, 0, 220.0);
    }
    return 0;
}

static int _mock_pub(void* ctx, uint32_t tag_id, uint32_t device_id, double val, uint64_t ts) {
    (void)ctx; (void)ts;
    g_pub_tag = tag_id;
    g_pub_dev = device_id;
    g_pub_val = val;
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

static int g_rec_n;
static char g_rec_addr[32][MAX_ADDR_LEN];
static uint32_t g_rec_dev[32];
static uint32_t g_rec_tag[32];

static int _mock_record(void* ctx, const char* addr, uint32_t device_id, uint32_t tag_id,
                        double val) {
    (void)ctx; (void)val;
    if (g_rec_n < 32) {
        strncpy(g_rec_addr[g_rec_n], addr, MAX_ADDR_LEN - 1);
        g_rec_addr[g_rec_n][MAX_ADDR_LEN - 1] = '\0';
        g_rec_dev[g_rec_n] = device_id;
        g_rec_tag[g_rec_n] = tag_id;
        g_rec_n++;
    }
    return 0;
}

static poller_sink_ops_t _record_sink(void) {
    poller_sink_ops_t s;
    memset(&s, 0, sizeof(s));
    s.record_telemetry = _mock_record;
    return s;
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
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 1);
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

static void test_success_counts_only_due_enabled_tasks(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "70000001", "dlt698", 9600, 1);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);
    dcu_poller_add_task(&poller, 2, 0, 1);
    dcu_poller_add_task(&poller, 3, 0, 1);
    assert_int_equal(dcu_poller_set_task_status(&poller, 2, DCU_TASK_STATUS_DISABLED), 0);
    poller.tasks[2].has_interval = false; /* TI 0：无周期触发 */

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);

    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 0);
    assert_int_equal(poller.tasks[2].monitor.success_meters, 0);
}


/* 提交发生在到期 tick、回调落在后续非到期 tick：成功仍须计入本次执行。 */
static void test_success_counts_across_non_due_tick(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "71000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "71000002", "dlt698", 9600, 2);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 10, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0); /* 到期 tick：两表各提交一项 */
    assert_int_equal(g_pending_n, 2);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);

    dcu_poller_step(&poller, &inv, 0); /* 非到期 tick：读回调随后才到 */
    g_pending_cb[0](g_pending_ud[0], 0, 220.0);
    g_pending_cb[1](g_pending_ud[1], 1, 0.0);
    dcu_poller_step(&poller, &inv, 0);

    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 2);
    assert_true(poller.tasks[0].monitor.success_meters <=
                poller.tasks[0].monitor.total_meters);
}

/* 一次执行内每个 (表, tag) 只抄一遍：慢读不得触发同执行重抄，下一次执行才重来。 */
static void test_execution_polls_each_meter_once(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "81000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "81000002", "dlt698", 9600, 2);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1); /* 每步都到期 */

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 2);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);

    /* 一块表回调成功、另一块在途：本执行不得把已访问过的表再抄一遍。 */
    g_pending_cb[0](g_pending_ud[0], 0, 220.0);
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 2);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);

    /* 两块都完成：本次执行结算，不再补抄。 */
    g_pending_cb[1](g_pending_ud[1], 0, 220.0);
    assert_int_equal(poller.tasks[0].monitor.status, 2);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 2);

    /* 下一次到期才重新按一遍：每表各一次。 */
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 4);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 2);
    g_pending_cb[2](g_pending_ud[2], 0, 220.0);
    g_pending_cb[3](g_pending_ud[3], 0, 220.0);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 2);
}

/* 单表两任务并集超过一次访问的装载容量：第二任务的项不得被静默丢弃。 */
static void test_pending_capacity_starves_second_task(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "F0000001", "dlt698", 9600, 1), 0);
    uint32_t s1[32], s2[32];
    for (int i = 0; i < 32; i++) {
        s1[i] = 0x3000u + (uint32_t)i;
        s2[i] = 0x4000u + (uint32_t)i;
    }
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, s1, 32), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, s2, 32), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 10, 1), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 2, 10, 2), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);

    int t2_tags = 0;
    int dup = 0;
    for (int i = 0; i < g_trace_n; i++) {
        if (g_trace_tag[i] >= 0x4000u && g_trace_tag[i] < 0x4020u) t2_tags++;
        for (int j = i + 1; j < g_trace_n; j++)
            if (g_trace_meter[i] == g_trace_meter[j] && g_trace_tag[i] == g_trace_tag[j]) dup++;
    }
    assert_int_equal(g_submit_count, 64);
    assert_int_equal(t2_tags, 32);
    assert_int_equal(dup, 0);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
}

/* 并集超容量且两任务共享 tag：共享项只提交一帧且掩码覆盖两个任务。 */
static void test_shared_tag_dedup_across_batches(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "F0000002", "dlt698", 9600, 1), 0);
    uint32_t s1[32], s2[32];
    for (int i = 0; i < 32; i++) s1[i] = 0x3000u + (uint32_t)i;
    s2[0] = 0x3000u; /* 与任务 1 共享，使并集 63 > 32 */
    for (int i = 1; i < 32; i++) s2[i] = 0x4000u + (uint32_t)(i - 1);
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, s1, 32), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, s2, 32), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 10, 1), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 2, 10, 2), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);

    int shared_frames = 0, dup = 0;
    uint32_t shared_mask = 0;
    for (int i = 0; i < g_trace_n; i++) {
        if (g_trace_tag[i] == 0x3000u) {
            shared_frames++;
            shared_mask |= g_trace_mask[i];
        }
        for (int j = i + 1; j < g_trace_n; j++)
            if (g_trace_meter[i] == g_trace_meter[j] && g_trace_tag[i] == g_trace_tag[j]) dup++;
    }
    assert_int_equal(g_submit_count, 63);
    assert_int_equal(shared_frames, 1);
    assert_int_equal(shared_mask, 0x3u);
    assert_int_equal(dup, 0);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
}

/* 任一批次失败：整表所有任务都按失败结算，先装完的任务不得先记成功。 */
static void test_capacity_batch_failure_fails_whole_meter(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "F0000004", "dlt698", 9600, 1), 0);
    uint32_t s1[32], s2[32];
    for (int i = 0; i < 32; i++) {
        s1[i] = 0x3000u + (uint32_t)i;
        s2[i] = 0x4000u + (uint32_t)i;
    }
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, s1, 32), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, s2, 32), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 10, 1), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 2, 10, 2), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    g_async_mode = 1;
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);

    int next = 0;
    int failed_task2_first = 0;
    for (int iter = 0; iter < 4000 && (poller.tasks[0].monitor.status == DCU_EXEC_ACTIVE ||
                                       poller.tasks[1].monitor.status == DCU_EXEC_ACTIVE);
         iter++) {
        if (next < g_pending_n) {
            poller_read_done_cb cb = g_pending_cb[next];
            void* ud = g_pending_ud[next];
            int status = 0;
            if (!failed_task2_first && g_trace_tag[next] >= 0x4000u &&
                g_trace_tag[next] < 0x4020u) {
                status = -1;
                failed_task2_first = 1;
            }
            next++;
            cb(ud, status, 220.0);
        } else {
            assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
        }
    }

    assert_int_equal(failed_task2_first, 1);
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 0);
}

/* 分批跨 step：异步回调下一次只推进一项，多批之间不得丢项或重发。 */
static void test_capacity_batches_across_steps(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "F0000003", "dlt698", 9600, 1), 0);
    uint32_t s1[32], s2[32];
    for (int i = 0; i < 32; i++) {
        s1[i] = 0x3000u + (uint32_t)i;
        s2[i] = 0x4000u + (uint32_t)i;
    }
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, s1, 32), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, s2, 32), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 10, 1), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 2, 10, 2), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    g_async_mode = 1;
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
    assert_int_equal(g_submit_count, 1); /* 链上在途最多 1：先只发出一项 */

    int next = 0;
    for (int iter = 0; iter < 4000 && poller.tasks[1].monitor.status == DCU_EXEC_ACTIVE; iter++) {
        if (next < g_pending_n) {
            poller_read_done_cb cb = g_pending_cb[next];
            void* ud = g_pending_ud[next];
            next++;
            cb(ud, 0, 220.0);
        } else {
            assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
        }
    }
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(g_submit_count, 64);
    int dup = 0;
    for (int i = 0; i < g_trace_n; i++)
        for (int j = i + 1; j < g_trace_n; j++)
            if (g_trace_meter[i] == g_trace_meter[j] && g_trace_tag[i] == g_trace_tag[j]) dup++;
    assert_int_equal(dup, 0);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 1);
}

/* 任务暂停期间的装载必须与 note_visit 同一资格：暂停时不装载、不记 visited，
 * 恢复后只按一遍，不得同一执行重发。 */
static void test_transient_disable_does_not_reload(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "82000001", "dlt698", 9600, 1);
    dcu_inventory_add(&inv, "82000002", "dlt698", 9600, 1);
    uint32_t csds[3] = {OAD_VOLTAGE, OAD_CURRENT, OAD_ACTIVE_ENERGY};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 3);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 1);
    assert_int_equal(dcu_poller_set_task_status(&poller, 1, DCU_TASK_STATUS_DISABLED), 0);

    int next = 0;
    while (next < g_pending_n) {
        poller_read_done_cb cb = g_pending_cb[next];
        void* ud = g_pending_ud[next];
        next++;
        cb(ud, 0, 220.0);
    }
    assert_int_equal(g_submit_count, 3); /* 第一块表的三项；暂停期间不得装载第二块 */

    /* 暂停使执行转 IDLE；恢复后是一次新执行，每表各一遍（与旧执行的重复是允许的）。 */
    int mark = g_trace_n;
    assert_int_equal(dcu_poller_set_task_status(&poller, 1, DCU_TASK_STATUS_NORMAL), 0);
    dcu_poller_step(&poller, &inv, 0);
    while (next < g_pending_n) {
        poller_read_done_cb cb = g_pending_cb[next];
        void* ud = g_pending_ud[next];
        next++;
        cb(ud, 0, 220.0);
    }

    assert_int_equal(g_trace_n - mark, 6); /* 新执行：两表各 3 项 */
    int dup = 0;
    for (int i = mark; i < g_trace_n; i++)
        for (int j = i + 1; j < g_trace_n; j++)
            if (g_trace_meter[i] == g_trace_meter[j] && g_trace_tag[i] == g_trace_tag[j]) dup++;
    assert_int_equal(dup, 0);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 2);
}

/* 档案重开：作废轮已装载未发出的 tag 必须丢弃，不能重开后补发再装载。 */
static void test_reopen_drops_stale_pending(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "83000001", "dlt698", 9600, 1);
    uint32_t csds[3] = {OAD_VOLTAGE, OAD_CURRENT, OAD_ACTIVE_ENERGY};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 3);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 1);
    assert_int_equal(poller.chains[1].pending_n, 3);
    assert_int_equal(poller.chains[1].pending_i, 1);

    dcu_inventory_add(&inv, "83000002", "dlt698", 9600, 1);
    int mark = g_submit_count;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 2);

    int next = 0;
    for (int iter = 0; iter < 400 && poller.tasks[0].monitor.status == 1; iter++) {
        if (next < g_pending_n) {
            poller_read_done_cb cb = g_pending_cb[next];
            void* ud = g_pending_ud[next];
            next++;
            cb(ud, 0, 220.0);
        } else {
            dcu_poller_step(&poller, &inv, 0);
        }
    }
    int dup = 0;
    for (int i = mark; i < g_trace_n; i++)
        for (int j = i + 1; j < g_trace_n; j++)
            if (g_trace_meter[i] == g_trace_meter[j] && g_trace_tag[i] == g_trace_tag[j]) dup++;
    assert_int_equal(dup, 0);
    assert_int_equal(g_submit_count - mark, 6);
}

/* 混合访问：一个 tag 成功、一个失败 → 该表成功计数必须是 0（全部成功才计）。 */
static void test_mixed_visit_does_not_count_success(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "84000001", "dlt698", 9600, 1);
    uint32_t csds[2] = {OAD_VOLTAGE, OAD_CURRENT};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 2);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_pending_n, 1);

    g_pending_cb[0](g_pending_ud[0], 0, 220.0);
    assert_int_equal(g_pending_n, 2);
    g_pending_cb[1](g_pending_ud[1], -1, 0.0);

    assert_int_equal(poller.tasks[0].monitor.status, 2);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 1);
}

/* 表级熔断：连续失败打开后跳过该表；冷却到期放行一次探测，成功即关闭。 */
static dcu_meter_poll_state_t* _find_state(dcu_poller_t* poller, const char* addr,
                                           uint8_t channel) {
    for (uint32_t i = 0; i < METER_TABLE_CAP; i++) {
        dcu_meter_poll_state_t* mp = &poller->meter_poll_state[i];
        if (mp->tracked && mp->channel_id == channel && strcmp(mp->address, addr) == 0) return mp;
    }
    return NULL;
}

static void test_breaker_cooldown_half_open(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "85000001", "dlt698", 9600, 1);
    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_fail_forever = 1;
    for (int i = 0; i < 6; i++) dcu_poller_step(&poller, &inv, 0);
    assert_true(dcu_poller_meter_breaker_open(&poller, "85000001"));
    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 0); /* 冷却未到期：跳过 */

    dcu_meter_poll_state_t* mp = _find_state(&poller, "85000001", 1);
    assert_non_null(mp);
    mp->breaker_state = DCU_BREAKER_OPEN;
    mp->breaker_since_ms = uv_hrtime() / 1000000ull - DCU_BREAKER_COOLDOWN_MS - 1;
    g_fail_forever = 0; /* 探测成功 */
    _trace_reset();
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_submit_count, 1);
    assert_false(dcu_poller_meter_breaker_open(&poller, "85000001"));
}

/* 冷却到期探测失败：熔断保持打开并重新计时，下一步继续跳过。 */
static void test_breaker_cooldown_probe_failure_rearms(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "85000002", "dlt698", 9600, 1);
    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_fail_forever = 1;
    for (int i = 0; i < 6; i++) dcu_poller_step(&poller, &inv, 0);
    dcu_meter_poll_state_t* mp = _find_state(&poller, "85000002", 1);
    assert_non_null(mp);
    mp->breaker_state = DCU_BREAKER_OPEN;
    mp->breaker_since_ms = uv_hrtime() / 1000000ull - DCU_BREAKER_COOLDOWN_MS - 1;
    uint64_t old = mp->breaker_since_ms;
    _trace_reset();
    g_fail_forever = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_attempts, 1); /* 放行一次探测 */
    assert_true(dcu_poller_meter_breaker_open(&poller, "85000002"));
    assert_true(mp->breaker_since_ms > old); /* 重新计时 */
    assert_int_equal(mp->breaker_state, DCU_BREAKER_OPEN);
    _trace_reset();
    g_fail_forever = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_attempts, 0); /* 新冷却内继续跳过 */
}

/* 执行内档案变化：重开重新快照 total，旧回调不结算新执行；success<=total 无条件。 */
static void test_reopen_resnapshots_and_ignores_stale_callbacks(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "86000001", "dlt698", 9600, 1);
    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_pending_n, 1);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 1);

    dcu_inventory_add(&inv, "86000002", "dlt698", 9600, 1);
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 2);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);

    g_pending_cb[0](g_pending_ud[0], 0, 220.0); /* 旧执行的迟到回调 */
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);
    assert_true(poller.tasks[0].monitor.success_meters <= poller.tasks[0].monitor.total_meters);
}

/* task_mask 变化：重开快照，target 清空时 success<=total 仍成立。 */
static void test_mask_change_reopens_execution(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "86000003", "dlt698", 9600, 1);
    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 1);

    assert_int_equal(dcu_poller_set_meter_tasks(&poller, "86000003", 1, 0), 0);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 0);
    g_pending_cb[0](g_pending_ud[0], 0, 220.0);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);
    assert_true(poller.tasks[0].monitor.success_meters <= poller.tasks[0].monitor.total_meters);
}

/* 同表重叠任务：访问未结算前 B 不得完成；结算后两任务各计一次成功。 */
static void test_overlapping_tasks_settle_on_shared_visit(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "86000004", "dlt698", 9600, 1);
    uint32_t csd_a[2] = {OAD_VOLTAGE, OAD_CURRENT};
    uint32_t csd_b[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csd_a, 2);
    dcu_poller_add_scheme(&poller, 2, 0, csd_b, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);
    dcu_poller_add_task(&poller, 2, 0, 2);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_pending_n, 1); /* 共享 tag 只发一帧 */

    g_pending_cb[0](g_pending_ud[0], 0, 220.0); /* t1 回来：B 的项已全部回调 */
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_ACTIVE);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_ACTIVE); /* A1：B 不得提前完成 */

    g_pending_cb[1](g_pending_ud[1], 0, 1.0); /* t2 回来，访问结算 */
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 1);
}

/* 601E 复位走同一个迁移函数：reset 后 CLOSED，可再次访问。 */
static void test_reset_circuit_breaker_closes(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "86000005", "dlt698", 9600, 1);
    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    dcu_poller_start(&poller, _src(), _bus(), sink);

    _trace_reset();
    g_fail_forever = 1;
    for (int i = 0; i < 6; i++) dcu_poller_step(&poller, &inv, 0);
    assert_true(dcu_poller_meter_breaker_open(&poller, "86000005"));
    assert_int_equal(dcu_poller_reset_circuit_breaker(&poller, 1), 0);
    assert_false(dcu_poller_meter_breaker_open(&poller, "86000005"));
}

/* ==================== 档案删除后的迟到回调身份 ==================== */

/* 捕获总线读请求：真实 edge_api 需要组合根，这里拿走后由测试决定何时完成。 */
static int g_wrap_reads;
static int g_wrap_bus;
static char g_wrap_protocol[16];
static uint32_t g_wrap_slave;
static uint32_t g_wrap_tag;
static legacy_on_complete_cb g_wrap_cb;
static void* g_wrap_ud;

static void _wrap_reset(void) {
    g_wrap_reads = 0;
    g_wrap_bus = -1;
    g_wrap_protocol[0] = '\0';
    g_wrap_slave = 0;
    g_wrap_tag = 0;
    g_wrap_cb = NULL;
    g_wrap_ud = NULL;
}

int __wrap_edge_api_read_tag(int bus_id, const char* protocol, uint32_t slave_id,
                             uint32_t physical_tag_id, int priority, legacy_on_complete_cb cb,
                             void* user_data);
int __wrap_dcu_698_rules_retry_with_spare(const char* protocol, uint32_t oad, int already_retried,
                                          uint32_t* spare_protocol_id);

int __wrap_edge_api_read_tag(int bus_id, const char* protocol, uint32_t slave_id,
                             uint32_t physical_tag_id, int priority, legacy_on_complete_cb cb,
                             void* user_data) {
    (void)priority;
    g_wrap_reads++;
    g_wrap_bus = bus_id;
    strncpy(g_wrap_protocol, protocol ? protocol : "", sizeof(g_wrap_protocol) - 1);
    g_wrap_protocol[sizeof(g_wrap_protocol) - 1] = '\0';
    g_wrap_slave = slave_id;
    g_wrap_tag = physical_tag_id;
    g_wrap_cb = cb;
    g_wrap_ud = user_data;
    return 0;
}

static int g_force_spare;
int __wrap_dcu_698_rules_retry_with_spare(const char* protocol, uint32_t oad, int already_retried,
                                          uint32_t* spare_protocol_id) {
    (void)protocol; (void)oad;
    if (!g_force_spare || already_retried || !spare_protocol_id) return 0;
    *spare_protocol_id = 0xDEAD;
    return 1;
}

static int g_done_n;
static int g_done_status;
static double g_done_value;

static void _capture_done(void* ud, int status, double value) {
    (void)ud;
    g_done_n++;
    g_done_status = status;
    g_done_value = value;
}

/* 档案删除用末元素覆盖删除位，下标会移位；在途读的迟到回调必须按提交时的
 * 身份（地址/tag/mask/slot）落库与发布，不能按回调时刻的档案下标反查。 */
static void test_reopen_late_callback_wrong_address(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    /* slot 与下标刻意错开：删除后 B 的下标=0、slot=6，只有快照身份能区分 A/B。 */
    inv.next_slot = 5;
    assert_int_equal(dcu_inventory_add(&inv, "A0000001", "dlt698", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "B0000002", "dlt698", 9600, 1), 0);
    assert_int_equal(inv.meters[0].slot_id, 5);
    assert_int_equal(inv.meters[1].slot_id, 6);

    uint32_t csds[3] = {OAD_VOLTAGE, OAD_CURRENT, OAD_ACTIVE_ENERGY};
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, csds, 3), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 0, 1), 0);
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), _record_sink()), 0);

    _trace_reset();
    g_rec_n = 0;
    g_pub_dev = UINT32_MAX;
    g_async_mode = 1;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_pending_n, 1); /* A 的第一项在途 */
    assert_int_equal(g_trace_meter[0], 0);
    assert_int_equal(g_trace_tag[0], OAD_VOLTAGE);

    assert_int_equal(dcu_inventory_remove(&inv, "A0000001"), 0);
    assert_string_equal(inv.meters[0].address, "B0000002");
    assert_int_equal(inv.meters[0].slot_id, 6);
    dcu_poller_step(&poller, &inv, 0); /* 档案变更：重开执行、清链上 pending */

    /* A 的迟到成功回调：值来自 A，必须记到 A 名下，且不计入新执行。 */
    g_pending_cb[0](g_pending_ud[0], 0, 220.0);
    assert_int_equal(g_rec_n, 1);
    assert_string_equal(g_rec_addr[0], "A0000001");
    assert_int_equal(g_rec_dev[0], 5);
    assert_int_equal(g_rec_tag[0], OAD_VOLTAGE);
    assert_int_equal(g_pub_dev, 5); /* 实时发布用 A 的 slot，不是 B 的 6 */
    assert_int_equal(poller.tasks[0].monitor.success_meters, 0);
    assert_int_equal(poller.tasks[0].monitor.settled_left, 1);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 1);

    /* 新执行正常抄完 B：A 的迟到回调不得让 B 提前结算或计成功。 */
    int next = 1;
    for (int iter = 0; iter < 64 && poller.tasks[0].monitor.status != DCU_EXEC_DONE; iter++) {
        if (next < g_pending_n) {
            poller_read_done_cb cb = g_pending_cb[next];
            void* ud = g_pending_ud[next];
            next++;
            cb(ud, 0, 220.0);
        } else {
            dcu_poller_step(&poller, &inv, 0);
        }
    }
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(g_rec_n, 4); /* A 的迟到项 + B 的三项 */
    for (int i = 1; i < g_rec_n; i++) {
        assert_string_equal(g_rec_addr[i], "B0000002");
        assert_int_equal(g_rec_dev[i], 6);
    }
}

/* 完成时的量纲换算必须用提交时的 protocol：A 在读途中被删，下标 0 换成 B。 */
static void test_late_callback_uses_submit_protocol_for_scale(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    assert_int_equal(dcu_inventory_add(&inv, "A0000001", "DLT645", 2400, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "B0000002", "DLT698", 9600, 1), 0);

    dcu_bus_source_t* bs = dcu_bus_source_start(NULL, &inv, NULL, 0);
    assert_non_null(bs);
    _wrap_reset();
    g_done_n = 0;
    g_done_value = -1.0;

    poller_source_ops_t ops = dcu_bus_source_ops(bs);
    assert_int_equal(ops.submit(ops.ctx, 0, OAD_CURRENT, 1, _capture_done, NULL), 0);
    assert_int_equal(g_wrap_reads, 1);
    assert_string_equal(g_wrap_protocol, "DLT645");
    assert_non_null(g_wrap_cb);

    assert_int_equal(dcu_inventory_remove(&inv, "A0000001"), 0);
    assert_string_equal(inv.meters[0].address, "B0000002");

    pipe_msg_t result;
    pipe_msg_init_scalar(&result, OAD_CURRENT, 1000.0, 0);
    g_wrap_cb(0, &result, g_wrap_ud);

    assert_int_equal(g_done_n, 1);
    assert_int_equal(g_done_status, 0);
    /* DLT645 的 OAD_CURRENT 规则 scale=0.001；若按回调时刻的 B(DLT698) 反查，
     * find_by_proto 查不到 645 的 DI，scale=0，值会原样是 1000.0。 */
    assert_true(g_done_value > 0.999 && g_done_value < 1.001);
    dcu_bus_source_stop(bs);
}

/* 迟到失败不得用移位后的下标发起备用 DI 重试：那会把 B 的读数记到 A 名下。 */
static void test_late_failure_does_not_retry_shifted_index(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    assert_int_equal(dcu_inventory_add(&inv, "A0000001", "DLT645", 2400, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "B0000002", "DLT698", 9600, 1), 0);

    dcu_bus_source_t* bs = dcu_bus_source_start(NULL, &inv, NULL, 0);
    assert_non_null(bs);
    _wrap_reset();
    g_done_n = 0;

    poller_source_ops_t ops = dcu_bus_source_ops(bs);
    assert_int_equal(ops.submit(ops.ctx, 0, OAD_VOLTAGE, 1, _capture_done, NULL), 0);
    assert_int_equal(g_wrap_reads, 1);

    assert_int_equal(dcu_inventory_remove(&inv, "A0000001"), 0);
    g_force_spare = 1;
    g_wrap_cb(CMD_STATUS_PROTO_ERROR, NULL, g_wrap_ud);
    g_force_spare = 0;

    assert_int_equal(g_wrap_reads, 1); /* 身份已不在原下标：不重试 */
    assert_int_equal(g_done_n, 1);
    assert_int_equal(g_done_status, CMD_STATUS_PROTO_ERROR);
    dcu_bus_source_stop(bs);
}

/* 档案 revision 已变但 step 尚未观测：迟到回调不得让旧访问把剩余项发到移位后的下标。
 * A 被删、B 顶到下标 0；旧访问的第二项若续发，会读到 B 却记到 A 名下。 */
static void test_late_callback_before_revision_observed(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    inv.next_slot = 5;
    assert_int_equal(dcu_inventory_add(&inv, "A0000001", "DLT645", 2400, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "B0000002", "DLT698", 9600, 1), 0);

    uint32_t csds[2] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, csds, 2), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 0, 1), 0);

    dcu_bus_source_t* bs = dcu_bus_source_start(NULL, &inv, NULL, 0);
    assert_non_null(bs);
    assert_int_equal(dcu_poller_start(&poller, dcu_bus_source_ops(bs), _bus(), _record_sink()), 0);

    _wrap_reset();
    g_rec_n = 0;
    g_pub_dev = UINT32_MAX;
    dcu_poller_step(&poller, &inv, 0);
    assert_int_equal(g_wrap_reads, 1);
    assert_string_equal(g_wrap_protocol, "DLT645");
    assert_int_equal(g_wrap_slave, 0);

    /* A 在途时删除 A：B 顶到下标 0。poller 还没 step，按设计尚未重开。 */
    assert_int_equal(dcu_inventory_remove(&inv, "A0000001"), 0);
    assert_string_equal(inv.meters[0].address, "B0000002");

    /* A 的迟到成功回调：自己的结果按快照落到 A；旧访问的剩余项不得再提交。 */
    pipe_msg_t result;
    pipe_msg_init_scalar(&result, OAD_VOLTAGE, 1000.0, 0);
    g_wrap_cb(0, &result, g_wrap_ud);

    assert_int_equal(g_wrap_reads, 1);
    assert_int_equal(g_rec_n, 1);
    assert_string_equal(g_rec_addr[0], "A0000001");
    assert_int_equal(g_rec_dev[0], 5);
    assert_int_equal(g_rec_tag[0], OAD_VOLTAGE);
    assert_int_equal(g_pub_dev, 5);
    assert_int_equal(poller.chains[1].visit_fail, 0);
    dcu_bus_source_stop(bs);
}

/* 表只移位未删：前面的 C 被删，A 从下标 1 移到 0。旧访问的剩余项按旧下标续发
 * 会被 bus_source 判越界，一帧都没发出；却给健康的 A 记一次访问失败和熔断 strike。 */
static void test_late_callback_shift_does_not_fail_meter(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    inv.next_slot = 5;
    assert_int_equal(dcu_inventory_add(&inv, "C0000000", "DLT698", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(&inv, "A0000001", "DLT645", 2400, 1), 0);

    uint32_t csds[2] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, csds, 2), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 0, 1), 0);

    dcu_bus_source_t* bs = dcu_bus_source_start(NULL, &inv, NULL, 0);
    assert_non_null(bs);
    assert_int_equal(dcu_poller_start(&poller, dcu_bus_source_ops(bs), _bus(), _record_sink()), 0);

    _wrap_reset();
    g_rec_n = 0;
    dcu_poller_step(&poller, &inv, 0);

    /* 抄完 C 的两项，游标推进到 A 并发出一帧 A 的在途读。 */
    for (int i = 0; i < 8 && g_wrap_slave != 1; i++) {
        if (g_wrap_cb) {
            legacy_on_complete_cb cb = g_wrap_cb;
            void* ud = g_wrap_ud;
            pipe_msg_t result;
            pipe_msg_init_scalar(&result, g_wrap_tag, 100.0, 0);
            cb(0, &result, ud);
        }
        if (g_wrap_slave != 1) dcu_poller_step(&poller, &inv, 0);
    }
    assert_int_equal(g_wrap_reads, 3);
    assert_int_equal(g_wrap_slave, 1);
    assert_string_equal(g_wrap_protocol, "DLT645");

    /* 删除前表 C：A 从下标 1 移到 0；poller 还没 step。 */
    assert_int_equal(dcu_inventory_remove(&inv, "C0000000"), 0);
    assert_int_equal(inv.count, 1);
    assert_string_equal(inv.meters[0].address, "A0000001");

    dcu_meter_poll_state_t* mp = _find_state(&poller, "A0000001", 1);
    assert_non_null(mp);
    uint32_t fails_before = mp->consecutive_fails;
    uint64_t errors_before = poller.channel_stats[1].error_count;
    int reads_before = g_wrap_reads;
    int rec_before = g_rec_n; /* C 的两项已落库 */

    /* A 第一项的迟到成功回调：不得续发越界帧，也不得给 A 记假失败。 */
    pipe_msg_t late;
    pipe_msg_init_scalar(&late, OAD_VOLTAGE, 1000.0, 0);
    g_wrap_cb(0, &late, g_wrap_ud);

    assert_int_equal(g_wrap_reads, reads_before);
    assert_int_equal(mp->consecutive_fails, fails_before);
    assert_int_equal(poller.channel_stats[1].error_count, errors_before);
    assert_int_equal(poller.chains[1].visit_fail, 0);
    assert_int_equal(g_rec_n, rec_before + 1);
    assert_string_equal(g_rec_addr[rec_before], "A0000001");
    assert_int_equal(g_rec_dev[rec_before], 6);
    assert_int_equal(g_rec_tag[rec_before], OAD_VOLTAGE);

    /* step 观测 revision 后重开：新执行把 A 的全部 tag 抄完并计一次成功。 */
    dcu_poller_step(&poller, &inv, 0);
    for (int i = 0; i < 8 && poller.tasks[0].monitor.status != DCU_EXEC_DONE; i++) {
        if (g_wrap_cb) {
            legacy_on_complete_cb cb = g_wrap_cb;
            void* ud = g_wrap_ud;
            pipe_msg_t result;
            pipe_msg_init_scalar(&result, g_wrap_tag, 100.0, 0);
            cb(0, &result, ud);
        } else {
            dcu_poller_step(&poller, &inv, 0);
        }
    }
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[0].monitor.total_meters, 1);
    assert_int_equal(g_rec_n, rec_before + 3); /* 迟到项 + 新执行的两项 */
    int seen_voltage = 0, seen_current = 0;
    for (int i = rec_before + 1; i < g_rec_n; i++) {
        assert_string_equal(g_rec_addr[i], "A0000001");
        assert_int_equal(g_rec_dev[i], 6);
        if (g_rec_tag[i] == OAD_VOLTAGE) seen_voltage++;
        if (g_rec_tag[i] == OAD_CURRENT) seen_current++;
    }
    assert_int_equal(seen_voltage, 1);
    assert_int_equal(seen_current, 1);
    dcu_bus_source_stop(bs);
}

/* slot 号耗尽后回收空洞：新表继承被删表的 slot_id 并落在同一下标。
 * 备用 DI 重试只认提交时的档案 revision：revision 变了下标可能已指向新表。 */
static void test_spare_retry_dropped_after_slot_reuse(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);

    char addr[32];
    for (int i = 0; i < MAX_METERS; i++) {
        snprintf(addr, sizeof(addr), "M%06d", i);
        assert_int_equal(dcu_inventory_add(&inv, addr, "DLT645", 2400, 1), 0);
    }
    assert_int_equal(inv.next_slot, MAX_METERS);
    for (int slot = MAX_METERS - 1; slot >= 2; slot--) {
        snprintf(addr, sizeof(addr), "M%06d", slot);
        assert_int_equal(dcu_inventory_remove(&inv, addr), 0);
    }
    assert_int_equal(inv.count, 2);
    assert_string_equal(inv.meters[0].address, "M000000");
    assert_string_equal(inv.meters[1].address, "M000001");

    dcu_bus_source_t* bs = dcu_bus_source_start(NULL, &inv, NULL, 0);
    assert_non_null(bs);
    _wrap_reset();
    g_done_n = 0;

    poller_source_ops_t ops = dcu_bus_source_ops(bs);
    assert_int_equal(ops.submit(ops.ctx, 1, OAD_VOLTAGE, 1, _capture_done, NULL), 0);
    assert_int_equal(g_wrap_reads, 1);

    /* 删除在途表（末尾元素）：新表 C 继承回收的 slot 1 并落在下标 1。 */
    assert_int_equal(dcu_inventory_remove(&inv, "M000001"), 0);
    assert_int_equal(dcu_inventory_add(&inv, "C0000003", "DLT698", 9600, 1), 0);
    assert_int_equal(inv.meters[1].slot_id, 1);
    assert_string_equal(inv.meters[1].address, "C0000003");

    g_force_spare = 1;
    g_wrap_cb(CMD_STATUS_PROTO_ERROR, NULL, g_wrap_ud);
    g_force_spare = 0;

    assert_int_equal(g_wrap_reads, 1); /* revision 已变：不得向继承 slot 的 C 重试 */
    assert_int_equal(g_done_n, 1);
    assert_int_equal(g_done_status, CMD_STATUS_PROTO_ERROR);
    dcu_bus_source_stop(bs);
}

/* revision 不变时备用 DI 重试仍要发生：同一次读的重试属于同一 revision，
 * 原下标不可能移位，重试必须沿用原 protocol/index/channel。 */
static void test_spare_retry_allowed_when_revision_unchanged(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    assert_int_equal(dcu_inventory_add(&inv, "D0000004", "DLT698", 9600, 2), 0);

    dcu_bus_source_t* bs = dcu_bus_source_start(NULL, &inv, NULL, 0);
    assert_non_null(bs);
    _wrap_reset();
    g_done_n = 0;

    poller_source_ops_t ops = dcu_bus_source_ops(bs);
    assert_int_equal(ops.submit(ops.ctx, 0, OAD_VOLTAGE, 1, _capture_done, NULL), 0);
    assert_int_equal(g_wrap_reads, 1);
    assert_int_equal(g_wrap_bus, 2);
    assert_string_equal(g_wrap_protocol, "DLT698");
    assert_int_equal(g_wrap_slave, 0);

    g_force_spare = 1;
    g_wrap_cb(CMD_STATUS_PROTO_ERROR, NULL, g_wrap_ud);
    g_force_spare = 0;

    assert_int_equal(g_wrap_reads, 2);
    assert_int_equal(g_wrap_bus, 2);
    assert_string_equal(g_wrap_protocol, "DLT698");
    assert_int_equal(g_wrap_slave, 0);
    assert_int_equal(g_wrap_tag, 0xDEAD);
    assert_int_equal(g_done_n, 0); /* 第一次失败被重试吸收 */

    pipe_msg_t result;
    pipe_msg_init_scalar(&result, OAD_VOLTAGE, 1000.0, 0);
    g_wrap_cb(0, &result, g_wrap_ud);
    assert_int_equal(g_done_n, 1);
    assert_int_equal(g_done_status, 0);
    dcu_bus_source_stop(bs);
}

/* ==================== F4 修复轮：执行身份回归 ==================== */

/* 6012 Add 对已存在 task_id 换方案走的正是 upsert 路径：执行身份含方案，换方案
 * 必须重开执行，旧 served 位图不得按新方案的按位下标解释（否则漏抄 + 假成功）。 */
static void test_scheme_change_mid_batch_reopens_execution(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "FF000001", "dlt698", 9600, 1), 0);
    uint32_t sa[16], sb[32], sc[32];
    for (int i = 0; i < 16; i++) sa[i] = 0x1000u + (uint32_t)i;
    for (int i = 0; i < 32; i++) sb[i] = 0x2000u + (uint32_t)i;
    /* 新方案 3：index 0..15 全新；index 16 复用方案 2 的 tag；17..31 全新。 */
    for (int i = 0; i < 16; i++) sc[i] = 0x4000u + (uint32_t)i;
    sc[16] = 0x2000u;
    for (int i = 17; i < 32; i++) sc[i] = 0x4010u + (uint32_t)(i - 17);
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, sa, 16), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, sb, 32), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 3, 0, sc, 32), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 0, 1), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 2, 0, 2), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    g_async_mode = 1;
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
    assert_int_equal(poller.chains[1].pending_n, 32); /* 任务1 16 项 + 任务2 方案2 前 16 项 */

    int next = 0;
    for (; next < 31; next++) {
        poller_read_done_cb cb = g_pending_cb[next];
        void* ud = g_pending_ud[next];
        cb(ud, 0, 220.0);
    }
    /* 无关任务 1：执行身份与已装载进度必须在 upsert 后原样保留。 */
    uint32_t unrelated_gen = poller.tasks[0].monitor.exec_gen;
    /* 最后一项在途时换方案：与 6012 Add 同一条 upsert 路径。 */
    uint32_t old_gen = poller.tasks[1].monitor.exec_gen;
    dcu_task_unit_t upd = poller.tasks[1];
    upd.scheme_id = 3;
    assert_int_equal(dcu_poller_upsert_task(&poller, &upd), 0);
    assert_true(poller.tasks[1].monitor.exec_gen != old_gen);
    assert_int_equal(poller.tasks[1].monitor.visited[0] & 1u, 0);
    assert_int_equal(poller.tasks[0].monitor.exec_gen, unrelated_gen);
    assert_int_equal(poller.tasks[0].monitor.visited[0] & 1u, 1);
    int mark = g_trace_n;

    for (int iter = 0; iter < 4000 && (poller.tasks[0].monitor.status == DCU_EXEC_ACTIVE ||
                                       poller.tasks[1].monitor.status == DCU_EXEC_ACTIVE);
         iter++) {
        if (next < g_pending_n) {
            poller_read_done_cb cb = g_pending_cb[next];
            void* ud = g_pending_ud[next];
            next++;
            cb(ud, 0, 220.0);
        } else {
            assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
        }
    }

    /* 重开后的新执行里，方案 3 的每个 tag 恰好提交一帧且都带任务 2 位。 */
    int per_tag[32] = {0};
    for (int i = mark; i < g_trace_n; i++) {
        if (!(g_trace_mask[i] & 0x2u)) continue;
        for (int j = 0; j < 32; j++)
            if (g_trace_tag[i] == sc[j]) per_tag[j]++;
    }
    int missing = 0, dup = 0;
    for (int j = 0; j < 32; j++) {
        if (per_tag[j] == 0) missing++;
        if (per_tag[j] > 1) dup++;
    }
    assert_int_equal(missing, 0);
    assert_int_equal(dup, 0);
    int unrelated_resubmits = 0;
    for (int i = mark; i < g_trace_n; i++)
        if (g_trace_mask[i] & 0x1u) unrelated_resubmits++;
    assert_int_equal(unrelated_resubmits, 0);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
}

/* F4：add_task_ex 只增加一个槽位，不得重开已 ACTIVE 的旧执行。旧实现调用
 * _poller_reopen_active：exec_gen 递增、visited/settled/succeeded 清零、链上
 * pending 丢弃，已装载的批次被重抄；新增任务自己还没有 ACTIVE 执行可重开。 */
static void test_add_task_does_not_reopen_active_execution(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "FF000004", "dlt698", 9600, 1), 0);
    uint32_t sa[16], sb[16];
    for (int i = 0; i < 16; i++) {
        sa[i] = 0x1000u + (uint32_t)i;
        sb[i] = 0x2000u + (uint32_t)i;
    }
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, sa, 16), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, sb, 16), 0);
    /* 非零周期：A 完成本轮后不会在同一步立刻重发，重复计数只可能来自重开。 */
    assert_int_equal(dcu_poller_add_task(&poller, 1, 3600, 1), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    g_async_mode = 1;
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_ACTIVE);
    assert_int_equal(poller.chains[1].pending_n, 16);
    assert_int_equal(poller.tasks[0].monitor.visited[0] & 1u, 1);
    assert_int_equal(poller.tasks[0].monitor.settled_left, 1);

    uint32_t gen_a = poller.tasks[0].monitor.exec_gen;
    uint8_t visited_a = poller.tasks[0].monitor.visited[0];
    assert_int_equal(dcu_poller_add_task_ex(&poller, 2, 3600, 2, 0), 0);

    /* A 的执行身份、进度与在途批次原样保留；B 还没有 ACTIVE 执行。 */
    assert_int_equal(poller.tasks[0].monitor.exec_gen, gen_a);
    assert_int_equal(poller.tasks[0].monitor.visited[0], visited_a);
    assert_int_equal(poller.tasks[0].monitor.settled_left, 1);
    assert_int_equal(poller.chains[1].pending_n, 16);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_IDLE);

    int next = 0;
    for (int iter = 0; iter < 4000 &&
                        (poller.tasks[0].monitor.status != DCU_EXEC_DONE ||
                         poller.tasks[1].monitor.status != DCU_EXEC_DONE);
         iter++) {
        if (next < g_pending_n) {
            poller_read_done_cb cb = g_pending_cb[next];
            void* ud = g_pending_ud[next];
            next++;
            cb(ud, 0, 220.0);
        } else {
            assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
        }
    }

    /* A 从未被重开：每个项恰好提交一次；B 按自己的 begin 正常完成。 */
    assert_int_equal(poller.tasks[0].monitor.exec_gen, gen_a);
    int per_a[16] = {0};
    int per_b[16] = {0};
    for (int i = 0; i < g_trace_n; i++) {
        for (int j = 0; j < 16; j++) {
            if (g_trace_tag[i] == sa[j] && (g_trace_mask[i] & 0x1u)) per_a[j]++;
            if (g_trace_tag[i] == sb[j] && (g_trace_mask[i] & 0x2u)) per_b[j]++;
        }
    }
    int a_missing = 0, a_dup = 0, b_missing = 0, b_dup = 0;
    for (int j = 0; j < 16; j++) {
        if (per_a[j] == 0) a_missing++;
        if (per_a[j] > 1) a_dup++;
        if (per_b[j] == 0) b_missing++;
        if (per_b[j] > 1) b_dup++;
    }
    assert_int_equal(a_missing, 0);
    assert_int_equal(a_dup, 0);
    assert_int_equal(b_missing, 0);
    assert_int_equal(b_dup, 0);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
    assert_true(poller.tasks[1].monitor.exec_gen > gen_a);
}

/* F4R：同一 scheme_id 的内容可被直接 C API 原地替换。执行身份必须随内容变化，
 * 引用任务重开；旧 served 位图不得按下标解释成新方案的已抄项（否则漏抄 + 假成功）。 */
static void test_scheme_content_change_reopens_referencing_execution(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "FF000003", "dlt698", 9600, 1), 0);
    uint32_t other[16], old_csd[32], new_csd[32];
    for (int i = 0; i < 16; i++) other[i] = 0x3000u + (uint32_t)i;
    for (int i = 0; i < 32; i++) {
        old_csd[i] = 0x1000u + (uint32_t)i;
        new_csd[i] = 0x2000u + (uint32_t)i;
    }
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, other, 16), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, old_csd, 32), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 0, 2), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 2, 0, 1), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    g_async_mode = 1;
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
    /* 任务 2 的 32 项只装进前 16 项就满批：served 位图留下半个旧方案。 */
    assert_int_equal(poller.chains[1].pending_n, 32);
    assert_int_equal(poller.chains[1].served[1], 0xFFFFu);
    assert_int_equal(poller.tasks[1].monitor.visited[0] & 1u, 0);

    uint32_t same_gen = poller.tasks[0].monitor.exec_gen;
    uint32_t old_gen = poller.tasks[1].monitor.exec_gen;
    dcu_scheme_t upd;
    memset(&upd, 0, sizeof(upd));
    upd.scheme_id = 1;
    upd.type = SCHEME_TYPE_NORMAL;
    upd.data.normal.csd_count = 32;
    memcpy(upd.data.normal.csd_list, new_csd, sizeof(new_csd));
    assert_int_equal(dcu_poller_upsert_scheme(&poller, &upd), 0);

    /* 身份变化是重开的可观测结果；无关任务的身份不得跟着变。 */
    bool reopened = poller.tasks[1].monitor.exec_gen != old_gen;
    bool unrelated_kept = poller.tasks[0].monitor.exec_gen == same_gen;
    int next = 0;
    int mark = g_trace_n; /* 在飞的旧方案项不参与新方案计数 */
    for (int iter = 0; iter < 4000 && (poller.tasks[0].monitor.status == DCU_EXEC_ACTIVE ||
                                       poller.tasks[1].monitor.status == DCU_EXEC_ACTIVE);
         iter++) {
        if (next < g_pending_n) {
            poller_read_done_cb cb = g_pending_cb[next];
            void* ud = g_pending_ud[next];
            next++;
            cb(ud, 0, 220.0);
        } else {
            assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
        }
    }

    int per_tag[32] = {0};
    for (int i = mark; i < g_trace_n; i++) {
        if (!(g_trace_mask[i] & 0x2u)) continue;
        for (int j = 0; j < 32; j++)
            if (g_trace_tag[i] == new_csd[j]) per_tag[j]++;
    }
    int missing = 0, dup = 0;
    for (int j = 0; j < 32; j++) {
        if (per_tag[j] == 0) missing++;
        if (per_tag[j] > 1) dup++;
    }
    assert_int_equal(missing, 0);
    assert_int_equal(dup, 0);
    assert_true(reopened);
    assert_true(unrelated_kept);
    assert_int_equal(poller.tasks[1].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
    assert_int_equal(poller.tasks[0].monitor.success_meters, 1);
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);

    /* 相同内容再 upsert 不得重开：6014 对已引用方案的同 CSD Add 走这条路径。 */
    uint32_t done_gen = poller.tasks[1].monitor.exec_gen;
    assert_int_equal(dcu_poller_upsert_scheme(&poller, &upd), 0);
    assert_int_equal(poller.tasks[1].monitor.exec_gen, done_gen);
}

/* 任务的新执行在另一任务同表分批进行中开始（disable -> enable）：前一批的失败
 * 属于旧执行，新执行 32 项全部成功必须记成功。用 first_fails 做 A/B 对照。 */
static void _run_new_exec_mid_batch(int first_fails, uint32_t* success_out, int* status_out,
                                    int* submits_out) {
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "FF000002", "dlt698", 9600, 1), 0);
    uint32_t sa[32], sb[32];
    for (int i = 0; i < 32; i++) {
        sa[i] = 0x1000u + (uint32_t)i;
        sb[i] = 0x2000u + (uint32_t)i;
    }
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, sa, 32), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, sb, 32), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 0, 1), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 2, 0, 2), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    g_async_mode = 1;
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);

    /* 第一批：任务 1 的 32 项；首项按参数决定成败。只消耗这一批。 */
    int next = 0;
    for (int i = 0; i < 32; i++) {
        assert_true(next < g_pending_n);
        poller_read_done_cb cb = g_pending_cb[next];
        void* ud = g_pending_ud[next];
        int status = (first_fails && i == 0) ? -1 : 0;
        next++;
        cb(ud, status, 220.0);
    }
    assert_int_equal(poller.chains[1].pending_n, 32);
    assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_ACTIVE);

    uint32_t old_gen = poller.tasks[0].monitor.exec_gen;
    assert_int_equal(dcu_poller_set_task_status(&poller, 1, DCU_TASK_STATUS_DISABLED), 0);
    assert_int_equal(dcu_poller_set_task_status(&poller, 1, DCU_TASK_STATUS_NORMAL), 0);
    assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
    assert_true(poller.tasks[0].monitor.exec_gen != old_gen);
    int mark = g_trace_n;

    for (int iter = 0; iter < 4000 && poller.tasks[0].monitor.status == DCU_EXEC_ACTIVE; iter++) {
        if (next < g_pending_n) {
            poller_read_done_cb cb = g_pending_cb[next];
            void* ud = g_pending_ud[next];
            next++;
            cb(ud, 0, 220.0);
        } else {
            assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
        }
    }

    int new_gen_submits = 0;
    for (int i = mark; i < g_trace_n; i++) {
        if ((g_trace_mask[i] & 0x1u) && g_trace_tag[i] >= 0x1000u && g_trace_tag[i] < 0x1020u)
            new_gen_submits++;
    }
    assert_int_equal(poller.tasks[0].monitor.total_meters, 1);
    assert_int_equal(new_gen_submits, 32);
    *success_out = poller.tasks[0].monitor.success_meters;
    *status_out = poller.tasks[0].monitor.status;
    *submits_out = new_gen_submits;
}

static void test_new_exec_mid_batch_failure_not_tainted(void** state) {
    (void)state;
    uint32_t fail_success = 0, ok_success = 0;
    int fail_status = 0, ok_status = 0, fail_submits = 0, ok_submits = 0;
    _run_new_exec_mid_batch(1, &fail_success, &fail_status, &fail_submits);
    _run_new_exec_mid_batch(0, &ok_success, &ok_status, &ok_submits);

    assert_int_equal(fail_success, 1);
    assert_int_equal(fail_status, DCU_EXEC_DONE);
    assert_int_equal(fail_submits, 32);
    assert_int_equal(ok_success, 1);
    assert_int_equal(ok_status, DCU_EXEC_DONE);
    assert_int_equal(ok_submits, 32);
    assert_int_equal(fail_success, ok_success);
}

/* 单表一次访问按容量分两批（两任务各 32 项，并集 64 > 32）。熔断与通道成功
 * 统计按整表访问结算一次：一批失败、一批成功算整表失败；全成功不得打开。 */
static bool _run_batched_visit_rounds(bool fail_first_batch, int rounds) {
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    assert_int_equal(dcu_inventory_add(&inv, "86000005", "dlt698", 9600, 1), 0);
    uint32_t s1[32], s2[32];
    for (int i = 0; i < 32; i++) {
        s1[i] = 0x3000u + (uint32_t)i;
        s2[i] = 0x4000u + (uint32_t)i;
    }
    assert_int_equal(dcu_poller_add_scheme(&poller, 1, 0, s1, 32), 0);
    assert_int_equal(dcu_poller_add_scheme(&poller, 2, 0, s2, 32), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 1, 0, 1), 0);
    assert_int_equal(dcu_poller_add_task(&poller, 2, 0, 2), 0);

    poller_sink_ops_t sink;
    memset(&sink, 0, sizeof(sink));
    assert_int_equal(dcu_poller_start(&poller, _src(), _bus(), sink), 0);

    _trace_reset();
    g_async_mode = 1;
    int next = 0;
    for (int r = 0; r < rounds; r++) {
        assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
        for (int iter = 0; iter < 4000; iter++) {
            if (next < g_pending_n) {
                int status = (fail_first_batch && g_trace_tag[next] >= 0x3000u &&
                              g_trace_tag[next] < 0x3020u)
                                 ? -1
                                 : 0;
                poller_read_done_cb cb = g_pending_cb[next];
                void* ud = g_pending_ud[next];
                next++;
                cb(ud, status, 220.0);
            } else {
                assert_int_equal(dcu_poller_step(&poller, &inv, 0), 0);
            }
            if (poller.tasks[0].monitor.status == DCU_EXEC_DONE &&
                poller.tasks[1].monitor.status == DCU_EXEC_DONE) {
                break;
            }
        }
        assert_int_equal(poller.tasks[0].monitor.status, DCU_EXEC_DONE);
        assert_int_equal(poller.tasks[1].monitor.status, DCU_EXEC_DONE);
    }

    bool open = dcu_poller_meter_breaker_open(&poller, "86000005");
    dcu_poller_destroy(&poller);
    _trace_reset();
    return open;
}

static void test_batched_visit_breaker_settles_once_per_round(void** state) {
    (void)state;
    /* 默认阈值 5：连续 5 轮整表失败才打开。 */
    assert_true(_run_batched_visit_rounds(true, 5));
    assert_false(_run_batched_visit_rounds(false, 5));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_task_and_scheme_add),
        cmocka_unit_test(test_add_scheme_bounds),
        cmocka_unit_test(test_chain_visits_each_meter_once),
        cmocka_unit_test(test_success_counts_only_due_enabled_tasks),
        cmocka_unit_test(test_success_counts_across_non_due_tick),
        cmocka_unit_test(test_dedup_overlapping_tasks),
        cmocka_unit_test(test_submit_failure_does_not_stall),
        cmocka_unit_test(test_breaker_skips_meter),
        cmocka_unit_test(test_channels_progress_independently),
        cmocka_unit_test(test_gc_retires_deleted_meter),
        cmocka_unit_test(test_execution_polls_each_meter_once),
        cmocka_unit_test(test_pending_capacity_starves_second_task),
        cmocka_unit_test(test_shared_tag_dedup_across_batches),
        cmocka_unit_test(test_capacity_batches_across_steps),
        cmocka_unit_test(test_capacity_batch_failure_fails_whole_meter),
        cmocka_unit_test(test_transient_disable_does_not_reload),
        cmocka_unit_test(test_reopen_drops_stale_pending),
        cmocka_unit_test(test_mixed_visit_does_not_count_success),
        cmocka_unit_test(test_breaker_cooldown_half_open),
        cmocka_unit_test(test_breaker_cooldown_probe_failure_rearms),
        cmocka_unit_test(test_reopen_resnapshots_and_ignores_stale_callbacks),
        cmocka_unit_test(test_mask_change_reopens_execution),
        cmocka_unit_test(test_overlapping_tasks_settle_on_shared_visit),
        cmocka_unit_test(test_reset_circuit_breaker_closes),
        cmocka_unit_test(test_reopen_late_callback_wrong_address),
        cmocka_unit_test(test_late_callback_uses_submit_protocol_for_scale),
        cmocka_unit_test(test_late_failure_does_not_retry_shifted_index),
        cmocka_unit_test(test_late_callback_before_revision_observed),
        cmocka_unit_test(test_late_callback_shift_does_not_fail_meter),
        cmocka_unit_test(test_spare_retry_dropped_after_slot_reuse),
        cmocka_unit_test(test_spare_retry_allowed_when_revision_unchanged),
        cmocka_unit_test(test_scheme_change_mid_batch_reopens_execution),
        cmocka_unit_test(test_add_task_does_not_reopen_active_execution),
        cmocka_unit_test(test_scheme_content_change_reopens_referencing_execution),
        cmocka_unit_test(test_new_exec_mid_batch_failure_not_tainted),
        cmocka_unit_test(test_batched_visit_breaker_settles_once_per_round),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
