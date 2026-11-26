#include <cmocka.h>
#include <sqlite3.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "dcu_698_freeze.h"
#include "dcu_698_axdr.h"
#include "dcu_698_freeze_cfg.h"
#include "dcu_inventory.h"
#include "dcu_router.h"
#include "dcu_storage.h"

typedef struct {
    uv_loop_t loop;
    dcu_storage_t storage;
    dcu_inventory_t inv;
    dcu_freeze_scheduler_t sched;
    char path[64];
} freeze_bench_t;

static time_t g_fake_epoch;
static int _fake_now(void *ctx, struct tm *out) {
    (void)ctx;
    time_t t = g_fake_epoch;
    return localtime_r(&t, out) ? 0 : -1;
}
static const dcu_clock_port_t g_fake_clock = {_fake_now, NULL, NULL};

static int _fake_read(void *ctx, uint32_t slot_id, uint32_t source_oad, double *out) {
    (void)ctx;
    *out = 10.0 + (double)slot_id + (source_oad == OAD_ACTIVE_ENERGY ? 0.0 : 1.0);
    return 0;
}

static uint64_t _epoch(int y, int mon, int day, int hour, int min, int sec) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_year = y - 1900;
    t.tm_mon = mon - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = min;
    t.tm_sec = sec;
    t.tm_isdst = 0;
    time_t v = mktime(&t);
    assert_true(v > 0);
    return (uint64_t)v;
}

static int _bench_init(freeze_bench_t *b) {
    memset(b, 0, sizeof(*b));
    strcpy(b->path, "/tmp/dcu_freeze_XXXXXX");
    int fd = mkstemp(b->path);
    if (fd < 0) return -1;
    close(fd);
    if (uv_loop_init(&b->loop) != 0) return -1;
    if (dcu_storage_init(&b->storage, &b->loop, b->path) != 0) return -1;
    if (dcu_inventory_init(&b->inv) != 0) return -1;
    if (dcu_inventory_add(&b->inv, "000000000001", "DLT645", 2400, 1) != 0) return -1;
    g_fake_epoch = (time_t)_epoch(2026, 11, 23, 0, 0, 0);
    dcu_freeze_scheduler_init(&b->sched, &b->storage, &b->inv, _fake_read, NULL, &g_fake_clock);
    static const uint32_t sources[] = {OAD_ACTIVE_ENERGY, OAD_VOLTAGE};
    dcu_freeze_scheduler_set_sources(&b->sched, sources, 2);
    return 0;
}

static void _bench_free(freeze_bench_t *b) {
    dcu_storage_destroy(&b->storage);
    uv_run(&b->loop, UV_RUN_NOWAIT);
    uv_loop_close(&b->loop);
    unlink(b->path);
    char side[80];
    snprintf(side, sizeof(side), "%s-wal", b->path);
    unlink(side);
    snprintf(side, sizeof(side), "%s-shm", b->path);
    unlink(side);
}

static uint16_t _rows_at(freeze_bench_t *b, uint32_t freeze_oad, uint64_t t) {
    dcu_freeze_record_t page[8];
    uint16_t count = 0;
    assert_int_equal(
        dcu_storage_query_freeze(&b->storage, "000000000001", freeze_oad, t, t, page, 8, &count),
        0);
    return count;
}

