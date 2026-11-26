/**
 * @file test_iems_health_monitor.c
 * @brief P6-02b 硬件健康门控：门控表/纯求值/采样源端口 + 真实 DataHub 写点。
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "datahub.h"
#include "datahub_priv.h"
#include "iems_driver_adapter.h"
#include "iems_health_monitor.h"
#include "iems_point_model.h"
#include "pipe_msg.h"

typedef struct {
    iems_health_metrics_t metrics;
    int reads;
    bool skip;
} fake_source_t;

static int fake_read(void *ctx, iems_health_metrics_t *out) {
    fake_source_t *f = (fake_source_t *)ctx;
    f->reads++;
    if (f->skip) return -1;
    *out = f->metrics;
    return 0;
}

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    iems_health_monitor_t mon;
    fake_source_t fake;
    char shm_name[64];
} health_fixture_t;

static int setup(void **state) {
    health_fixture_t *f = calloc(1, sizeof(*f));
    assert_non_null(f);
    assert_int_equal(uv_loop_init(&f->loop), 0);
    snprintf(f->shm_name, sizeof(f->shm_name), "IEMS_HEALTH_TEST_%d", (int)getpid());
    assert_int_equal(datahub_init(&f->hub, &f->loop, f->shm_name, 64, 9216, NULL, 0, NULL, NULL,
                                  NULL),
                     0);
    datahub_set_tag_mapper_v2(&f->hub, iems_tag_map_fn);
    f->fake.metrics.power_ok = true;
    iems_health_source_t source = {.read = fake_read, .ctx = &f->fake};
    iems_health_monitor_init(&f->mon, &f->hub, source);
    *state = f;
    return 0;
}

static int teardown(void **state) {
    health_fixture_t *f = (health_fixture_t *)*state;
    if (!f) return 0;
    iems_health_monitor_stop(&f->mon);
    datahub_destroy(&f->hub);
    uv_run(&f->loop, UV_RUN_NOWAIT);
    uv_loop_close(&f->loop);
    free(f);
    return 0;
}

static uint64_t publish_count(health_fixture_t *f) {
    datahub_backend_stats_t stats;
    assert_int_equal(datahub_get_stats(&f->hub, &stats), 0);
    return stats.total_publishes;
}

static double shadow_value(health_fixture_t *f, uint32_t tag_id) {
    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    assert_int_equal(datahub_read_shadow(&f->hub, tag_id, &msg), 0);
    return msg.payload.val.f;
}

/* A1: 电源掉电一拍置位写 1，恢复一拍写 0；重复拍不重复写；源跳过时保留状态。 */
static void test_power_alarm_sets_and_clears_once(void **state) {
    health_fixture_t *f = (health_fixture_t *)*state;
    uint64_t now = 1000000;

    f->fake.metrics.power_ok = false;
    assert_int_equal(iems_health_monitor_step(&f->mon, now), 0);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_POWER), 1.0, 0.0);
    assert_int_equal(publish_count(f), 1);

    assert_int_equal(iems_health_monitor_step(&f->mon, now + 1000), 0);
    assert_int_equal(publish_count(f), 1);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_POWER), 1.0, 0.0);

    /* 源本拍失败：跳过，不发布也不改状态 */
    f->fake.skip = true;
    assert_int_equal(iems_health_monitor_step(&f->mon, now + 2000), 0);
    assert_int_equal(publish_count(f), 1);
    f->fake.skip = false;

    f->fake.metrics.power_ok = true;
    assert_int_equal(iems_health_monitor_step(&f->mon, now + 3000), 0);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_POWER), 0.0, 0.0);
    assert_int_equal(publish_count(f), 2);

    assert_int_equal(iems_health_monitor_step(&f->mon, now + 4000), 0);
    assert_int_equal(publish_count(f), 2);
    assert_int_equal(f->fake.reads, 5);
}

