#include "dcu_bus_source.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

#include "dcu_inventory.h"
#include "edge_api.h"

/* 槽位与档案下标一一对应：一只表同时只允许一条在途请求。
 * 下标会被复用（删除用末尾覆盖、清档归零），所以槽位记地址，地址不符就作废。 */
#define DCU_BUS_SLOTS MAX_METERS

typedef enum {
    SLOT_IDLE = 0,  /* 可提交 */
    SLOT_INFLIGHT,  /* 已提交，等 on_finish */
    SLOT_DONE,      /* on_finish 成功，待取值 */
    SLOT_FAILED     /* on_finish 失败，待记一次失败 */
} bus_slot_state_t;

typedef struct dcu_bus_source_s dcu_bus_source_t;

/* 每次提交的身份凭据：回调只认自己那一张，槽位被复用也不会错认 */
typedef struct bus_ctx_s {
    struct bus_slot_s *slot; /* 作废后置 NULL：迟到的回调不得再碰已释放的 source */
    uint64_t gen;
    uint8_t used;
} bus_ctx_t;

typedef struct bus_slot_s {
    dcu_bus_source_t *owner;
    bus_ctx_t *ctx;   /* 当前在途请求的凭据（仅 INFLIGHT 期间有效） */
    uint32_t meter_idx;
    uint32_t tag_id;
    char address[MAX_ADDR_LEN]; /* 提交时的表地址：下标被别的表复用后用于作废 */
    uint64_t gen;
    uint64_t ts;      /* 提交时刻：仅作"回调丢失"的兜底计时 */
    bus_slot_state_t state;
} bus_slot_t;

struct dcu_bus_source_s {
    datahub_t *hub;
    const dcu_inventory_t *inv;
    uint64_t timeout_ms;
    uint64_t gen_seq;
    uint32_t ctx_cursor;
    int refs;         /* 基础引用 + 在途提交数 + 正在执行的 read_value 数 */
    uint8_t closing;
    uv_mutex_t lock;
    bus_slot_t slots[DCU_BUS_SLOTS];
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
            c->slot = NULL;
            bs->ctx_cursor = (bs->ctx_cursor + n + 1) % DCU_BUS_SLOTS;
            return c;
        }
    }
    return NULL;
}

/* on_finish 只会来一次（master_engine 的超时/成功/协议错误三条路径各调用一次） */
static void _on_done(int status, const pipe_msg_t *result, void *user_data) {
    (void)result;
    bus_ctx_t *c = (bus_ctx_t *)user_data;
    if (!c) return;

    bus_slot_t *s = c->slot;
    if (!s || !s->owner) { /* 已作废的请求：只归还凭据，不碰 source */
        c->used = 0;
        c->slot = NULL;
        return;
    }
    dcu_bus_source_t *bs = s->owner;

    int free_now = 0;
    uv_mutex_lock(&bs->lock);
    if (s->gen == c->gen && s->state == SLOT_INFLIGHT) {
        s->state = (status == 0) ? SLOT_DONE : SLOT_FAILED;
        s->ctx = NULL;
    }
    c->used = 0;
    c->slot = NULL;
    free_now = (--bs->refs == 0);
    uv_mutex_unlock(&bs->lock);
    if (free_now) _bs_free(bs);
}

/* 作废当前在途请求：槽位放回，凭据脱离 source，迟到的回调不会 UAF */
static void _abandon_locked(dcu_bus_source_t *bs, bus_slot_t *s, int drop_ref) {
    if (s->ctx) {
        s->ctx->slot = NULL;
        s->ctx = NULL;
        if (drop_ref) bs->refs--;
    }
    s->state = SLOT_IDLE;
}