/* 启动不回补历史；跨日/月/结算日/年边界各写一条正确 freeze_time 的记录。 */
static void test_freeze_boundaries(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);

    uint64_t nov29_2359 = _epoch(2026, 11, 29, 23, 59, 59);
    assert_int_equal(dcu_freeze_tick(&b.sched, nov29_2359), 0); /* 首次只对齐，不回补 */

    uint64_t nov30 = _epoch(2026, 11, 30, 0, 0, 0);
    int written = dcu_freeze_tick(&b.sched, nov30);
    /* 5001/2/3/4：4 个 OI × 2 源 = 8 行；5005 在次月 1 日结算，5006 也在 12 月 1 日。 */
    assert_int_equal(written, 8);
    assert_int_equal(_rows_at(&b, 0x50050200, nov30), 0);
    assert_int_equal(_rows_at(&b, 0x50060200, nov30), 0);

    uint64_t dec1 = _epoch(2026, 12, 1, 0, 0, 0);
    written = dcu_freeze_tick(&b.sched, dec1);
    /* 5001/2/3/4/5/6：6×2=12；5005 结算 11 月。 */
    assert_int_equal(written, 12);
    assert_int_equal(_rows_at(&b, 0x50010200, dec1), 2);
    assert_int_equal(_rows_at(&b, 0x50040200, dec1), 2);
    assert_int_equal(_rows_at(&b, 0x50060200, dec1), 2);
    assert_int_equal(_rows_at(&b, 0x50050200, dec1), 2); /* 上月最后一天 24:00 */
    assert_int_equal(_rows_at(&b, 0x50070200, dec1), 0);
    assert_int_equal(_rows_at(&b, 0x50040200, dec1 + 1), 0); /* 时间戳就是边界，不是回调时刻 */

    assert_int_equal(dcu_freeze_tick(&b.sched, dec1), 0); /* 同一秒重入不重复 */
    assert_int_equal(_rows_at(&b, 0x50040200, dec1), 2);

    (void)dcu_freeze_tick(&b.sched, _epoch(2026, 12, 31, 23, 59, 59));
    assert_int_equal(_rows_at(&b, 0x50050200, _epoch(2026, 12, 31, 0, 0, 0)), 0);
    uint64_t jan1 = _epoch(2027, 1, 1, 0, 0, 0);
    written = dcu_freeze_tick(&b.sched, jan1);
    /* 5001/2/3/4/5/6/7：7 个 OI × 2 源 = 14；1 月 1 日结算 12 月。 */
    assert_int_equal(written, 14);
    assert_int_equal(_rows_at(&b, 0x50070200, jan1), 2);
    assert_int_equal(_rows_at(&b, 0x50060200, jan1), 2);
    assert_int_equal(_rows_at(&b, 0x50050200, jan1), 2);

    /* 切换类 OI 永不产生记录。 */
    assert_int_equal(_rows_at(&b, 0x50080200, dec1), 0);
    assert_int_equal(_rows_at(&b, 0x50110200, jan1), 0);
    _bench_free(&b);
}

/* 非整小时时区：小时边界必须按本地 tm 取整，UTC 取模会把 00:00 错成 23:30。 */
static void test_freeze_hour_boundary_uses_local_time(void **state) {
    (void)state;
    setenv("TZ", "Asia/Kolkata", 1);
    tzset();
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);

    uint64_t prime = _epoch(2026, 11, 30, 23, 59, 59);
    uint64_t midnight = _epoch(2026, 12, 1, 0, 0, 0);
    uint64_t utc_hour = midnight - 1800; /* IST 23:30 = UTC 整点 */
    assert_int_equal(dcu_freeze_tick(&b.sched, prime), 0);
    assert_true(dcu_freeze_tick(&b.sched, midnight) > 0);

    assert_int_equal(_rows_at(&b, 0x50030200, midnight), 2); /* 本地 00:00 */
    assert_int_equal(_rows_at(&b, 0x50030200, utc_hour), 0); /* 不是 UTC 整点 */
    assert_int_equal(_rows_at(&b, 0x50020200, midnight), 2); /* 本地整分 */
    assert_int_equal(_rows_at(&b, 0x50040200, midnight), 2); /* 本地零点 */
    _bench_free(&b);
    setenv("TZ", "UTC", 1);
    tzset();
}

