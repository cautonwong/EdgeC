/**
 * @file iems_health_monitor.c
 * @brief 硬件健康阈值门控：门控表 + 纯求值 + 采样源端口 + 1s 外壳。
 */

#include "iems_health_monitor.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>

#include "edge_log.h"
#include "iems_driver_adapter.h"
#include "pipe_msg.h"

/* 门控表是唯一真相；每条 gate 用 metric 选择指标（与数组位置无关），
 * tag_id 只决定写哪个点。 */
static const iems_health_gate_t k_health_gates[IEMS_HEALTH_GATE_COUNT] = {
    {.tag_id = IEMS_TAG_EMS_POWER,
     .metric = IEMS_HEALTH_METRIC_POWER,
     .threshold_pct = 0.0,
     .hold_ns = 0},
    {.tag_id = IEMS_TAG_EMS_CPU_HIGH,
     .metric = IEMS_HEALTH_METRIC_CPU,
     .threshold_pct = 90.0,
     .hold_ns = IEMS_HEALTH_HOLD_CPU_MEM_NS},
    {.tag_id = IEMS_TAG_EMS_MEM_HIGH,
     .metric = IEMS_HEALTH_METRIC_MEM,
     .threshold_pct = 90.0,
     .hold_ns = IEMS_HEALTH_HOLD_CPU_MEM_NS},
    {.tag_id = IEMS_TAG_EMS_EMMC_HIGH,
     .metric = IEMS_HEALTH_METRIC_EMMC,
     .threshold_pct = 95.0,
     .hold_ns = IEMS_HEALTH_HOLD_DISK_NS},
    {.tag_id = IEMS_TAG_EMS_SSD_HIGH,
     .metric = IEMS_HEALTH_METRIC_SSD,
     .threshold_pct = 95.0,
     .hold_ns = IEMS_HEALTH_HOLD_DISK_NS},
};

void iems_health_monitor_init(iems_health_monitor_t *mon, datahub_t *hub,
                              iems_health_source_t source) {
    if (!mon) return;
    memset(mon, 0, sizeof(*mon));
    mon->hub = hub;
    mon->source = source;
    memcpy(mon->gates, k_health_gates, sizeof(mon->gates));
}

uint32_t iems_health_evaluate(iems_health_gate_t *gates, size_t n,
                              const iems_health_metrics_t *m, uint64_t now_ns) {
    if (!gates || !m) return 0;
    uint32_t changed = 0;
    for (size_t i = 0; i < n && i < 32; i++) {
        iems_health_gate_t *g = &gates[i];
        bool want = g->alarm;
        bool evaluates = true;

        if (g->metric == IEMS_HEALTH_METRIC_POWER) {
            /* 电源没有百分比：false 立即置位，true 立即恢复 */
            want = !m->power_ok;
        } else {
            double value = 0.0;
            switch (g->metric) {
                case IEMS_HEALTH_METRIC_CPU:
                    value = m->cpu_pct;
                    break;
                case IEMS_HEALTH_METRIC_MEM:
                    value = m->mem_pct;
                    break;
                case IEMS_HEALTH_METRIC_EMMC:
                    value = m->emmc_pct;
                    break;
                case IEMS_HEALTH_METRIC_SSD:
                    evaluates = m->ssd_present; /* SSD 不在：忽略这扇门，状态保持 */
                    value = m->ssd_pct;
                    break;
                default:
                    evaluates = false;
                    break;
            }
            if (evaluates) {
                if (value > g->threshold_pct) {
                    if (g->over_since_ns == 0) g->over_since_ns = now_ns;
                    uint64_t over_ns = now_ns >= g->over_since_ns ? now_ns - g->over_since_ns : 0;
                    want = g->hold_ns == 0 || over_ns >= g->hold_ns;
                } else {
                    g->over_since_ns = 0;
                    want = false;
                }
            }
        }

        if (evaluates && want != g->alarm) {
            g->alarm = want;
            changed |= 1u << i;
        }
    }
    return changed;
}