/* A2: CPU >90 严格；299s 不置位，满 300s 置位写 1；降到 89.9 立即恢复写 0。 */
static void test_cpu_hold_then_recover(void **state) {
    health_fixture_t *f = (health_fixture_t *)*state;
    iems_health_gate_t *gate = &f->mon.gates[1];
    assert_int_equal(gate->tag_id, IEMS_TAG_EMS_CPU_HIGH);
    assert_true(gate->hold_ns == IEMS_HEALTH_HOLD_CPU_MEM_NS);
    assert_true(fabs(gate->threshold_pct - 90.0) < 1e-9);

    const uint64_t t0 = 1000000000ULL;
    f->fake.metrics.cpu_pct = 90.1;
    assert_int_equal(iems_health_monitor_step(&f->mon, t0), 0);
    assert_int_equal(publish_count(f), 0);
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + gate->hold_ns - 1), 0);
    assert_int_equal(publish_count(f), 0);
    assert_float_equal(shadow_value(f, gate->tag_id), 0.0, 0.0);

    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + gate->hold_ns), 0);
    assert_int_equal(publish_count(f), 1);
    assert_float_equal(shadow_value(f, gate->tag_id), 1.0, 0.0);

    /* 持续超阈不重复写 */
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + gate->hold_ns + 1000), 0);
    assert_int_equal(publish_count(f), 1);

    f->fake.metrics.cpu_pct = 89.9;
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + gate->hold_ns + 2000), 0);
    assert_int_equal(publish_count(f), 2);
    assert_float_equal(shadow_value(f, gate->tag_id), 0.0, 0.0);

    /* 边界严格大于：90.0 持续满 hold 也不置位 */
    const uint64_t t1 = t0 + gate->hold_ns + 3000;
    f->fake.metrics.cpu_pct = 90.0;
    assert_int_equal(iems_health_monitor_step(&f->mon, t1), 0);
    assert_int_equal(iems_health_monitor_step(&f->mon, t1 + gate->hold_ns), 0);
    assert_int_equal(publish_count(f), 2);
    assert_float_equal(shadow_value(f, gate->tag_id), 0.0, 0.0);
}

/* A3: 内存阈值/持续时间语义与 CPU 相同。 */
static void test_mem_hold_then_recover(void **state) {
    health_fixture_t *f = (health_fixture_t *)*state;
    iems_health_gate_t *gate = &f->mon.gates[2];
    assert_int_equal(gate->tag_id, IEMS_TAG_EMS_MEM_HIGH);
    assert_true(gate->hold_ns == IEMS_HEALTH_HOLD_CPU_MEM_NS);

    const uint64_t t0 = 1000000000ULL;
    f->fake.metrics.mem_pct = 90.1;
    assert_int_equal(iems_health_monitor_step(&f->mon, t0), 0);
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + gate->hold_ns - 1), 0);
    assert_int_equal(publish_count(f), 0);
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + gate->hold_ns), 0);
    assert_int_equal(publish_count(f), 1);
    assert_float_equal(shadow_value(f, gate->tag_id), 1.0, 0.0);

    f->fake.metrics.mem_pct = 89.9;
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + gate->hold_ns + 1000), 0);
    assert_int_equal(publish_count(f), 2);
    assert_float_equal(shadow_value(f, gate->tag_id), 0.0, 0.0);
}