static void test_next_boundary_aligns_to_second(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    uint64_t now = _epoch(2026, 11, 30, 0, 0, 5);
    assert_int_equal(dcu_freeze_tick(&b.sched, now), 0);
    uint64_t boundary = 0;
    assert_int_equal(dcu_freeze_next_boundary(&b.sched, now, &boundary), 1);
    assert_int_equal(boundary, now + 1); /* 5001 对齐到下一整秒 */
    _bench_free(&b);
}


/* 属性 3：默认集 → 方法 5 删除 → 记录列/持久化；方法 7/8 与周期=0 + 方法 3。 */
static void test_freeze_attr3_config_drives_records(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    const dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_find(&cfgs, 0x5004);
    assert_int_equal(cfg->configured, 0);
    assert_int_equal(cfg->count, 3);

    uint8_t a3[64];
    size_t len = 0;
    assert_int_equal(dcu_freeze_cfg_encode(cfg, a3, sizeof(a3), &len), 0);
    assert_int_equal(len, 2 + 3 * 13);
    assert_int_equal(a3[0], 0x01);
    assert_int_equal(a3[1], 3);
    assert_int_equal(a3[2], 0x02);
    assert_int_equal(a3[3], 3);

    dcu_698_freeze_bind(&b.storage, &cfgs, &b.sched);
    dcu_freeze_scheduler_set_configs(&b.sched, &cfgs);
    dcu_698_object_t obj;
    memset(&obj, 0, sizeof(obj));
    obj.oi = 0x5004;

    /* 方法 5：删除有功电能。 */
    uint8_t del[5] = {0x51, 0x00, 0x10, 0x02, 0x00};
    dcu_freeze_op_t op;
    assert_int_equal(dcu_698_freeze_decode_op(5, del, sizeof(del), &op), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 5, &op, NULL), 0);
    assert_int_equal(dcu_freeze_cfg_find(&cfgs, 0x5004)->count, 2);
    dcu_freeze_cfgs_t reloaded;
    assert_int_equal(dcu_freeze_cfgs_load(&reloaded, &b.storage), 0);
    assert_int_equal(dcu_freeze_cfg_find(&reloaded, 0x5004)->count, 2);

    /* 下一次日冻结只写剩下的两个源。 */
    uint64_t t = _epoch(2026, 12, 2, 0, 0, 0);
    assert_int_equal(dcu_freeze_tick(&b.sched, t - 1), 0);
    assert_true(dcu_freeze_tick(&b.sched, t) > 0);
    assert_int_equal(_rows_at(&b, 0x50040200, t), 2);
    dcu_freeze_record_t page[8];
    uint16_t n = 0;
    assert_int_equal(dcu_storage_query_freeze(&b.storage, "000000000001", 0x50040200, t, t, page, 8,
                                              &n),
                     0);
    for (uint16_t i = 0; i < n; i++) assert_true(page[i].source_oad != 0x00100200);

    /* 方法 8 清空：主站配置过就不再回落到默认集。 */
    dcu_freeze_op_t clr;
    memset(&clr, 0, sizeof(clr));
    uint8_t nul = 0x00;
    assert_int_equal(dcu_698_freeze_decode_op(8, &nul, 1, &clr), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 8, &clr, NULL), 0);
    assert_int_equal(dcu_freeze_cfg_find(&cfgs, 0x5004)->count, 0);
    assert_int_equal(dcu_freeze_cfgs_load(&reloaded, &b.storage), 0);
    assert_int_equal(dcu_freeze_cfg_find(&reloaded, 0x5004)->count, 0);

    /* 用方法 7 装回三源，周期 0：自动冻结关闭，方法 3 仍能立即冻结。 */
    uint8_t batch[2 + 3 * 13] = {0x01, 3};
    size_t off = 2;
    static const uint32_t oads[] = {0x00100200, 0x20000200, 0x20010200};
    for (int i = 0; i < 3; i++) {
        batch[off++] = 0x02;
        batch[off++] = 3;
        batch[off++] = 0x12;
        batch[off++] = 0;
        batch[off++] = 0;
        batch[off++] = 0x51;
        batch[off++] = (uint8_t)(oads[i] >> 24);
        batch[off++] = (uint8_t)(oads[i] >> 16);
        batch[off++] = (uint8_t)(oads[i] >> 8);
        batch[off++] = (uint8_t)oads[i];
        batch[off++] = 0x12;
        batch[off++] = 0;
        batch[off++] = 1;
    }
    dcu_freeze_op_t add;
    assert_int_equal(dcu_698_freeze_decode_op(7, batch, off, &add), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 7, &add, NULL), 0);
    assert_int_equal(dcu_freeze_cfg_find(&cfgs, 0x5004)->count, 3);
    (void)dcu_freeze_tick(&b.sched, t + 86400); /* 其它 OI 的默认节奏仍会触发 */
    assert_int_equal(_rows_at(&b, 0x50040200, t + 86400), 0); /* 5004 周期 0：不自动 */
    assert_int_equal(dcu_freeze_trigger(&b.sched, 0x5004), 3); /* 方法 3 立即冻结 */
    _bench_free(&b);
}