/* ---------------- 外壳：定时读源、只在变化时写点 ---------------- */

static void _health_tick(uv_timer_t *timer) {
    iems_health_monitor_t *mon = (iems_health_monitor_t *)timer->data;
    (void)iems_health_monitor_step(mon, uv_hrtime());
}

int iems_health_monitor_start(iems_health_monitor_t *mon, uv_loop_t *loop, uint64_t period_ms) {
    if (!mon || !loop || period_ms == 0) return -1;
    if (mon->timer_initialized) return 0;
    uv_timer_init(loop, &mon->timer);
    mon->timer.data = mon;
    mon->timer_initialized = true;
    uv_timer_start(&mon->timer, _health_tick, period_ms, period_ms);
    return 0;
}

void iems_health_monitor_stop(iems_health_monitor_t *mon) {
    if (!mon || !mon->timer_initialized) return;
    uv_timer_stop(&mon->timer);
    uv_close((uv_handle_t *)&mon->timer, NULL);
    mon->timer_initialized = false;
}

int iems_health_monitor_step(iems_health_monitor_t *mon, uint64_t now_ns) {
    if (!mon || !mon->hub || !mon->source.read) return -1;
    iems_health_metrics_t metrics;
    memset(&metrics, 0, sizeof(metrics));
    if (mon->source.read(mon->source.ctx, &metrics) != 0) return 0; /* 本拍跳过 */

    uint32_t changed =
        iems_health_evaluate(mon->gates, IEMS_HEALTH_GATE_COUNT, &metrics, now_ns);
    int rc = 0;
    for (size_t i = 0; i < IEMS_HEALTH_GATE_COUNT; i++) {
        if ((changed & (1u << i)) == 0) continue;
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, mon->gates[i].tag_id, mon->gates[i].alarm ? 1.0 : 0.0,
                             uv_hrtime());
        if (datahub_publish(mon->hub, &msg) != 0) rc = -1;
    }
    return rc;
}

/* ---------------- Linux 采样源 ---------------- */

void iems_health_linux_source_init(iems_health_linux_source_t *src) {
    if (src) memset(src, 0, sizeof(*src));
}

static bool _read_file(const char *path, char *buf, size_t cap) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    size_t n = fread(buf, 1, cap - 1, f);
    fclose(f);
    buf[n] = '\0';
    return true;
}

int iems_health_parse_proc_stat(const char *text, iems_health_cpu_sample_t *out) {
    if (!text || !out) return -1;
    const char *p = text;
    while (*p == ' ' || *p == '\t') p++;
    /* 只认聚合行 "cpu ..."，不认 "cpu0" 等分行 */
    if (strncmp(p, "cpu", 3) != 0 || (p[3] != ' ' && p[3] != '\t')) return -1;
    p += 3;

    uint64_t vals[8] = {0};
    int count = 0;
    while (count < 8) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p < '0' || *p > '9') break;
        char *end = NULL;
        unsigned long long v = strtoull(p, &end, 10);
        if (end == p) break;
        vals[count++] = (uint64_t)v;
        p = end;
    }
    if (count < 4) return -1; /* user nice system idle 至少四项 */

    uint64_t total = 0;
    for (int i = 0; i < count; i++) total += vals[i];
    out->total = total;
    out->idle = vals[3] + (count > 4 ? vals[4] : 0); /* idle + iowait */
    return 0;
}

int iems_health_parse_meminfo(const char *text, double *out_pct) {
    if (!text || !out_pct) return -1;
    uint64_t total_kb = 0;
    uint64_t avail_kb = 0;
    bool have_total = false;
    bool have_avail = false;
    const char *line = text;
    while (line && *line) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            total_kb = (uint64_t)strtoull(line + 9, NULL, 10);
            have_total = total_kb > 0;
        } else if (strncmp(line, "MemAvailable:", 13) == 0) {
            avail_kb = (uint64_t)strtoull(line + 13, NULL, 10);
            have_avail = true;
        }
        const char *nl = strchr(line, '\n');
        line = nl ? nl + 1 : NULL;
    }
    if (!have_total || !have_avail) return -1;
    if (avail_kb > total_kb) avail_kb = total_kb;
    *out_pct = 100.0 * (double)(total_kb - avail_kb) / (double)total_kb;
    return 0;
}

