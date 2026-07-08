#include "edge_metrics.h"
#include "shmring.h"
#include <string.h>
#include <stdio.h>
#include <uv.h>

#define MAX_METRICS 128

typedef struct {
    uint32_t tag_id;
    char name[64];
    char desc[128];
    bool registered;
} metric_entry_t;

static metric_entry_t g_metrics[MAX_METRICS];
static uint32_t g_metric_count = 0;

int edge_metrics_register(uint32_t tag_id, const char *name, const char *description) {
    if (tag_id < EDGE_METRIC_TAG_BASE || tag_id > EDGE_METRIC_TAG_MAX) return -1;
    if (!name || g_metric_count >= MAX_METRICS) return -1;

    // 检查是否已注册
    for (uint32_t i = 0; i < g_metric_count; i++) {
        if (g_metrics[i].tag_id == tag_id) return 0; // 已注册，幂等
    }

    metric_entry_t *e = &g_metrics[g_metric_count++];
    e->tag_id = tag_id;
    strncpy(e->name, name, sizeof(e->name) - 1);
    if (description) strncpy(e->desc, description, sizeof(e->desc) - 1);
    e->registered = true;

    printf(" [Metrics] Registered: Tag=%u, Name=%s\n", tag_id, name);
    return 0;
}

void edge_metrics_report(datahub_t *hub, uint32_t tag_id, double value, uint64_t ts) {
    if (!hub) return;

    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.tag_id = tag_id;
    msg.type = PIPEMSG_TYPE_METRIC;
    msg.payload.val.f = value;
    msg.ts = ts > 0 ? ts : (uint64_t)uv_now(uv_default_loop());
    msg.quality = 0xC0; // Metric 质量位：可靠

    datahub_publish(hub, &msg);
}

int edge_metrics_register_fast(datahub_t *hub, uint32_t base_tag, uint32_t count) {
    (void)hub;
    if (base_tag < EDGE_METRIC_TAG_BASE || base_tag + count > EDGE_METRIC_TAG_MAX) return -1;
    for (uint32_t i = 0; i < count; i++) {
        char name[64];
        snprintf(name, sizeof(name), "metric.auto.%u", base_tag + i);
        edge_metrics_register(base_tag + i, name, "Auto-registered fast metric");
    }
    return 0;
}

uint32_t edge_metrics_count(void) {
    return g_metric_count;
}