/* 失败路径必须回滚：删除不存在的 OAD 时，默认集物化不得留下部分行。 */
static void test_freeze_attr3_rollback_and_dar(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    dcu_698_freeze_bind(&b.storage, &cfgs, &b.sched);
    dcu_freeze_scheduler_set_configs(&b.sched, &cfgs);

    dcu_698_object_t obj;
    memset(&obj, 0, sizeof(obj));
    obj.oi = 0x5005;

    uint8_t del[5] = {0x51, 0x12, 0x34, 0x56, 0x78}; /* 不在默认集里 */
    dcu_freeze_op_t op;
    assert_int_equal(dcu_698_freeze_decode_op(5, del, sizeof(del), &op), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 5, &op, NULL), EDGE_SLAVE_SRC_NOT_FOUND);
    const dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_find(&cfgs, 0x5005);
    assert_int_equal(cfg->configured, 0);
    assert_int_equal(cfg->count, 3);
    dcu_freeze_cfgs_t reloaded;
    assert_int_equal(dcu_freeze_cfgs_load(&reloaded, &b.storage), 0);
    assert_int_equal(dcu_freeze_cfg_find(&reloaded, 0x5005)->configured, 0);
    assert_int_equal(dcu_freeze_cfg_find(&reloaded, 0x5005)->count, 3);

    uint8_t delayed[3] = {0x12, 0x00, 0x05};
    dcu_freeze_op_t trig;
    assert_int_equal(dcu_698_freeze_decode_op(3, delayed, sizeof(delayed), &trig), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 3, &trig, NULL), 0);
    assert_int_equal(_rows_at(&b, 0x50050200, (uint64_t)g_fake_epoch), 0);

    /* 未绑定存储/调度器：产品初始化故障 → DAR 2（TEMPORARY）。 */
    dcu_698_freeze_bind(NULL, &cfgs, &b.sched);
    uint8_t del_voltage[5] = {0x51, 0x20, 0x00, 0x02, 0x00};
    assert_int_equal(dcu_698_freeze_decode_op(5, del_voltage, sizeof(del_voltage), &op), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 5, &op, NULL), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(dcu_freeze_cfg_find(&cfgs, 0x5005)->count, 3);
    dcu_698_freeze_bind(&b.storage, &cfgs, &b.sched);
    _bench_free(&b);
}


static uint16_t _source_rows(freeze_bench_t *b, uint32_t oad, uint64_t t, uint32_t src) {
    dcu_freeze_record_t page[32];
    uint16_t n = 0;
    assert_int_equal(dcu_storage_query_freeze(&b->storage, "000000000001", oad, t, t, page, 32, &n),
                     0);
    uint16_t count = 0;
    for (uint16_t i = 0; i < n; i++)
        if (page[i].source_oad == src) count++;
    return count;
}