double iems_health_cpu_pct(const iems_health_cpu_sample_t *prev,
                           const iems_health_cpu_sample_t *cur) {
    if (!prev || !cur || cur->total <= prev->total) return 0.0;
    uint64_t dtotal = cur->total - prev->total;
    uint64_t didle = cur->idle >= prev->idle ? cur->idle - prev->idle : 0;
    if (didle > dtotal) didle = dtotal;
    return 100.0 * (double)(dtotal - didle) / (double)dtotal;
}

double iems_health_disk_pct(uint64_t total_bytes, uint64_t avail_bytes) {
    if (total_bytes == 0) return 0.0;
    if (avail_bytes > total_bytes) avail_bytes = total_bytes;
    return 100.0 * (double)(total_bytes - avail_bytes) / (double)total_bytes;
}

static void _warn_limited(const char *what, int err) {
    static uint64_t last_ns;
    static int last_err;
    uint64_t now = uv_hrtime();
    if (now - last_ns < 60ULL * 1000000000ULL && err == last_err) return;
    last_ns = now;
    last_err = err;
    LOG_WARN(LOG_CAT_CORE, "IEMS health: %s failed (%s); skipping this tick", what,
             strerror(err));
}

static int _linux_read(void *ctx, iems_health_metrics_t *out) {
    iems_health_linux_source_t *src = (iems_health_linux_source_t *)ctx;
    if (!src || !out) return -1;
    /* /proc/stat 聚合行约 100B；/proc/meminfo 的前几行（含 MemAvailable）也在
     * 512B 内。截断只会让解析失败并跳过该拍，不会读越界。 */
    char buf[512];

    if (!_read_file("/proc/stat", buf, sizeof(buf))) return -1;
    iems_health_cpu_sample_t cur;
    if (iems_health_parse_proc_stat(buf, &cur) != 0) return -1;
    double cpu_pct = 0.0;
    if (src->have_prev) {
        iems_health_cpu_sample_t prev = {.total = src->prev_total, .idle = src->prev_idle};
        cpu_pct = iems_health_cpu_pct(&prev, &cur);
    }
    src->prev_total = cur.total;
    src->prev_idle = cur.idle;
    src->have_prev = true;

    if (!_read_file("/proc/meminfo", buf, sizeof(buf))) return -1;
    double mem_pct = 0.0;
    if (iems_health_parse_meminfo(buf, &mem_pct) != 0) return -1;

    struct statvfs vfs;
    if (statvfs("/", &vfs) != 0) return -1;
    double emmc_pct = iems_health_disk_pct((uint64_t)vfs.f_blocks * (uint64_t)vfs.f_frsize,
                                           (uint64_t)vfs.f_bavail * (uint64_t)vfs.f_frsize);

    bool ssd_present = false;
    double ssd_pct = 0.0;
    if (statvfs("/mnt/ssd", &vfs) == 0) {
        ssd_present = true;
        ssd_pct = iems_health_disk_pct((uint64_t)vfs.f_blocks * (uint64_t)vfs.f_frsize,
                                       (uint64_t)vfs.f_bavail * (uint64_t)vfs.f_frsize);
    } else if (errno != ENOENT) {
        _warn_limited("/mnt/ssd statvfs", errno);
        return -1;
    }

    out->power_ok = true; /* EdgeC 还没有电源 HAL（P6-02b §3 残余） */
    out->cpu_pct = cpu_pct;
    out->mem_pct = mem_pct;
    out->emmc_pct = emmc_pct;
    out->ssd_pct = ssd_pct;
    out->ssd_present = ssd_present;
    return 0;
}

iems_health_source_t iems_health_linux_source_port(iems_health_linux_source_t *src) {
    return (iems_health_source_t){.read = _linux_read, .ctx = src};
}
