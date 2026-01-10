/**
 * @file iec104_apci.c
 * @brief IEC 104 应用规约控制信息 (APCI) 实现
 */

#include "iec104_apci.h"

#include <string.h>

#include "edge_common.h"
#include "edge_log.h"

static void _reclaim_acked_frames(iec104_apci_mgr_t *mgr, uint16_t nr) {
    uint16_t current_nr = mgr->acked_nr;
    while (current_nr != nr) {
        int slot = current_nr % 32;
        if (mgr->unacked_frames[slot]) {
            net_buf_unref(mgr->unacked_frames[slot]);
            mgr->unacked_frames[slot] = NULL;
        }
        current_nr = (uint16_t)((current_nr + 1) & 0x7FFF);
    }
    mgr->acked_nr = nr;
    if (mgr->acked_nr == mgr->ns) mgr->t1_start_ts = 0;  // 所有包已确认，停掉 t1
}

void iec104_apci_init(iec104_apci_mgr_t *mgr) {
    memset(mgr, 0, sizeof(iec104_apci_mgr_t));
    mgr->state = IEC104_STATE_DISCONNECTED;
    mgr->k = 12;  // 默认最大未确认发送 12
    mgr->w = 8;   // 默认最大未确认接收 8
}

/**
 * @brief 内部辅助：构建 6 字节 APCI 头部
 */
static void _build_apci_header(uint8_t *p, uint8_t len, uint16_t control1, uint16_t control2) {
    p[0] = 0x68;
    p[1] = len;
    p[2] = (uint8_t)(control1 & 0xFF);
    p[3] = (uint8_t)(control1 >> 8);
    p[4] = (uint8_t)(control2 & 0xFF);
    p[5] = (uint8_t)(control2 >> 8);
}

int iec104_apci_build_u(iec104_u_type_t type, net_buf_t *out) {
    // U 帧：无 ASDU，固定 6 字节
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
    _build_apci_header(p, 4, c1, 0);  // 长度 4 指 APCI 去掉 0x68 和长度字节后的长度
    return 0;
}

int iec104_apci_build_s(iec104_apci_mgr_t *mgr, net_buf_t *out) {
    // S 帧：确认对方的 I 帧，不带 ASDU
    struct net_buf_simple *b = &out->b;
    net_buf_simple_reset(b);
    uint8_t *p = net_buf_simple_add(b, 6);

    _build_apci_header(p, 4, 0x0001, (uint16_t)(mgr->nr << 1));
    return 0;
}

int iec104_apci_wrap_i(iec104_apci_mgr_t *mgr, net_buf_t *asdu) {
    size_t asdu_len = asdu->b.len;

    if (asdu_len > 249) return -1;

    // 1. 检查滑窗是否已满 (k 因子)
    uint16_t unacked_count = (uint16_t)((mgr->ns - mgr->acked_nr + 0x8000) & 0x7FFF);
    if (unacked_count >= mgr->k) {
        LOG_WARN(LOG_CAT_IO, "IEC104: Tx Window Full (k=%d). Backpressure activated.", mgr->k);
        return EDGE_ERR_BUSY;
    }

    // 2. 执行 Zero-Copy 封包

    uint8_t *p = net_buf_push(asdu, 6);

    uint16_t c1 = (uint16_t)(mgr->ns << 1);

    uint16_t c2 = (uint16_t)(mgr->nr << 1);

    _build_apci_header(p, (uint8_t)(asdu_len + 4), c1, c2);

    // 3. 物理备份：存入环形队列并增加引用

    int slot = mgr->ns % 32;

    if (mgr->unacked_frames[slot]) net_buf_unref(mgr->unacked_frames[slot]);

    mgr->unacked_frames[slot] = net_buf_ref(asdu);

    // 4. 时序维护

    if (mgr->t1_start_ts == 0) mgr->t1_start_ts = uv_hrtime() / 1000000;

    mgr->t3_start_ts = uv_hrtime() / 1000000;

    mgr->ns = (uint16_t)((mgr->ns + 1) & 0x7FFF);

    return 0;
}

void iec104_apci_handle_nr(iec104_apci_mgr_t *mgr, uint16_t nr) {
    _reclaim_acked_frames(mgr, nr);
}

iec104_apci_event_t iec104_apci_tick(iec104_apci_mgr_t *mgr, uint64_t now) {
    if (mgr->state == IEC104_STATE_DISCONNECTED) return IEC104_EVENT_NONE;

    // 1. t1 监测：等待发送确认
    if (mgr->t1_start_ts > 0 && (now - mgr->t1_start_ts) > 15000) {
        return IEC104_EVENT_T1_TIMEOUT;
    }

    // 2. t2 监测：延迟确认
    if (mgr->t2_start_ts > 0 && (now - mgr->t2_start_ts) > 10000) {
        mgr->t2_start_ts = 0;  // 防止重复触发
        return IEC104_EVENT_SEND_S;
    }

    // 3. t3 监测：空闲心跳
    if (mgr->t3_start_ts > 0 && (now - mgr->t3_start_ts) > 20000) {
        mgr->t3_start_ts = now;
        return IEC104_EVENT_SEND_TESTFR;
    }

    return IEC104_EVENT_NONE;
}
