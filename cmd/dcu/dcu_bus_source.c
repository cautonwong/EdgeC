#include "dcu_bus_source.h"

#include "dcu_driver_adapter.h"
#include "dcu_node.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

#include "dcu_inventory.h"
#include "edge_api.h"

/* 在途凭据池：链式模型下每通道最多 depth 个在途，池子只需覆盖所有通道的峰值。
 * 取 MAX_METERS 是保守上限，实际用不到这么多；留着是为了不引入新的配置面。 */
#define DCU_BUS_SLOTS MAX_METERS

/* 每次提交的身份凭据：回调只认自己那一张。
 *
 * 这里不再有"按档案下标排的槽位表"——那是轮询取值的产物（read_value 被反复调用，
 * 要靠槽位暂存结果）。链式模型下每次提交只对应一次回调，凭据池足够，
 * 4096 张槽位（约 400KB）整块省掉。 */
typedef struct bus_ctx_s {
    struct dcu_bus_source_s *bs; /* 作废后置 NULL：迟到的回调不得再碰已释放的 source */
    poller_read_done_cb cb;
    void *cb_ud;
    uint32_t meter_idx;
    uint32_t tag_id;   /* 消费侧 OAD */
    uint32_t proto_id; /* 线上协议号(DI)：交给驱动 */
    uint64_t gen;
    uint8_t used;
} bus_ctx_t;

struct dcu_bus_source_s {
    datahub_t *hub;
    dcu_node_t *node; /* 保留：装配方仍按旧签名传入，本源已不再直接落库 */
    const dcu_inventory_t *inv;
    uint64_t timeout_ms;
    uint64_t gen_seq;
    uint32_t ctx_cursor;
    int refs;         /* 基础引用 + 在途提交数 */
    uint8_t closing;
    uv_mutex_t lock;
    bus_ctx_t ctxs[DCU_BUS_SLOTS];
};

static uint64_t _now_ms(void) { return uv_hrtime() / 1000000ULL; }

/* 静默失败会让"一帧都不发"无从定位：每个 -1 出口带原因码，每秒最多一条 */
static void _src_err(const char *reason, uint32_t meter_idx, uint32_t tag_id, int extra) {
    static uint64_t last_ms = 0;
    static const char *last_reason = NULL;
    uint64_t now = _now_ms();
    if (last_reason == reason && now - last_ms < 1000) return;
    last_ms = now;
    last_reason = reason;
    printf("[SRC-ERR] reason=%s meter=%u tag=%08x extra=%d\n", reason, meter_idx, tag_id, extra);
}

static void _bs_free(dcu_bus_source_t *bs) {
    uv_mutex_destroy(&bs->lock);
    free(bs);
}

/* 持锁调用：借一张未用的身份凭据 */
static bus_ctx_t *_ctx_acquire(dcu_bus_source_t *bs) {
    for (uint32_t n = 0; n < DCU_BUS_SLOTS; n++) {
        bus_ctx_t *c = &bs->ctxs[(bs->ctx_cursor + n) % DCU_BUS_SLOTS];
        if (!c->used) {
            c->used = 1;
            c->bs = bs;
            bs->ctx_cursor = (bs->ctx_cursor + n + 1) % DCU_BUS_SLOTS;
            return c;
        }
    }
    return NULL;
}

/* 请求完成：把结果交给链（poller），由它决定落库与扇出。
 * 本层只做协议适配——OAD→DI、量纲换算——不碰 datahub、不碰存储。 */
static void _on_done(int status, const pipe_msg_t *result, void *user_data) {
    bus_ctx_t *c = (bus_ctx_t *)user_data;
    if (!c) return;

    dcu_bus_source_t *bs = c->bs;
    poller_read_done_cb cb = c->cb;
    void *ud = c->cb_ud;
    uint32_t meter_idx = c->meter_idx;
    uint32_t tag_id = c->tag_id;
    uint32_t proto_id = c->proto_id;

    double value = 0.0;
    int ok = (status == 0 && result != NULL);
    if (ok) {
        value = result->payload.val.f;
        if (bs && bs->inv && meter_idx < bs->inv->count) {
            const production_meter_t *m = &bs->inv->meters[meter_idx];
            double scale = dcu_driver_adapter_scale(m->protocol, proto_id);
            if (scale > 0.0) value *= scale;
        }
    }

    int free_now = 0;
    if (bs) {
        uv_mutex_lock(&bs->lock);
        c->used = 0;
        c->bs = NULL;
        c->cb = NULL;
        c->cb_ud = NULL;
        free_now = (--bs->refs == 0);
        uv_mutex_unlock(&bs->lock);
    }

    /* 先回调再释放：cb 里可能还要用 poller 的状态 */
    if (cb) cb(ud, ok ? 0 : (status ? status : -1), value);
    if (free_now && bs && bs->closing) _bs_free(bs);

    if (!ok) _src_err("read-fail", meter_idx, tag_id, status);
}

