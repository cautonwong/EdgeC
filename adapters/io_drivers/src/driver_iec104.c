/**
 * @file driver_iec104.c
 * @brief IEC 60870-5-104 驱动 —— 纯 codec（ADR-002 S3-d）
 *
 * ## 点位标识
 *
 * IEC104 的 ASDU 需要「类型标识 + 信息对象地址」两个东西才能组帧，
 * 与 IEC101 完全同构，所以用同一套编码（两个都是协议原生概念）：
 *
 *     native_id = (type_id << 24) | (ioa & 0xFFFFFF)
 *
 * 产品侧用 IEC_NATIVE_ID() 编，驱动侧解。驱动不再持有点位表。
 *
 * ## 两个 priv 的分工
 *
 * - `iec104_req_t`（`req_priv_size`）：请求 scratch，只放"这一次要读/写哪个点"
 * - `iec104_session_t`（`ctx_size`）：会话，放 APCI 状态机、收发序号、
 *   总召唤标记、自发上报的死区缓存。**序号必须跨帧保留**——放 scratch 里
 *   每帧归零，对端会直接判帧错。
 *
 * ## 四个槽位
 *
 * - `req_prepare` / `req_encode` / `req_decode`：主站点对点读
 * - `req_slave_exchange`：从站应答（STARTDT、总召唤、遥控/遥调）
 * - `req_tick`：主站建链与总召唤、从站自发上报、APCI 保活、t1 超时断链
 */

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "iec104_apci.h"
#include "iec_asdu.h"
#include "net_buf.h"

/* 自发上报的死区缓存。定长，不用动态内存——核心路径不允许 malloc。
 * 16 个槽位是被 ctx_size 上限逼出来的：APCI 状态机自己就占 512 字节，
 * 而 driver_manager.c 的 DRIVER_PRIV_SIZE 是 1024。 */
#define IEC104_CACHE_SLOTS 16
/* 没有点位级死区配置时的默认门槛：只要变就报 */
#define IEC104_DEFAULT_DEADBAND 0.0
/* v3 只接受站召唤（iems_v3/src/iec104_server/iec104_server.c:183-187）。 */
#define IEC104_QOI_STATION 20

typedef struct {
    uint32_t ioa;
    double last;
    bool used;
    bool sampled; /* 是否已有基线；首帧只建基线不报 */
} iec104_cache_slot_t;

typedef struct {
    iec104_apci_mgr_storage_t apci_store;
    iec104_apci_mgr_t *apci;
    uint16_t casdu;
    bool needs_gi;
    bool transfer_active; /* STARTDT 之后才发自发帧；STOPDT 暂停，连接与保活保留 */
    bool initialized; /* priv 由调用方清零；首次 on_init 前不能解引用未构造的 apci */
    uint64_t last_gi_ts;
    iec104_cache_slot_t cache[IEC104_CACHE_SLOTS];
} iec104_session_t;

typedef struct {
    uint8_t type_id;
    uint32_t ioa;
    double value;
    bool is_write;
} iec104_req_t;

/* ctx_size 超了 DRIVER_PRIV_SIZE，driver_instance_create 会**静默**返回 NULL，
 * 表现成"会话建不出来"，很难查。这里编译期挡住。 */
_Static_assert(sizeof(iec104_session_t) <= 1024,
               "IEC104 会话超出 DRIVER_PRIV_SIZE(1024)，见 driver_manager.c");

/* ==================== 会话 ==================== */

static int iec104_on_init(edge_driver_ctx_t *ctx) {
    if (!ctx || !ctx->priv) return -1;
    iec104_session_t *s = (iec104_session_t *)ctx->priv;
    /* 会话复位会再次进来：先归还上一会话未确认 I 帧的备份引用，
     * 否则下面的 memset 丢掉唯一引用，每轮泄漏 2~3 个池缓冲。 */
    if (s->initialized && s->apci) iec104_apci_release_unacked(s->apci);
    memset(s, 0, sizeof(*s));

    s->apci = iec104_apci_mgr_create_in(&s->apci_store);
    if (!s->apci) return -1;
    /* 单 CA：device_id 是组合根给本会话的公共地址。v3 从 JSON commonAddr
     * 读，且可配多个 CA；仓库没有该配置源，多 CA 仍是遗留。 */
    s->casdu = (uint16_t)ctx->device_id;
    /* 主站建链后要发总召唤；从站不主动发起 */
    s->needs_gi = (ctx->role == PROTOCOL_ROLE_MASTER);
    s->initialized = true;
    return 0;
}

