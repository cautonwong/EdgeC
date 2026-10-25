#include "dcu_bus_source.h"

#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "edge_api.h"

#define DCU_BUS_SLOTS 8192

typedef struct {
    uint32_t meter_idx;
    uint32_t tag_id;
    uint64_t submit_ts;
    int ready;
    int status;
    uint8_t used;
} bus_slot_t;

struct dcu_bus_source_s {
    datahub_t *hub;
    const dcu_inventory_t *inv;
    uint64_t timeout_ms;
    bus_slot_t slots[DCU_BUS_SLOTS];
};

static uint64_t _now_ms(void) { return uv_hrtime() / 1000000ULL; }

static bus_slot_t *_slot(dcu_bus_source_t *bs, uint32_t meter_idx, uint32_t tag_id) {
    uint32_t h = (uint32_t)((meter_idx * 2654435761u) ^ (tag_id * 40503u)) % DCU_BUS_SLOTS;
    for (uint32_t n = 0; n < DCU_BUS_SLOTS; n++) {
        bus_slot_t *s = &bs->slots[(h + n) % DCU_BUS_SLOTS];
        if (!s->used) {
            s->used = 1;
            s->meter_idx = meter_idx;
            s->tag_id = tag_id;
            return s;
        }
        if (s->meter_idx == meter_idx && s->tag_id == tag_id) return s;
    }
    return NULL;
}

/* 总线程回调：只打标记，取值留给 read_value 从 shadow 拿 */
static void _on_done(int status, const pipe_msg_t *result, void *user_data) {
    (void)result;
    bus_slot_t *s = (bus_slot_t *)user_data;
    if (!s) return;
    s->status = status;
    s->ready = 1;
}

static int _read_value(void *ctx, uint32_t meter_idx, uint32_t tag_id, double *out_val) {
    dcu_bus_source_t *bs = (dcu_bus_source_t *)ctx;
    if (!bs || !out_val || !bs->inv || meter_idx >= bs->inv->count) return -1;

    bus_slot_t *s = _slot(bs, meter_idx, tag_id);
    if (!s) return -1;

    if (s->ready) {
        int st = s->status;
        s->ready = 0;
        s->status = 0;
        s->submit_ts = 0;
        if (st != 0) return -1;

        pipe_msg_t msg;
        if (datahub_read_shadow_device(bs->hub, tag_id, meter_idx, &msg) == 0 && msg.ts != 0) {
            *out_val = msg.payload.val.f;
            return 0;
        }
        return -1; /* 应答成功但没落 shadow：本轮算失败，不回 0.0 */
    }

    if (s->submit_ts != 0) {
        if (_now_ms() - s->submit_ts <= bs->timeout_ms) return POLL_READ_PENDING;
        s->submit_ts = 0;
        return -1; /* 超时：判本轮失败，不回 0.0 */
    }

    const production_meter_t *m = &bs->inv->meters[meter_idx];
    s->status = 0;
    s->ready = 0;
    s->submit_ts = _now_ms();
    if (edge_api_read_tag((int)m->channel_id, m->protocol, meter_idx, tag_id, _on_done, s) != 0) {
        s->submit_ts = 0;
        return -1;
    }
    return POLL_READ_PENDING;
}

dcu_bus_source_t *dcu_bus_source_start(datahub_t *hub, const dcu_inventory_t *inv,
                                       uint32_t timeout_ms) {
    dcu_bus_source_t *bs = (dcu_bus_source_t *)calloc(1, sizeof(*bs));
    if (!bs) return NULL;
    bs->hub = hub;
    bs->inv = inv;
    bs->timeout_ms = timeout_ms ? timeout_ms : 4000;
    return bs;
}

void dcu_bus_source_stop(dcu_bus_source_t *bs) { free(bs); }

void dcu_bus_source_bind(dcu_bus_source_t *bs, datahub_t *hub, const dcu_inventory_t *inv) {
    if (!bs) return;
    bs->hub = hub;
    bs->inv = inv;
}

poller_source_ops_t dcu_bus_source_ops(dcu_bus_source_t *bs) {
    poller_source_ops_t ops;
    ops.ctx = bs;
    ops.read_value = _read_value;
    return ops;
}