static void _empty_other_ois(dcu_freeze_cfgs_t *cfgs, uint16_t keep) {
    for (uint16_t oi = 0x5000; oi <= 0x5007; oi++) {
        if (oi == keep) continue;
        dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_mut(cfgs, oi);
        cfg->configured = true;
        cfg->count = 0;
    }
}

static size_t _put_item(uint8_t *buf, uint32_t period, uint32_t oad, uint32_t depth) {
    size_t off = 0;
    buf[off++] = 0x02;
    buf[off++] = 0x03;
    buf[off++] = 0x12;
    buf[off++] = (uint8_t)(period >> 8);
    buf[off++] = (uint8_t)period;
    buf[off++] = 0x51;
    buf[off++] = (uint8_t)(oad >> 24);
    buf[off++] = (uint8_t)(oad >> 16);
    buf[off++] = (uint8_t)(oad >> 8);
    buf[off++] = (uint8_t)oad;
    buf[off++] = 0x12;
    buf[off++] = (uint8_t)(depth >> 8);
    buf[off++] = (uint8_t)depth;
    return off;
}

static void test_freeze_per_entry_periods(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_mut(&cfgs, 0x5002);
    cfg->configured = true;
    cfg->count = 2;
    cfg->items[0] = (dcu_freeze_attr_t){OAD_ACTIVE_ENERGY, 1, 1};
    cfg->items[1] = (dcu_freeze_attr_t){OAD_VOLTAGE, 2, 1};
    dcu_freeze_scheduler_set_configs(&b.sched, &cfgs);

    uint64_t t0 = _epoch(2026, 12, 2, 0, 2, 59);
    assert_int_equal(dcu_freeze_tick(&b.sched, t0), 0);
    uint64_t t1 = _epoch(2026, 12, 2, 0, 3, 0);
    assert_true(dcu_freeze_tick(&b.sched, t1) > 0);
    assert_int_equal(_source_rows(&b, 0x50020200, t1, OAD_ACTIVE_ENERGY), 1);
    assert_int_equal(_source_rows(&b, 0x50020200, t1, OAD_VOLTAGE), 0);

    uint64_t t2 = _epoch(2026, 12, 2, 0, 4, 0);
    assert_true(dcu_freeze_tick(&b.sched, t2) > 0);
    assert_int_equal(_source_rows(&b, 0x50020200, t2, OAD_ACTIVE_ENERGY), 1);
    assert_int_equal(_source_rows(&b, 0x50020200, t2, OAD_VOLTAGE), 1);
    _bench_free(&b);
}

static void test_freeze_delayed_trigger(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    _empty_other_ois(&cfgs, 0x5003);
    dcu_698_freeze_bind(&b.storage, &cfgs, &b.sched);
    dcu_freeze_scheduler_set_configs(&b.sched, &cfgs);

    g_fake_epoch = (time_t)_epoch(2026, 12, 2, 0, 0, 0);
    dcu_698_object_t obj;
    memset(&obj, 0, sizeof(obj));
    obj.oi = 0x5003;
    uint8_t delayed[3] = {0x12, 0x00, 0x02};
    dcu_freeze_op_t op;
    assert_int_equal(dcu_698_freeze_decode_op(3, delayed, sizeof(delayed), &op), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 3, &op, NULL), 0);

    uint64_t now = (uint64_t)g_fake_epoch;
    assert_int_equal(dcu_freeze_tick(&b.sched, now), 0);
    assert_int_equal(dcu_freeze_tick(&b.sched, now + 1), 0);
    assert_int_equal(_source_rows(&b, 0x50030200, now + 1, OAD_ACTIVE_ENERGY), 0);
    assert_true(dcu_freeze_tick(&b.sched, now + 2) > 0);
    assert_int_equal(_source_rows(&b, 0x50030200, now + 2, OAD_ACTIVE_ENERGY), 1);
    assert_int_equal(dcu_freeze_tick(&b.sched, now + 2), 0);

    g_fake_epoch = (time_t)(now + 3);
    uint8_t immediate[3] = {0x12, 0x00, 0x00};
    assert_int_equal(dcu_698_freeze_decode_op(3, immediate, sizeof(immediate), &op), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 3, &op, NULL), 0);
    assert_int_equal(_source_rows(&b, 0x50030200, now + 3, OAD_ACTIVE_ENERGY), 1);
    _bench_free(&b);
}