/* 死区缓存：找 ioa 的槽位，找不到就占一个空槽 */
static iec104_cache_slot_t *iec104_cache_find(iec104_session_t *s, uint32_t ioa, bool create) {
    iec104_cache_slot_t *free_slot = NULL;
    for (int i = 0; i < IEC104_CACHE_SLOTS; i++) {
        if (s->cache[i].used && s->cache[i].ioa == ioa) return &s->cache[i];
        if (!s->cache[i].used && !free_slot) free_slot = &s->cache[i];
    }
    if (!create || !free_slot) return NULL;
    free_slot->used = true;
    free_slot->ioa = ioa;
    free_slot->last = 0.0;
    free_slot->sampled = false;
    return free_slot;
}

/* ==================== APCI 拆包（主站与从站共用） ==================== */

/**
 * @brief 剥掉 APCI，把 ASDU 留在 `asdu` 里
 *
 * @return 0 = 是 I 帧，`asdu` 已指向 ASDU 起始；
 *         1 = 是 U/S 帧（已处理序号），没有 ASDU；
 *         3 = 长度域非法（已在边界丢掉，不应答）；
 *         EDGE_ERR_INCOMPLETE = 还没收全；<0 = 帧格式错
 */
static int iec104_consume_apci(iec104_session_t *s, const uint8_t *in, size_t in_len,
                               struct net_buf_simple *asdu, size_t *consumed) {
    size_t skip = 0;
    while (skip < in_len && in[skip] != 0x68) skip++;
    if (skip > 0) {
        *consumed = skip; /* 前导垃圾，让上层继续找帧头 */
        return 1;
    }
    if (in_len < 6) return EDGE_ERR_INCOMPLETE;

    uint8_t total_len = in[1];
    /* 长度域 = 4 字节控制域 + ASDU；ASDU 上限 249（见 iec104_apci_wrap_i），
     * 所以合法范围是 [4, 253]。小于 4 时 asdu_len = total_len - 4 会下溢成
     * 巨大值，后面按它解 ASDU 就是越界读；大于 253 则超出协议上限。
     * 两种都在这里拒掉：只丢帧头两个字节，剩下的按"重新找 0x68"处理，
     * 不把连接楔死。 */
    if (total_len < 4 || total_len > 253) {
        *consumed = 2;
        return 3;
    }
    if (in_len < (size_t)total_len + 2) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)total_len + 2;
    /* 收到完整 APDU 就是链路活动，t3 空闲基线前移（含 U/S 帧） */
    iec104_apci_note_activity(s->apci);

    uint8_t c1 = in[2];
    if (c1 == 0x07) { /* STARTDT ACT */
        iec104_apci_set_state(s->apci, IEC104_STATE_CONNECTED);
        s->transfer_active = true;
        LOG_INFO(LOG_CAT_IO, "IEC104: 收到 STARTDT ACT");
        return 1;
    }
    if (c1 == 0x0B) { /* STARTDT CON */
        iec104_apci_set_state(s->apci, IEC104_STATE_CONNECTED);
        s->transfer_active = true;
        LOG_INFO(LOG_CAT_IO, "IEC104: 链路已激活");
        return 1;
    }
    if (c1 == 0x13) { /* STOPDT ACT：停数据传送，连接与 TESTFR 保活保留 */
        s->transfer_active = false;
        LOG_INFO(LOG_CAT_IO, "IEC104: 收到 STOPDT ACT");
        return 1;
    }
    if (c1 == 0x43) { /* TESTFR ACT → 由调用方回 CON */
        return 2;
    }
    if (c1 == 0x83) { /* TESTFR CON */
        return 1;
    }
    if (c1 == 0x01) { /* S 帧：控制域 01，N(R) 在 4-5 字节 */
        uint16_t nr = (uint16_t)((in[4] | (in[5] << 8)) >> 1);
        iec104_apci_handle_nr(s->apci, nr);
        return 1;
    }
    if ((c1 & 0x01) != 0) {
        /* 其余 U 帧 / 未知控制域：吃掉，不猜 */
        return 1;
    }

    /* I 帧：更新收发序号 */
    uint16_t ns = (uint16_t)((in[2] | (in[3] << 8)) >> 1);
    uint16_t nr = (uint16_t)((in[4] | (in[5] << 8)) >> 1);
    iec104_apci_set_nr(s->apci, (uint16_t)((ns + 1) & 0x7FFF));
    iec104_apci_handle_nr(s->apci, nr);

    net_buf_simple_init_with_data(asdu, (void *)(in + 6), in_len - 6);
    asdu->len = (uint16_t)(total_len - 4); /* 6 字节 APCI 之后是 ASDU */
    return 0;
}