static int _read_value_impl(dcu_bus_source_t *bs, uint32_t meter_idx, uint32_t tag_id,
                            double *out_val) {
    if (!out_val) return -1;
    if (!bs->inv) {
        _src_err("no-inv", meter_idx, tag_id, -1);
        return -1;
    }
    if (meter_idx >= bs->inv->count) {
        _src_err("idx-out-of-range", meter_idx, tag_id, (int)bs->inv->count);
        return -1;
    }

    const production_meter_t *m = &bs->inv->meters[meter_idx];
    const char *address = m->address;

    int action = 0; /* 1=已提交, -1=本轮失败, 2=成功待取值, 0=在途 */
    bus_slot_t *s = &bs->slots[meter_idx];
    bus_ctx_t *c = NULL;

    uv_mutex_lock(&bs->lock);
    s->owner = bs;
    s->meter_idx = meter_idx;

    int same_owner = (s->state == SLOT_IDLE) ||
                     (strncmp(s->address, address, sizeof(s->address)) == 0 && s->tag_id == tag_id);
    if (!same_owner) {
        /* 下标已指向另一只表 / 另一路 CSD：上一条的结果不再属于任何调用方 */
        if (s->state == SLOT_INFLIGHT) {
            action = 0; /* 等它落地再作废，避免同一 (表, CSD) 出现第二路在途请求 */
        } else {
            s->state = SLOT_IDLE;
            action = -1;
        }
    } else if (s->state == SLOT_IDLE) {
        c = _ctx_acquire(bs);
        if (c) {
            strncpy(s->address, address, sizeof(s->address) - 1);
            s->address[sizeof(s->address) - 1] = '\0';
            s->tag_id = tag_id;
            s->gen = ++bs->gen_seq;
            s->ts = _now_ms();
            s->state = SLOT_INFLIGHT;
            s->ctx = c;
            c->gen = s->gen;
            c->slot = s;
            bs->refs++; /* 在途：由 _on_done 归还 */
            action = 1;
        } else {
            action = -1; /* 凭据池耗尽（只可能发生在回调长期丢失后）：按失败计，不挂着 */
        }
    } else if (s->state == SLOT_FAILED) {
        s->state = SLOT_IDLE;
        action = -1; /* 失败只记一次 */
    } else if (s->state == SLOT_DONE) {
        s->state = SLOT_IDLE;
        action = 2;
    } else if (_now_ms() - s->ts > bs->timeout_ms) {
        /* 兜底：总线自身 2s 必超时并回调，走到这里说明回调丢了。
         * shortcut: 丢一个回调漏一张凭据（约 24B），池子耗尽后相关表按失败计。 */
        _abandon_locked(bs, s, 1);
        action = -1;
    }
    uv_mutex_unlock(&bs->lock);

    if (action == 0) return POLL_READ_PENDING; /* 在途 */
    if (action == -1) {
        _src_err("state-fail", meter_idx, tag_id, (int)s->state);
        return -1;
    }

    if (action == 1) {
        if (edge_api_read_tag((int)m->channel_id, m->protocol, meter_idx, tag_id, _on_done, c) !=
            0) {
            uv_mutex_lock(&bs->lock);
            if (c->slot == s) {
                c->slot = NULL;
                s->ctx = NULL;
                s->state = SLOT_IDLE;
            }
            c->used = 0;
            bs->refs--; /* 提交失败不会有回调 */
            uv_mutex_unlock(&bs->lock);
            /* edge_api_read_tag 自己会打 [API-ERROR]，这里只标来源 */
            _src_err("submit-fail", meter_idx, tag_id, (int)m->channel_id);
            return -1;
        }
        return POLL_READ_PENDING;
    }

    pipe_msg_t msg;
    if (datahub_read_shadow_device(bs->hub, tag_id, meter_idx, &msg) == 0 && msg.ts != 0) {
        *out_val = msg.payload.val.f;
        return 0;
    }
    _src_err("no-shadow", meter_idx, tag_id, (int)tag_id);
    return -1; /* 应答成功但没落 shadow：本轮算失败，不回 0.0 */
}

static int _read_value(void *ctx, uint32_t meter_idx, uint32_t tag_id, double *out_val) {
    dcu_bus_source_t *bs = (dcu_bus_source_t *)ctx;
    if (!bs) return -1;

    uv_mutex_lock(&bs->lock);
    if (bs->closing) {
        uv_mutex_unlock(&bs->lock);
        return -1;
    }
    bs->refs++; /* 本次调用期间 stop() 不能释放 bs */
    uv_mutex_unlock(&bs->lock);

    int rc = _read_value_impl(bs, meter_idx, tag_id, out_val);

    int free_now = 0;
    uv_mutex_lock(&bs->lock);
    free_now = (--bs->refs == 0);
    uv_mutex_unlock(&bs->lock);
    if (free_now) _bs_free(bs);
    return rc;
}

dcu_bus_source_t *dcu_bus_source_start(datahub_t *hub, const dcu_inventory_t *inv,
                                       uint32_t timeout_ms) {
    dcu_bus_source_t *bs = (dcu_bus_source_t *)calloc(1, sizeof(*bs));
    if (!bs) return NULL;
    bs->hub = hub;
    bs->inv = inv;
    bs->timeout_ms = timeout_ms ? timeout_ms : 4000;
    bs->refs = 1; /* 基础引用 */
    for (uint32_t i = 0; i < DCU_BUS_SLOTS; i++) {
        bs->slots[i].owner = bs;
        bs->slots[i].meter_idx = i;
    }
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
    /* 仍有在途请求：内存交给最后一个回调/read_value 释放（edge_api 无 cancel 接口） */
}

poller_source_ops_t dcu_bus_source_ops(dcu_bus_source_t *bs) {
    poller_source_ops_t ops;
    ops.ctx = bs;
    ops.read_value = _read_value;
    return ops;
}