static void test_freeze_depth_trim(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    assert_int_equal(dcu_inventory_add(&b.inv, "000000000002", "DLT645", 2400, 2), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    _empty_other_ois(&cfgs, 0x5001);
    dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_mut(&cfgs, 0x5001);
    cfg->configured = true;
    cfg->count = 2;
    cfg->items[0] = (dcu_freeze_attr_t){OAD_ACTIVE_ENERGY, 1, 2};
    cfg->items[1] = (dcu_freeze_attr_t){OAD_VOLTAGE, 1, 1};
    dcu_freeze_scheduler_set_configs(&b.sched, &cfgs);

    uint64_t t0 = _epoch(2026, 12, 2, 0, 0, 0);
    assert_int_equal(dcu_freeze_tick(&b.sched, t0), 0);
    for (int i = 1; i <= 3; i++) assert_true(dcu_freeze_tick(&b.sched, t0 + (uint64_t)i) > 0);

    dcu_freeze_record_t page[32];
    uint16_t n = 0;
    assert_int_equal(dcu_storage_query_freeze(&b.storage, "000000000001", 0x50010200, 0,
                                              t0 + 10, page, 32, &n),
                     0);
    uint16_t energy = 0, voltage = 0;
    for (uint16_t i = 0; i < n; i++) {
        if (page[i].source_oad == OAD_ACTIVE_ENERGY) {
            energy++;
            assert_true(page[i].freeze_time >= t0 + 2);
        } else if (page[i].source_oad == OAD_VOLTAGE) {
            voltage++;
            assert_int_equal(page[i].freeze_time, t0 + 3);
        }
    }
    assert_int_equal(energy, 2);
    assert_int_equal(voltage, 1);

    n = 0;
    assert_int_equal(dcu_storage_query_freeze(&b.storage, "000000000002", 0x50010200, 0,
                                              t0 + 10, page, 32, &n),
                     0);
    energy = voltage = 0;
    for (uint16_t i = 0; i < n; i++) {
        if (page[i].source_oad == OAD_ACTIVE_ENERGY) energy++;
        if (page[i].source_oad == OAD_VOLTAGE) voltage++;
    }
    assert_int_equal(energy, 2);
    assert_int_equal(voltage, 1);
    _bench_free(&b);
}

static void test_freeze_attr3_capacity_dar(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    dcu_698_freeze_bind(&b.storage, &cfgs, &b.sched);
    dcu_698_object_t obj;
    memset(&obj, 0, sizeof(obj));
    obj.oi = 0x5007;

    for (uint32_t i = 1; i <= 3; i++) {
        uint8_t buf[16];
        size_t len = _put_item(buf, 1, 0x30000000u + i, 1);
        dcu_freeze_op_t op;
        assert_int_equal(dcu_698_freeze_decode_op(4, buf, len, &op), 0);
        assert_int_equal(dcu_698_freeze_execute(&obj, 4, &op, NULL), 0);
    }
    assert_int_equal(dcu_freeze_cfg_find(&cfgs, 0x5007)->count, 6);

    uint8_t dedupe[2 + 3 * 13];
    dedupe[0] = 0x01;
    dedupe[1] = 3;
    size_t doff = 2;
    static const uint32_t dup_oads[] = {0x32000001, 0x32000002, 0x32000001};
    for (int i = 0; i < 3; i++) doff += _put_item(dedupe + doff, 1, dup_oads[i], 1);
    dcu_freeze_op_t batch_dup;
    assert_int_equal(dcu_698_freeze_decode_op(7, dedupe, doff, &batch_dup), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 7, &batch_dup, NULL), 0);
    assert_int_equal(dcu_freeze_cfg_find(&cfgs, 0x5007)->count, 8);
    dcu_freeze_cfgs_t reloaded;
    assert_int_equal(dcu_freeze_cfgs_load(&reloaded, &b.storage), 0);
    assert_int_equal(dcu_freeze_cfg_find(&reloaded, 0x5007)->count, 8);
    assert_false(dcu_freeze_cfg_find(&reloaded, 0x5007)->truncated);

    uint8_t ninth[16];
    size_t len = _put_item(ninth, 1, 0x30000006u, 1);
    dcu_freeze_op_t op;
    assert_int_equal(dcu_698_freeze_decode_op(4, ninth, len, &op), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 4, &op, NULL), EDGE_SLAVE_SRC_DENIED);
    assert_int_equal(dcu_freeze_cfgs_load(&reloaded, &b.storage), 0);
    assert_int_equal(dcu_freeze_cfg_find(&reloaded, 0x5007)->count, 8);
    assert_false(dcu_freeze_cfg_find(&reloaded, 0x5007)->truncated);

    uint8_t batch[2 + 9 * 13];
    batch[0] = 0x01;
    batch[1] = 9;
    size_t off = 2;
    for (uint32_t i = 0; i < 9; i++) off += _put_item(batch + off, 1, 0x40000000u + i, 1);
    dcu_freeze_op_t too_many;
    assert_int_equal(dcu_698_freeze_decode_op(7, batch, off, &too_many), EDGE_SLAVE_SRC_DENIED);

    uint8_t upsert[16];
    len = _put_item(upsert, 5, OAD_ACTIVE_ENERGY, 3);
    assert_int_equal(dcu_698_freeze_decode_op(4, upsert, len, &op), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 4, &op, NULL), 0);
    assert_int_equal(dcu_freeze_cfg_find(&cfgs, 0x5007)->count, 8);
    assert_int_equal(dcu_freeze_cfg_find(&cfgs, 0x5007)->items[0].period, 5);

    char sql[256];
    snprintf(sql, sizeof(sql),
             "INSERT OR REPLACE INTO dcu_freeze_attr3(oi, seq, oad, period, depth) "
             "VALUES(0x5007, 99, 0x50000001, 1, 1);");
    assert_int_equal(sqlite3_exec(b.storage.db, sql, NULL, NULL, NULL), SQLITE_OK);
    assert_int_equal(dcu_freeze_cfgs_load(&reloaded, &b.storage), 0);
    assert_int_equal(dcu_freeze_cfg_find(&reloaded, 0x5007)->count, 8);
    assert_true(dcu_freeze_cfg_find(&reloaded, 0x5007)->truncated);
    _bench_free(&b);
}