/* ==================== 主站：组请求 ==================== */

static int iec104_req_prepare(void *priv_v, uint32_t slave_id, uint32_t point_id,
                              const void *meta_list, uint32_t meta_list_len,
                              const void *point_desc, size_t point_desc_len, bool is_write,
                              double value) {
    (void)meta_list;
    (void)meta_list_len;
    (void)point_desc;
    (void)point_desc_len;
    iec104_req_t *req = (iec104_req_t *)priv_v;
    if (!req) return -1;
    (void)slave_id; /* 104 是面向连接的，从站地址由会话决定，不在帧里 */

    req->type_id = IEC_NATIVE_TYPE(point_id);
    req->ioa = IEC_NATIVE_IOA(point_id);
    req->is_write = is_write;
    req->value = value;

    if (is_write) {
        /* 写请求必须带**命令类型**：拿测量类型去写是协议错误，
         * 也不能像读那样缺省成 M_ME_NC_1。 */
        if (!IEC_IS_COMMAND_TYPE(req->type_id)) {
            LOG_WARN(LOG_CAT_IO, "IEC104: 写请求的 type_id=%u 不是命令类型", req->type_id);
            return -1;
        }
        return 0;
    }

    /* 读：类型缺省成模拟量测量值（短浮点），而不是发一个类型 0 的非法 ASDU */
    if (req->type_id == 0) req->type_id = IEC_TYPE_M_ME_NC_1;
    return 0;
}

static int iec104_req_encode(void *priv_v, void *session, net_buf_t *out) {
    iec104_req_t *req = (iec104_req_t *)priv_v;
    iec104_session_t *s = (iec104_session_t *)session;
    if (!req || !s || !out) return -1;
    if (iec104_apci_get_state(s->apci) != IEC104_STATE_CONNECTED) return -1;

    if (req->is_write) {
        /* 控制方向用 ACT（激活） */
        iec_asdu_init(out, (iec_type_id_t)req->type_id, false, 1, IEC_COT_ACT, s->casdu,
                      &IEC_ADDR_CONF_104);
        int rc;
        if (req->type_id == IEC_TYPE_C_SC_NA_1) {
            rc = iec_asdu_add_c_sc_na_1(out, req->ioa, req->value > 0.0, false,
                                        &IEC_ADDR_CONF_104);
        } else if (req->type_id == IEC_TYPE_C_DC_NA_1) {
            uint8_t dcs = (req->value > 0.0) ? 2 : 1; /* 1=分闸，2=合闸 */
            rc = iec_asdu_add_c_dc_na_1(out, req->ioa, dcs, false, &IEC_ADDR_CONF_104);
        } else {
            rc = iec_asdu_add_c_se_nc_1(out, req->ioa, (float)req->value, 0, false,
                                        &IEC_ADDR_CONF_104);
        }
        if (rc != 0) return -1;
        return iec104_apci_wrap_i(s->apci, out) == 0 ? 0 : -1;
    }

    iec_asdu_init(out, (iec_type_id_t)req->type_id, false, 1, IEC_COT_REQ, s->casdu,
                  &IEC_ADDR_CONF_104);
    iec_asdu_add_any(out, req->type_id, req->ioa, 0, 0, 0, (iec_qds_t){0}, &IEC_ADDR_CONF_104);
    return iec104_apci_wrap_i(s->apci, out) == 0 ? 0 : -1;
}

/* ==================== 主站：解应答 ==================== */