/* A4: eMMC/SSD 95.1% 满 24h 才置位；95.0% 不算超；SSD 不存在时不告警。 */
static void test_disk_hold_present_and_absent(void **state) {
    health_fixture_t *f = (health_fixture_t *)*state;
    iems_health_gate_t *emmc = &f->mon.gates[3];
    iems_health_gate_t *ssd = &f->mon.gates[4];
    assert_int_equal(emmc->tag_id, IEMS_TAG_EMS_EMMC_HIGH);
    assert_int_equal(ssd->tag_id, IEMS_TAG_EMS_SSD_HIGH);
    assert_true(emmc->hold_ns == IEMS_HEALTH_HOLD_DISK_NS);

    const uint64_t t0 = 1000000000ULL;
    f->fake.metrics.emmc_pct = 95.1;
    f->fake.metrics.ssd_present = false;
    f->fake.metrics.ssd_pct = 99.0;
    assert_int_equal(iems_health_monitor_step(&f->mon, t0), 0);
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + emmc->hold_ns - 1), 0);
    assert_int_equal(publish_count(f), 0);
    assert_float_equal(shadow_value(f, emmc->tag_id), 0.0, 0.0);
    assert_float_equal(shadow_value(f, ssd->tag_id), 0.0, 0.0);

    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + emmc->hold_ns), 0);
    assert_int_equal(publish_count(f), 1);
    assert_float_equal(shadow_value(f, emmc->tag_id), 1.0, 0.0);
    /* SSD 缺席：即使 pct 很高也不告警 */
    assert_float_equal(shadow_value(f, ssd->tag_id), 0.0, 0.0);

    /* 严格大于：95.0 立刻恢复 */
    f->fake.metrics.emmc_pct = 95.0;
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + emmc->hold_ns + 1000), 0);
    assert_int_equal(publish_count(f), 2);
    assert_float_equal(shadow_value(f, emmc->tag_id), 0.0, 0.0);

    /* SSD 出现：重新走 hold */
    const uint64_t t1 = t0 + emmc->hold_ns + 2000;
    f->fake.metrics.emmc_pct = 10.0;
    f->fake.metrics.ssd_present = true;
    f->fake.metrics.ssd_pct = 95.1;
    assert_int_equal(iems_health_monitor_step(&f->mon, t1), 0);
    assert_int_equal(iems_health_monitor_step(&f->mon, t1 + ssd->hold_ns - 1), 0);
    assert_int_equal(publish_count(f), 2);
    assert_int_equal(iems_health_monitor_step(&f->mon, t1 + ssd->hold_ns), 0);
    assert_int_equal(publish_count(f), 3);
    assert_float_equal(shadow_value(f, ssd->tag_id), 1.0, 0.0);

    f->fake.metrics.ssd_pct = 95.0;
    assert_int_equal(iems_health_monitor_step(&f->mon, t1 + ssd->hold_ns + 1000), 0);
    assert_int_equal(publish_count(f), 4);
    assert_float_equal(shadow_value(f, ssd->tag_id), 0.0, 0.0);
}

/* A5: 每个变化的门只写自己的点（tag 身份固定），未变化不写；五个点都能在
 * 真 DataHub 读到 1/0。 */
static void test_all_gates_publish_once_on_change(void **state) {
    health_fixture_t *f = (health_fixture_t *)*state;
    const uint64_t t0 = 1000000000ULL;
    f->fake.metrics.power_ok = false;
    f->fake.metrics.cpu_pct = 91.0;
    f->fake.metrics.mem_pct = 10.0;
    f->fake.metrics.emmc_pct = 96.0;
    f->fake.metrics.ssd_present = false;

    assert_int_equal(iems_health_monitor_step(&f->mon, t0), 0);
    assert_int_equal(publish_count(f), 1); /* 只有电源即时置位 */
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_POWER), 1.0, 0.0);

    const uint64_t t1 = t0 + IEMS_HEALTH_HOLD_CPU_MEM_NS;
    assert_int_equal(iems_health_monitor_step(&f->mon, t1), 0);
    assert_int_equal(publish_count(f), 2); /* CPU 到期；内存未超阈不能写 */
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_CPU_HIGH), 1.0, 0.0);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_MEM_HIGH), 0.0, 0.0);

    const uint64_t t2 = t0 + IEMS_HEALTH_HOLD_DISK_NS;
    assert_int_equal(iems_health_monitor_step(&f->mon, t2), 0);
    assert_int_equal(publish_count(f), 3); /* eMMC 到期；SSD 不在不写 */
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_EMMC_HIGH), 1.0, 0.0);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_SSD_HIGH), 0.0, 0.0);

    /* 全部已告警且指标不变：一拍都不写 */
    assert_int_equal(iems_health_monitor_step(&f->mon, t2 + 1000), 0);
    assert_int_equal(publish_count(f), 3);

    /* 内存与 SSD 各自到期，仍只写自己的点 */
    const uint64_t t3 = t2 + 2000;
    f->fake.metrics.mem_pct = 91.0;
    f->fake.metrics.ssd_present = true;
    f->fake.metrics.ssd_pct = 96.0;
    assert_int_equal(iems_health_monitor_step(&f->mon, t3), 0);
    assert_int_equal(publish_count(f), 3);
    assert_int_equal(iems_health_monitor_step(&f->mon, t3 + IEMS_HEALTH_HOLD_CPU_MEM_NS), 0);
    assert_int_equal(publish_count(f), 4);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_MEM_HIGH), 1.0, 0.0);
    assert_int_equal(iems_health_monitor_step(&f->mon, t3 + IEMS_HEALTH_HOLD_DISK_NS), 0);
    assert_int_equal(publish_count(f), 5);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_SSD_HIGH), 1.0, 0.0);

    assert_int_equal(iems_health_monitor_step(&f->mon, t3 + IEMS_HEALTH_HOLD_DISK_NS + 1000), 0);
    assert_int_equal(publish_count(f), 5);
}