/* 写失败时 pending 保留并重试一次，不能收到 DAR 0 却静默丢掉冻结。 */
static void test_freeze_pending_retries_after_failure(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    _empty_other_ois(&cfgs, 0x5003);
    dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_mut(&cfgs, 0x5003);
    for (uint8_t i = 0; i < cfg->count; i++) cfg->items[i].period = 0;
    dcu_freeze_scheduler_set_configs(&b.sched, &cfgs);

    g_fake_epoch = (time_t)_epoch(2026, 12, 2, 0, 0, 0);
    uint64_t now = (uint64_t)g_fake_epoch;
    b.sched.storage = NULL;
    assert_int_equal(dcu_freeze_schedule(&b.sched, 0x5003, 2), 0);
    assert_int_equal(dcu_freeze_tick(&b.sched, now + 2), 0);
    assert_true(dcu_freeze_pending_next_due_ms(&b.sched) != 0);
    assert_int_equal(_rows_at(&b, 0x50030200, now + 2), 0);

    b.sched.storage = &b.storage;
    assert_true(dcu_freeze_tick(&b.sched, now + 2) > 0);
    assert_int_equal(dcu_freeze_pending_next_due_ms(&b.sched), 0);
    assert_int_equal(_source_rows(&b, 0x50030200, now + 2, OAD_ACTIVE_ENERGY), 1);
    _bench_free(&b);
}