static int iec104_req_decode(void *priv_v, void *session, const uint8_t *buf, size_t len,
                             size_t *consumed, double *out_value) {
    iec104_req_t *req = (iec104_req_t *)priv_v;
    iec104_session_t *s = (iec104_session_t *)session;
    if (!req || !s || !buf || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple asdu;
    int rc = iec104_consume_apci(s, buf, len, &asdu, consumed);
    if (rc == EDGE_ERR_INCOMPLETE) return EDGE_ERR_INCOMPLETE;
    if (rc != 0) return -1; /* U/S 帧不是测量值 */

    iec_type_id_t type;
    bool sq;
    uint8_t count;
    iec_cot_t cot;
    uint16_t casdu;
    if (iec_asdu_parse_header(&asdu, &type, &sq, &count, &cot, &casdu, &IEC_ADDR_CONF_104) != 0) {
        return -1;
    }

    /* 总召唤的 ACT CON / ACT TERM 不是值，吃掉即可 */
    if (type == IEC_TYPE_C_IC_NA_1) return -1;

    uint32_t ioa = 0;
    double val = 0.0;
    if (type == IEC_TYPE_M_ME_NC_1) {
        float f = 0.0f;
        iec_qds_t qds;
        if (iec_asdu_parse_m_me_nc_1(&asdu, &ioa, &f, &qds, false, false, &IEC_ADDR_CONF_104) != 0) return -1;
        val = (double)f;
    } else if (type == IEC_TYPE_M_SP_NA_1) {
        bool b = false;
        iec_qds_t qds;
        if (iec_asdu_parse_m_sp_na_1(&asdu, &ioa, &b, &qds, false, false, &IEC_ADDR_CONF_104) != 0) return -1;
        val = b ? 1.0 : 0.0;
    } else {
        return -1; /* 其余类型本次不解，不猜 */
    }

    if (ioa != req->ioa) return -1; /* 不是这次问的点，交给上层重试 */
    if (out_value) *out_value = val; /* 标定归调用方 */
    return 0;
}

/* ==================== 从站：应答 ==================== */

/* 总召唤按 v3 分三组攒包：bool/int/float 各一条 ASDU，每组最多 26 个对象
 * （iems_v3/src/iec104_server/iec104_server.c:177,190-238,330-339）。 */
#define IEC104_GI_GROUPS 3
#define IEC104_GI_MAX_OBJECTS 26

typedef struct {
    iec104_session_t *s;
    const edge_slave_source_ops_t *src;
    void *src_ctx;
    net_buf_t *out;
    struct {
        uint8_t count;
        net_buf_t *asdu; /* 未封 APCI 的 ASDU，攒够 26 个再封 */
    } group[IEC104_GI_GROUPS];
    int sent;
} iec104_gi_ctx_t;

static int iec104_gi_group(uint8_t type_id) {
    switch (type_id) {
        case IEC_TYPE_M_SP_NA_1:
            return 0;
        case IEC_TYPE_M_ME_NB_1:
            return 1;
        case IEC_TYPE_M_ME_NC_1:
            return 2;
        default:
            return -1;
    }
}

static void iec104_gi_flush(iec104_gi_ctx_t *g, int idx) {
    if (!g->group[idx].asdu) return;
    /* k 窗口满（EDGE_ERR_BUSY）时不能把没封 APCI 的 ASDU 追加进去 */
    if (g->group[idx].count > 0 && iec104_apci_wrap_i(g->s->apci, g->group[idx].asdu) == 0) {
        net_buf_simple_add_mem(&g->out->b, g->group[idx].asdu->b.data,
                               g->group[idx].asdu->b.len);
        g->sent++;
    }
    net_buf_unref(g->group[idx].asdu);
    g->group[idx].asdu = NULL;
    g->group[idx].count = 0;
}

/* 总召唤遍历回调：按类型追加到该组 ASDU；满 26 个对象先封一包 */
static void iec104_gi_emit(void *ud, uint32_t point_id) {
    iec104_gi_ctx_t *g = (iec104_gi_ctx_t *)ud;
    uint8_t type_id = IEC_NATIVE_TYPE(point_id);
    uint32_t ioa = IEC_NATIVE_IOA(point_id);
    if (type_id == 0) type_id = IEC_TYPE_M_ME_NC_1;
    int idx = iec104_gi_group(type_id);
    if (idx < 0) return;

    edge_point_value_t v;
    memset(&v, 0, sizeof(v));
    if (!g->src || !g->src->resolve) return;
    if (g->src->resolve(g->src_ctx, point_id, NULL, 0, &v) != 0) return;

    if (g->group[idx].count >= IEC104_GI_MAX_OBJECTS) iec104_gi_flush(g, idx);
    if (!g->group[idx].asdu) {
        g->group[idx].asdu = net_buf_alloc(g->out->pool, 0);
        if (!g->group[idx].asdu) return;
        iec_asdu_init(g->group[idx].asdu, (iec_type_id_t)type_id, false, 0, IEC_COT_INTROGEN,
                      g->s->casdu, &IEC_ADDR_CONF_104);
    }
    if (iec_asdu_add_any(g->group[idx].asdu, type_id, ioa, v.value, 0, 0, (iec_qds_t){0},
                         &IEC_ADDR_CONF_104) != 0) {
        return;
    }
    g->group[idx].count++;
    g->group[idx].asdu->b.data[1] = g->group[idx].count; /* VSQ 对象数 */
}

/* v3 的确认就是请求 ASDU 本身，只把 COT 换成确认码
 * （iems_v3/src/iec104_server/iec104_server.c:437-445,532）。回显保留 IOA
 * 与命令值；header-only 确认会让校验信息体的旧客户端判协议错。 */
static int iec104_echo_i(iec104_session_t *s, net_buf_t *out, const uint8_t *asdu, size_t len,
                         iec_cot_t cot, bool negative) {
    if (!asdu || len < 6) return -1; /* 104 ASDU 头 */
    net_buf_t *frame = net_buf_alloc(out->pool, 0);
    if (!frame) return -1;
    net_buf_simple_add_mem(&frame->b, asdu, len);
    iec_asdu_set_cot(&frame->b, cot, negative, &IEC_ADDR_CONF_104);
    int rc = iec104_apci_wrap_i(s->apci, frame);
    if (rc == 0) net_buf_simple_add_mem(&out->b, frame->b.data, frame->b.len);
    net_buf_unref(frame);
    return rc;
}

static int iec104_req_slave_exchange(void *priv_v, void *session, const uint8_t *in, size_t in_len,
                                     const edge_slave_source_ops_t *src, void *src_ctx,
                                     net_buf_t *out, size_t *consumed) {
    (void)priv_v; /* 从站应答只认 session：序号、状态机都在里面 */
    iec104_session_t *s = (iec104_session_t *)session;
    if (!s || !in || !out || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple asdu;
    int rc = iec104_consume_apci(s, in, in_len, &asdu, consumed);
    if (rc == EDGE_ERR_INCOMPLETE) return EDGE_ERR_INCOMPLETE;
    if (rc == 3) return 0; /* 长度域非法：边界处已丢掉，不回任何帧 */
    if (rc == 1) {
        /* STARTDT/STOPDT ACT 要回 CON；其余 U/S 帧无需应答 */
        if (in_len >= 6 && in[2] == 0x07) {
            iec104_apci_build_u(IEC104_U_STARTDT_CON, out);
        } else if (in_len >= 6 && in[2] == 0x13) {
            iec104_apci_build_u(IEC104_U_STOPDT_CON, out);
        }
        return 0;
    }
    if (rc == 2) { /* TESTFR ACT */
        iec104_apci_build_u(IEC104_U_TESTFR_CON, out);
        return 0;
    }
    if (rc < 0) return -1;

    const uint8_t *asdu_raw = in + 6;
    size_t asdu_len = (size_t)in[1] - 4; /* in[1] = 控制域(4) + ASDU 字节数 */

    iec_type_id_t type;
    bool sq;
    uint8_t count;
    iec_cot_t cot;
    uint16_t casdu;
    if (iec_asdu_parse_header(&asdu, &type, &sq, &count, &cot, &casdu, &IEC_ADDR_CONF_104) != 0) {
        return 0;
    }

    if (type == IEC_TYPE_C_IC_NA_1 && cot == IEC_COT_ACT) {
        uint32_t ioa = 0;
        uint8_t qoi = 0;
        if (iec_asdu_parse_c_ic_na_1(&asdu, &ioa, &qoi, &IEC_ADDR_CONF_104) != 0) return 0;

        if (qoi != IEC104_QOI_STATION) {
            /* v3 对其他 QOI 回负确认（:183-187）。lib60870 的
             * IMasterConnection_sendACT_CON(negative=true) 把 COT 设为
             * ACT_CON(7) 并置 P/N 位（cs104_slave.c 实现），线上首字节 0x47。 */
            LOG_INFO(LOG_CAT_IO, "IEC104: 总召唤 QOI=%u 不支持，回负 ACT_CON", qoi);
            iec104_echo_i(s, out, asdu_raw, asdu_len, IEC_COT_ACT_CON, true);
            return 0;
        }

        /* 总召唤：ACT_CON → 按类型分组的 INTROGEN → ACT_TERM。确认帧回显
         * 请求 ASDU（带 IOA 与 QOI），v3 的库按同一帧结构生成（:199,344）。 */
        iec104_echo_i(s, out, asdu_raw, asdu_len, IEC_COT_ACT_CON, false);

        /* v3 在正 ACT_CON 之后才按 CA 找点表；找不到直接 ACT_TERM，不发数据
         * （:199-210）。配置 CA 来自会话的 device_id（组合根传 1）。 */
        if (casdu != s->casdu) {
            LOG_WARN(LOG_CAT_IO, "IEC104: 总召 CASDU=%u 未配置（配置 %u），只回 ACT_CON/TERM",
                     casdu, s->casdu);
            iec104_echo_i(s, out, asdu_raw, asdu_len, IEC_COT_ACT_TERM, false);
            return 0;
        }

        iec104_gi_ctx_t g = {.s = s, .src = src, .src_ctx = src_ctx, .out = out, .sent = 0};
        if (src && src->enumerate) {
            src->enumerate(src_ctx, iec104_gi_emit, &g);
        }
        for (int i = 0; i < IEC104_GI_GROUPS; i++) iec104_gi_flush(&g, i);

        iec104_echo_i(s, out, asdu_raw, asdu_len, IEC_COT_ACT_TERM, false);
        return 0;
    }

    if (type == IEC_TYPE_C_SC_NA_1 || type == IEC_TYPE_C_DC_NA_1 || type == IEC_TYPE_C_SE_NC_1) {
        /* v3 的 asduHandler 顺序：COT 非 ACT 先回 45，CA 不认识回 46，再看点表
         * （iems_v3/src/iec104_server/iec104_server.c:437-481）。 */
        if (cot != IEC_COT_ACT) {
            iec104_echo_i(s, out, asdu_raw, asdu_len, IEC_COT_UNKNOWN_COT, false);
            return 0;
        }
        if (casdu != s->casdu) {
            LOG_WARN(LOG_CAT_IO, "IEC104: 命令 CASDU=%u 未配置（配置 %u），回 COT=46", casdu,
                     s->casdu);
            iec104_echo_i(s, out, asdu_raw, asdu_len, IEC_COT_UNKNOWN_CA, false);
            return 0;
        }

        if (type == IEC_TYPE_C_DC_NA_1) {
            /* v3 的命令 switch 没有 C_DC 分支，落 default 回 COT=44
             * （iems_v3/src/iec104_server/iec104_server.c:485-523）。DCO 状态取
             * 低两位再记录：ON(2) 不能按单命令位读成分闸。不落写。
             * 与 v3 的剩余差异：v3 在 switch 之前先查控制点表，未映射回 47。 */
            uint32_t ioa = 0;
            uint8_t dco = 0;
            if (iec_asdu_parse_c_sc_na_1(&asdu, &ioa, &dco, &IEC_ADDR_CONF_104) == 0) {
                LOG_INFO(LOG_CAT_IO, "IEC104: C_DC IOA=%u DCO=%u 不支持，回 COT=44", ioa,
                         dco & 0x03);
            }
            iec104_echo_i(s, out, asdu_raw, asdu_len, IEC_COT_UNKNOWN_TYPE_ID, false);
            return 0;
        }

        {
            uint32_t ioa = 0;
            double val = 0.0;
            bool ok = false;

            if (type == IEC_TYPE_C_SE_NC_1) {
                float f = 0.0f;
                uint8_t qos = 0;
                if (iec_asdu_parse_c_se_nc_1(&asdu, &ioa, &f, &qos, &IEC_ADDR_CONF_104) == 0) {
                    val = (double)f;
                    ok = true;
                }
            } else {
                uint8_t scs = 0;
                if (iec_asdu_parse_c_sc_na_1(&asdu, &ioa, &scs, &IEC_ADDR_CONF_104) == 0) {
                    val = (double)(scs & 0x01);
                    ok = true;
                }
            }
            if (!ok) return 0; /* 畸形帧连 IOA 都没有，没法回显 */

            int accept_rc =
                (src && src->accept) ? src->accept(src_ctx, IEC_NATIVE_ID(type, ioa), val) : -1;
            /* v3 先写 tag 再回显（:528-532）；写被拒不能回正 7，未映射 IOA 回 47（:476-481）。 */
            iec104_echo_i(s, out, asdu_raw, asdu_len,
                          accept_rc == 0 ? IEC_COT_ACT_CON : IEC_COT_UNKNOWN_IOA, false);
            return 0;
        }
    }

    return 0;
}

/* ==================== 周期发帧 ==================== */

typedef struct {
    iec104_session_t *s;
    const edge_slave_source_ops_t *src;
    void *src_ctx;
    net_buf_pool_t *pool;
    net_buf_t *out;
} iec104_spont_ctx_t;

/* 自发上报遍历回调：只报变化超过死区的点 */
static void iec104_spont_emit(void *ud, uint32_t point_id) {
    iec104_spont_ctx_t *c = (iec104_spont_ctx_t *)ud;
    uint8_t type_id = IEC_NATIVE_TYPE(point_id);
    uint32_t ioa = IEC_NATIVE_IOA(point_id);
    if (type_id == 0) type_id = IEC_TYPE_M_ME_NC_1;

    edge_point_value_t v;
    memset(&v, 0, sizeof(v));
    if (!c->src || !c->src->resolve) return;
    if (c->src->resolve(c->src_ctx, point_id, NULL, 0, &v) != 0) return;

    iec104_cache_slot_t *slot = iec104_cache_find(c->s, point_id, true);
    if (!slot) return; /* 缓存满了：宁可不报，也不动态分配 */
    double deadband = IEC104_DEFAULT_DEADBAND;
    if (c->src->deadband) {
        double d = c->src->deadband(c->src_ctx, point_id);
        if (d > 0.0) deadband = d;
    }
    bool changed = slot->sampled && fabs(v.value - slot->last) > deadband;
    slot->sampled = true;
    slot->last = v.value;
    if (!changed) return;

    if (!c->out) {
        c->out = net_buf_alloc(c->pool, 0);
        if (!c->out) return;
    }
    net_buf_t *df = net_buf_alloc(c->pool, 0);
    if (!df) return;
    iec_asdu_init(df, (iec_type_id_t)type_id, false, 1, IEC_COT_SPONT, c->s->casdu,
                  &IEC_ADDR_CONF_104);
    iec_asdu_add_any(df, type_id, ioa, v.value, 0, 0, (iec_qds_t){0}, &IEC_ADDR_CONF_104);
    if (iec104_apci_wrap_i(c->s->apci, df) == 0) {
        net_buf_simple_add_mem(&c->out->b, df->b.data, df->b.len);
        slot->last = v.value;
    }
    net_buf_unref(df);
}

static int iec104_req_tick(void *priv_v, const edge_north_source_ops_t *north, void *north_ctx,
                           const edge_slave_source_ops_t *slave, void *slave_ctx,
                           net_buf_pool_t *pool, net_buf_t **out_tx) {
    (void)north; /* 104 不发北向报文 */
    (void)north_ctx;
    iec104_session_t *s = (iec104_session_t *)priv_v;
    if (!s || !pool || !out_tx) return -1;

    uint64_t now = uv_hrtime() / 1000000;

    /* 1. 主站建链 */
    if (iec104_apci_get_state(s->apci) == IEC104_STATE_DISCONNECTED && s->needs_gi) {
        *out_tx = net_buf_alloc(pool, 0);
        if (!*out_tx) return -1;
        iec104_apci_build_u(IEC104_U_STARTDT_ACT, *out_tx);
        iec104_apci_set_state(s->apci, IEC104_STATE_STARTING);
        return 0;
    }

    /* 2. 建链后总召唤一次 */
    if (iec104_apci_get_state(s->apci) == IEC104_STATE_CONNECTED && s->needs_gi) {
        *out_tx = net_buf_alloc(pool, 0);
        if (!*out_tx) return -1;
        iec_asdu_init(*out_tx, IEC_TYPE_C_IC_NA_1, false, 1, IEC_COT_ACT, s->casdu,
                      &IEC_ADDR_CONF_104);
        iec_asdu_add_c_ic_na_1(*out_tx, 0, 20, &IEC_ADDR_CONF_104);
        iec104_apci_wrap_i(s->apci, *out_tx);
        s->needs_gi = false;
        s->last_gi_ts = now;
        LOG_INFO(LOG_CAT_IO, "IEC104: 已发总召唤 (GI)");
        return 0;
    }

    /* 3. APCI 定时器（t1 断链 / t2 S 帧 / t3 TESTFR）必须先于自发上报执行：
     * 自发分支每 tick 都有帧时会提前 return，定时器将永远不被检查。 */
    iec104_apci_event_t evt = iec104_apci_tick(s->apci, now);
    if (evt == IEC104_EVENT_T1_TIMEOUT) {
        /* I 帧 15s 未确认：协议上没有重发队列可恢复，唯一正确反应是断链，
         * 让主站重连后从 N(S)=0 重来。断 socket 是产品侧的事（close_link
         * 端口）；这里复位状态，后续 tick 不再重复报同一事件。 */
        if (slave && slave->close_link) slave->close_link(slave_ctx);
        iec104_apci_set_state(s->apci, IEC104_STATE_DISCONNECTED);
        s->transfer_active = false;
        LOG_WARN(LOG_CAT_IO, "IEC104: t1 超时（I 帧 15s 未确认），断开客户端");
        return 0; /* 状态事件，不产生帧 */
    }

    /* 4. 从站自发上报（测点变化 → SPONT）；STOPDT 后暂停，STARTDT 恢复 */
    if (iec104_apci_get_state(s->apci) == IEC104_STATE_CONNECTED && s->transfer_active &&
        !s->needs_gi && slave && slave->enumerate) {
        iec104_spont_ctx_t c = {.s = s, .src = slave, .src_ctx = slave_ctx, .pool = pool,
                                .out = NULL};
        slave->enumerate(slave_ctx, iec104_spont_emit, &c);
        if (c.out && c.out->b.len > 0) {
            /* I 帧自带 N(R)，S 帧不用再发；但本 tick 已到点的 t3 TESTFR 不能丢，
             * 追加在同一次写里（TCP 流允许连续两个 APDU）。 */
            if (evt == IEC104_EVENT_SEND_TESTFR) {
                net_buf_t *u = net_buf_alloc(pool, 0);
                if (u) {
                    iec104_apci_build_u(IEC104_U_TESTFR_ACT, u);
                    if (net_buf_simple_tailroom(&c.out->b) >= u->b.len) {
                        net_buf_simple_add_mem(&c.out->b, u->b.data, u->b.len);
                    }
                    net_buf_unref(u);
                }
            }
            *out_tx = c.out;
            return 0;
        }
        if (c.out) net_buf_unref(c.out); /* 滑窗满：本次没组出 I 帧 */
    }

    /* 5. 没有自发帧时才补发 S / TESTFR */
    if (evt == IEC104_EVENT_SEND_S) {
        *out_tx = net_buf_alloc(pool, 0);
        if (!*out_tx) return -1;
        iec104_apci_build_s(s->apci, *out_tx);
        return 0;
    }
    if (evt == IEC104_EVENT_SEND_TESTFR) {
        *out_tx = net_buf_alloc(pool, 0);
        if (!*out_tx) return -1;
        iec104_apci_build_u(IEC104_U_TESTFR_ACT, *out_tx);
        return 0;
    }
    return -1;
}

const edge_driver_t iec104_driver = {
    .name = "IEC104",
    .ctx_size = sizeof(iec104_session_t),  /* 序号与状态机必须跨帧 */
    .req_priv_size = sizeof(iec104_req_t), /* 请求 scratch 只放这一个点 */
    .on_init = iec104_on_init,
    .req_prepare = iec104_req_prepare,
    .req_encode = iec104_req_encode,
    .req_decode = iec104_req_decode,
    .req_slave_exchange = iec104_req_slave_exchange,
    .req_tick = iec104_req_tick,
};

int iec104_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &iec104_driver);
}