/* A6: 合成 /proc 文本与 statvfs 计算的纯函数；真实 Linux 源读本机。 */
static void test_parsers_and_real_source(void **state) {
    (void)state;
    const char *prev_text = "cpu  100 0 100 700 0 50 50 0 0 0\ncpu0 1 2 3 4\n";
    const char *cur_text = "cpu  200 0 200 1100 0 100 100 0 0 0\ncpu0 1 2 3 4\n";
    iems_health_cpu_sample_t prev, cur;
    assert_int_equal(iems_health_parse_proc_stat(prev_text, &prev), 0);
    assert_int_equal(prev.total, 1000);
    assert_int_equal(prev.idle, 700);
    assert_int_equal(iems_health_parse_proc_stat(cur_text, &cur), 0);
    assert_int_equal(cur.total, 1700);
    assert_int_equal(cur.idle, 1100);
    const double busy = 100.0 * 300.0 / 700.0;
    assert_true(fabs(iems_health_cpu_pct(&prev, &cur) - busy) < 1e-9);
    assert_float_equal(iems_health_cpu_pct(&cur, &cur), 0.0, 0.0);
    assert_int_equal(iems_health_parse_proc_stat("cpu0 1 2 3 4", &cur), -1);
    assert_int_equal(iems_health_parse_proc_stat(NULL, &cur), -1);

    const char *mem_text =
        "MemTotal:       1000000 kB\nMemFree:          1 kB\nMemAvailable:    250000 kB\n";
    double mem_pct = 0.0;
    assert_int_equal(iems_health_parse_meminfo(mem_text, &mem_pct), 0);
    assert_float_equal(mem_pct, 75.0, 1e-9);
    assert_int_equal(iems_health_parse_meminfo("MemTotal: 1000 kB\n", &mem_pct), -1);
    assert_int_equal(iems_health_parse_meminfo(NULL, &mem_pct), -1);

    assert_float_equal(iems_health_disk_pct(1000, 50), 95.0, 1e-9);
    assert_float_equal(iems_health_disk_pct(1000, 1000), 0.0, 1e-9);
    assert_float_equal(iems_health_disk_pct(0, 0), 0.0, 1e-9);

    iems_health_linux_source_t src;
    iems_health_linux_source_init(&src);
    iems_health_source_t port = iems_health_linux_source_port(&src);
    assert_non_null(port.read);
    iems_health_metrics_t m;
    memset(&m, 0, sizeof(m));
    assert_int_equal(port.read(port.ctx, &m), 0);
    assert_true(m.power_ok);
    assert_true(m.cpu_pct >= 0.0 && m.cpu_pct <= 100.0);
    assert_true(m.mem_pct > 0.0 && m.mem_pct <= 100.0);
    assert_true(m.emmc_pct >= 0.0 && m.emmc_pct <= 100.0);
    if (m.ssd_present) assert_true(m.ssd_pct >= 0.0 && m.ssd_pct <= 100.0);
}

/* A7: 5 个系统级点的元数据与 UA 名字可查；无设备槽（slot = -1）。 */
static void test_health_points_metadata_and_names(void **state) {
    (void)state;
    static const struct {
        uint32_t tag_id;
        const char *name;
    } points[] = {
        {IEMS_TAG_EMS_POWER, "EMS.EMSPower"},
        {IEMS_TAG_EMS_CPU_HIGH, "EMS.EMSCpuHigh"},
        {IEMS_TAG_EMS_MEM_HIGH, "EMS.EMSMemHigh"},
        {IEMS_TAG_EMS_EMMC_HIGH, "EMS.EmmcDiskHigh"},
        {IEMS_TAG_EMS_SSD_HIGH, "EMS.SsdDiskHigh"},
    };
    for (size_t i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
        const edge_tag_meta_t *meta = iems_get_tag_meta(points[i].tag_id);
        assert_non_null(meta);
        assert_string_equal(meta->category, "HEALTH");
        assert_float_equal(meta->min, 0.0, 0.0);
        assert_float_equal(meta->max, 1.0, 0.0);
        assert_false(meta->persist);
        assert_true(meta->upload);

        uint32_t tag = 0;
        assert_int_equal(iems_point_lookup_name(points[i].name, &tag), 0);
        assert_int_equal(tag, points[i].tag_id);
        const char *name = NULL;
        assert_int_equal(iems_point_lookup_tag(points[i].tag_id, &name), 0);
        assert_string_equal(name, points[i].name);

        uint32_t slot = 0xA5A5A5A5u;
        assert_int_equal(iems_point_slot(points[i].tag_id, &slot), -1);
        assert_int_equal(slot, 0xA5A5A5A5u);
    }
}