/* Class 9 出口：属性 3 的 GET 编码与未实现属性的 DAR。 */
static void test_freeze_attr3_get_axdr_via_resolve(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    dcu_698_freeze_bind(&b.storage, &cfgs, &b.sched);
    dcu_freeze_scheduler_set_configs(&b.sched, &cfgs);

    uint8_t buf[64];
    size_t len = 0;
    assert_int_equal(dcu_north_resolve_axdr(NULL, 0x50040300, buf, sizeof(buf), &len), 0);
    assert_int_equal(buf[0], 0x01);
    assert_int_equal(buf[1], 3);
    size_t off = 2;
    for (int i = 0; i < 3; i++) {
        assert_int_equal(buf[off], 0x02);
        assert_int_equal(buf[off + 1], 3);
        assert_int_equal(buf[off + 2], 0x12);
        assert_int_equal(buf[off + 5], 0x51);
        assert_int_equal(buf[off + 10], 0x12);
        off += 13;
    }
    assert_int_equal(len, off);

    assert_int_equal(dcu_north_resolve_axdr(NULL, 0x50040200, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    _bench_free(&b);
}

/* Class 9 方法参数 DAR：形状 7、未知 OAD 6、清表参数非 NULL 7。 */
static void test_freeze_method_dar_shapes(void **state) {
    (void)state;
    freeze_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    dcu_freeze_cfgs_t cfgs;
    assert_int_equal(dcu_freeze_cfgs_load(&cfgs, &b.storage), 0);
    dcu_698_freeze_bind(&b.storage, &cfgs, &b.sched);
    dcu_freeze_scheduler_set_configs(&b.sched, &cfgs);
    dcu_698_object_t obj;
    memset(&obj, 0, sizeof(obj));
    obj.oi = 0x5006;

    uint8_t wrong_shape[] = {0x11, 0x01};
    dcu_freeze_op_t op;
    assert_int_equal(dcu_698_freeze_decode_op(4, wrong_shape, sizeof(wrong_shape), &op),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    uint8_t del_unknown[5] = {0x51, 0x12, 0x34, 0x56, 0x78};
    assert_int_equal(dcu_698_freeze_decode_op(5, del_unknown, sizeof(del_unknown), &op), 0);
    assert_int_equal(dcu_698_freeze_execute(&obj, 5, &op, NULL), EDGE_SLAVE_SRC_NOT_FOUND);

    uint8_t non_null[2] = {0x11, 0x01};
    assert_int_equal(dcu_698_freeze_decode_op(8, non_null, sizeof(non_null), &op),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    _bench_free(&b);
}

static int setup_tz(void **state) {
    (void)state;
    setenv("TZ", "UTC", 1);
    tzset();
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_freeze_boundaries),
        cmocka_unit_test(test_freeze_hour_boundary_uses_local_time),
        cmocka_unit_test(test_next_boundary_aligns_to_second),
        cmocka_unit_test(test_freeze_attr3_config_drives_records),
        cmocka_unit_test(test_freeze_attr3_rollback_and_dar),
        cmocka_unit_test(test_freeze_per_entry_periods),
        cmocka_unit_test(test_freeze_delayed_trigger),
        cmocka_unit_test(test_freeze_depth_trim),
        cmocka_unit_test(test_freeze_attr3_capacity_dar),
        cmocka_unit_test(test_freeze_pending_retries_after_failure),
        cmocka_unit_test(test_freeze_attr3_get_axdr_via_resolve),
        cmocka_unit_test(test_freeze_method_dar_shapes),
    };
    return cmocka_run_group_tests(tests, setup_tz, NULL);
}