/* 链式提交的唯一入口：受理即返回，结果走 cb。
 * 通道忙不忙由 arbiter 判（is_busy/queue_len），本层不判、也不排队。 */
static int _submit(void *ctx, uint32_t meter_idx, uint32_t tag_id, poller_read_done_cb cb,
                   void *ud) {
    dcu_bus_source_t *bs = (dcu_bus_source_t *)ctx;
    if (!bs || !cb) return -1;
    if (!bs->inv) {
        _src_err("no-inv", meter_idx, tag_id, -1);
        return -1;
    }
    if (meter_idx >= bs->inv->count) {
        _src_err("idx-out-of-range", meter_idx, tag_id, (int)bs->inv->count);
        return -1;
    }

    const production_meter_t *m = &bs->inv->meters[meter_idx];
    uint32_t proto_id = dcu_driver_adapter_oad_to_proto(m->protocol, tag_id);
    if (proto_id == 0) {
        _src_err("no-proto", meter_idx, tag_id, (int)tag_id);
        return -1;
    }

    uv_mutex_lock(&bs->lock);
    if (bs->closing) {
        uv_mutex_unlock(&bs->lock);
        return -1;
    }
    bus_ctx_t *c = _ctx_acquire(bs);
    if (!c) {
        uv_mutex_unlock(&bs->lock);
        _src_err("ctx-exhausted", meter_idx, tag_id, 0);
        return -1;
    }
    c->cb = cb;
    c->cb_ud = ud;
    c->meter_idx = meter_idx;
    c->tag_id = tag_id;
    c->proto_id = proto_id;
    c->gen = ++bs->gen_seq;
    bs->refs++; /* 在途：由 _on_done 归还 */
    uv_mutex_unlock(&bs->lock);

    /* 抄表是后台任务：低优先级，让交互式请求先过总线 */
    if (edge_api_read_tag((int)m->channel_id, m->protocol, meter_idx, proto_id, ARBITER_PRIO_LOW,
                          _on_done, c) != 0) {
        uv_mutex_lock(&bs->lock);
        c->used = 0;
        c->bs = NULL;
        c->cb = NULL;
        c->cb_ud = NULL;
        bs->refs--; /* 提交失败不会有回调 */
        uv_mutex_unlock(&bs->lock);
        /* edge_api_read_tag 自己会打 [API-ERROR]，这里只标来源 */
        _src_err("submit-fail", meter_idx, tag_id, (int)m->channel_id);
        return -1;
    }
    return 0;
}

dcu_bus_source_t *dcu_bus_source_start(datahub_t *hub, const dcu_inventory_t *inv,
                                       dcu_node_t *node, uint32_t timeout_ms) {
    dcu_bus_source_t *bs = (dcu_bus_source_t *)calloc(1, sizeof(*bs));
    if (!bs) return NULL;
    bs->hub = hub;
    bs->inv = inv;
    bs->node = node;
    bs->timeout_ms = timeout_ms ? timeout_ms : 4000;
    bs->refs = 1; /* 基础引用 */
    if (uv_mutex_init(&bs->lock) != 0) {
        free(bs);
        return NULL;
    }
    return bs;
}

void dcu_bus_source_stop(dcu_bus_source_t *bs) {
    if (!bs) return;
    int free_now = 0;
    uv_mutex_lock(&bs->lock);
    bs->closing = 1;
    free_now = (--bs->refs == 0);
    uv_mutex_unlock(&bs->lock);
    if (free_now) _bs_free(bs);
    /* 仍有在途请求：内存交给最后一个回调释放（edge_api 无 cancel 接口） */
}

poller_source_ops_t dcu_bus_source_ops(dcu_bus_source_t *bs) {
    poller_source_ops_t ops;
    memset(&ops, 0, sizeof(ops));
    ops.ctx = bs;
    ops.submit = _submit;
    return ops;
}
