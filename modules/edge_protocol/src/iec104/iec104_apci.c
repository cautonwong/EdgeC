/**
 * @file iec104_apci.c
 * @brief IEC 104 应用规约控制信息 (APCI) 实现 (Phase 5 Task 5 - Sliding Window)
 */

#include "iec104_apci.h"

#include <string.h>

#include "edge_common.h"
#include "edge_log.h"

static void _reclaim_acked_frames(iec104_apci_mgr_t *mgr, uint16_t nr) {
    // 处理回绕情况下的序号比较 (RFC 风格或简单的 15位对齐比较)
    uint16_t current_nr = mgr->acked_nr;
    int reclaimed = 0;

    while (current_nr != nr) {
        int slot = current_nr % 32;
        if (mgr->unacked_frames[slot]) {
            net_buf_unref(mgr->unacked_frames[slot]);
            mgr->unacked_frames[slot] = NULL;
            reclaimed++;
        }
        current_nr = (uint16_t)((current_nr + 1) & 0x7FFF);

        // 安全限制，防止死循环
        if (reclaimed > 32) break;
    }

    mgr->acked_nr = nr;

    // 如果所有发送的 I 帧都已确认，停止 t1 计时
    if (mgr->acked_nr == mgr->ns) {
        mgr->t1_start_ts = 0;
    } else {
        // 如果还有未确认的，重置 t1 为当前时间（或者保持原有起始时间以防长时间未回）
        // 标准 104 建议保持第一个未确认包的时间戳
    }
}

struct iec104_apci_mgr {
    iec104_state_t state;

    // 序列号维护
    uint16_t ns;       /**< 发送序列号 N(S) */
    uint16_t nr;       /**< 接收序列号 N(R) */
    uint16_t acked_nr; /**< 上次对方确认到的序列号 */

    // 滑窗控制 (k, w 因子)
    uint16_t k; /**< 最大未确认发送 I 帧数 (默认 12) */
    uint16_t w; /**< 最大未确认接收 I 帧数 (默认 8) */

    // 定时器 (t0, t1, t2, t3)
    uint64_t t1_start_ts; /**< I 帧发送时间戳 (用于 t1 超时监测) */
    uint64_t t2_start_ts; /**< 未确认接收 I 帧时间戳 (用于 t2 自动发 S 帧) */
    uint64_t t3_start_ts; /**< 空闲链路时间戳 (用于 t3 发送 TESTFR) */

    // 发送备份队列 (用于重发，逻辑上暂由 Engine 协助)
    net_buf_t *unacked_frames[32];
};

_Static_assert(sizeof(struct iec104_apci_mgr) <= sizeof(iec104_apci_mgr_storage_t),
               "iec104_apci_mgr_storage_t size is insufficient");

iec104_apci_mgr_t *iec104_apci_mgr_create_in(iec104_apci_mgr_storage_t *store) {
    if (!store) {
        return NULL;
    }
    iec104_apci_mgr_t *mgr = (iec104_apci_mgr_t *)store->raw;
    iec104_apci_init(mgr);
    return mgr;
}

iec104_state_t iec104_apci_get_state(const iec104_apci_mgr_t *mgr) {
    return mgr ? mgr->state : IEC104_STATE_DISCONNECTED;
}

void iec104_apci_set_state(iec104_apci_mgr_t *mgr, iec104_state_t state) {
    if (mgr) {
        mgr->state = state;
    }
}

uint16_t iec104_apci_get_ns(const iec104_apci_mgr_t *mgr) {
    return mgr ? mgr->ns : 0;
}

uint16_t iec104_apci_get_nr(const iec104_apci_mgr_t *mgr) {
    return mgr ? mgr->nr : 0;
}

void iec104_apci_set_nr(iec104_apci_mgr_t *mgr, uint16_t nr) {
    if (mgr) {
        mgr->nr = nr;
    }
}

void iec104_apci_init(iec104_apci_mgr_t *mgr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(struct iec104_apci_mgr));
    mgr->state = IEC104_STATE_DISCONNECTED;
    mgr->k = 12;
    mgr->w = 8;
}