/* A7 补充：每条 gate 显式绑定 metric，metric 与 tag 一一对应（顺序不承载语义）。 */
static void test_gate_table_binds_metric_to_tag(void **state) {
    (void)state;
    iems_health_monitor_t mon;
    memset(&mon, 0, sizeof(mon));
    iems_health_source_t none = {0};
    iems_health_monitor_init(&mon, NULL, none);

    static const struct {
        uint32_t tag_id;
        iems_health_metric_t metric;
        double threshold_pct;
    } want[] = {
        {IEMS_TAG_EMS_POWER, IEMS_HEALTH_METRIC_POWER, 0.0},
        {IEMS_TAG_EMS_CPU_HIGH, IEMS_HEALTH_METRIC_CPU, 90.0},
        {IEMS_TAG_EMS_MEM_HIGH, IEMS_HEALTH_METRIC_MEM, 90.0},
        {IEMS_TAG_EMS_EMMC_HIGH, IEMS_HEALTH_METRIC_EMMC, 95.0},
        {IEMS_TAG_EMS_SSD_HIGH, IEMS_HEALTH_METRIC_SSD, 95.0},
    };
    for (size_t i = 0; i < IEMS_HEALTH_GATE_COUNT; i++) {
        assert_int_equal(mon.gates[i].tag_id, want[i].tag_id);
        assert_int_equal(mon.gates[i].metric, want[i].metric);
        assert_true(fabs(mon.gates[i].threshold_pct - want[i].threshold_pct) < 1e-9);
        assert_false(mon.gates[i].alarm);
        assert_int_equal(mon.gates[i].over_since_ns, 0);
        for (size_t j = i + 1; j < IEMS_HEALTH_GATE_COUNT; j++) {
            assert_int_equal(mon.gates[i].metric != mon.gates[j].metric, 1);
        }
    }
}

/* 顺序无关：运行时交换 CPU/MEM 两门，各自仍按 metric 写自己的 tag。 */
static void test_gate_order_does_not_change_tag_mapping(void **state) {
    health_fixture_t *f = (health_fixture_t *)*state;
    iems_health_gate_t tmp = f->mon.gates[1];
    f->mon.gates[1] = f->mon.gates[2];
    f->mon.gates[2] = tmp;

    const uint64_t t0 = 1000000000ULL;
    f->fake.metrics.cpu_pct = 91.0;
    f->fake.metrics.mem_pct = 10.0;
    assert_int_equal(iems_health_monitor_step(&f->mon, t0), 0);
    assert_int_equal(iems_health_monitor_step(&f->mon, t0 + IEMS_HEALTH_HOLD_CPU_MEM_NS), 0);

    assert_int_equal(publish_count(f), 1);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_CPU_HIGH), 1.0, 0.0);
    assert_float_equal(shadow_value(f, IEMS_TAG_EMS_MEM_HIGH), 0.0, 0.0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_power_alarm_sets_and_clears_once, setup, teardown),
        cmocka_unit_test_setup_teardown(test_cpu_hold_then_recover, setup, teardown),
        cmocka_unit_test_setup_teardown(test_mem_hold_then_recover, setup, teardown),
        cmocka_unit_test_setup_teardown(test_disk_hold_present_and_absent, setup, teardown),
        cmocka_unit_test_setup_teardown(test_all_gates_publish_once_on_change, setup, teardown),
        cmocka_unit_test_setup_teardown(test_gate_order_does_not_change_tag_mapping, setup,
                                        teardown),
        cmocka_unit_test(test_parsers_and_real_source),
        cmocka_unit_test(test_health_points_metadata_and_names),
        cmocka_unit_test(test_gate_table_binds_metric_to_tag),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