static void _build_apci_header(uint8_t *p, uint8_t len, uint16_t control1, uint16_t control2) {
    p[0] = 0x68;
    p[1] = len;
    p[2] = (uint8_t)(control1 & 0xFF);
    p[3] = (uint8_t)(control1 >> 8);
    p[4] = (uint8_t)(control2 & 0xFF);
    p[5] = (uint8_t)(control2 >> 8);
}

int iec104_apci_build_u(iec104_u_type_t type, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    net_buf_simple_reset(b);
    uint8_t *p = net_buf_simple_add(b, 6);

    uint16_t c1 = 0;
    switch (type) {
        case IEC104_U_STARTDT_ACT:
            c1 = 0x0007;
            break;
        case IEC104_U_STARTDT_CON:
            c1 = 0x000B;
            break;
        case IEC104_U_STOPDT_ACT:
            c1 = 0x0013;
            break;
        case IEC104_U_STOPDT_CON:
            c1 = 0x0023;
            break;
        case IEC104_U_TESTFR_ACT:
            c1 = 0x0043;
            break;
        case IEC104_U_TESTFR_CON:
            c1 = 0x0083;
            break;
    }
    _build_apci_header(p, 4, c1, 0);
    return 0;
}

int iec104_apci_build_s(iec104_apci_mgr_t *mgr, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;
    net_buf_simple_reset(b);
    uint8_t *p = net_buf_simple_add(b, 6);

    _build_apci_header(p, 4, 0x0001, (uint16_t)(mgr->nr << 1));

    // 发送 S 帧后，重置 t2 计时
    mgr->t2_start_ts = 0;
    return 0;
}

void iec104_apci_handle_nr(iec104_apci_mgr_t *mgr, uint16_t nr) {
    _reclaim_acked_frames(mgr, nr);
}

int iec104_apci_wrap_i(iec104_apci_mgr_t *mgr, net_buf_t *asdu) {
    size_t asdu_len = asdu->b.len;
    if (asdu_len > 249) return -1;

    // 1. 检查发送滑窗 (k 因子)
    uint16_t unacked_count = (uint16_t)((mgr->ns - mgr->acked_nr + 0x8000) & 0x7FFF);
    if (unacked_count >= mgr->k) {
        return EDGE_ERR_BUSY;
    }

    // 2. 封包
    uint8_t *p = net_buf_push(asdu, 6);
    uint16_t c1 = (uint16_t)(mgr->ns << 1);
    uint16_t c2 = (uint16_t)(mgr->nr << 1);
    _build_apci_header(p, (uint8_t)(asdu_len + 4), c1, c2);

    // 3. 备份用于重发
    int slot = mgr->ns % 32;
    if (mgr->unacked_frames[slot]) net_buf_unref(mgr->unacked_frames[slot]);
    mgr->unacked_frames[slot] = net_buf_ref(asdu);

    // 4. 时序与序号
    if (mgr->t1_start_ts == 0) mgr->t1_start_ts = uv_hrtime() / 1000000;
    mgr->t3_start_ts = uv_hrtime() / 1000000;
    mgr->ns = (uint16_t)((mgr->ns + 1) & 0x7FFF);

    return 0;
}

iec104_apci_event_t iec104_apci_tick(iec104_apci_mgr_t *mgr, uint64_t now) {
    if (mgr->state == IEC104_STATE_DISCONNECTED) return IEC104_EVENT_NONE;

    // 1. t1 监测 (15s)
    if (mgr->t1_start_ts > 0 && (now - mgr->t1_start_ts) > 15000) {
        return IEC104_EVENT_T1_TIMEOUT;
    }

    // 2. t2 监测与 w 因子 (10s 或未确认包数达到 w)
    // 注意：w 因子判定通常在 handle_rx 中做，tick 中只看时间
    if (mgr->t2_start_ts > 0 && (now - mgr->t2_start_ts) > 10000) {
        return IEC104_EVENT_SEND_S;
    }

    // 3. t3 监测 (20s)
    if (mgr->t3_start_ts > 0 && (now - mgr->t3_start_ts) > 20000) {
        mgr->t3_start_ts = now;
        return IEC104_EVENT_SEND_TESTFR;
    }

    return IEC104_EVENT_NONE;
}