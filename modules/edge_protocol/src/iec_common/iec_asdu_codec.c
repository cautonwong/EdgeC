/**
 * @file iec_asdu_codec.c
 * @brief IEC 60870-5-101/104 共享 ASDU 编解码内核实现
 */

#include <string.h>
#include <time.h>

#include "edge_log.h"
#include "iec_asdu.h"
#include "iec_cp56time.h"
#include "net_buf.h"

void iec_asdu_init(net_buf_t *buf, iec_type_id_t type, bool sq, uint8_t count, iec_cot_t cot,
                   uint16_t casdu, const iec_addr_config_t *conf) {
    net_buf_reserve(buf, 16);
    struct net_buf_simple *b = &buf->b;
    net_buf_simple_add_u8(b, (uint8_t)type);
    uint8_t vsq = count & 0x7F;
    if (sq) vsq |= 0x80;
    net_buf_simple_add_u8(b, vsq);

    if (conf->cot_len == 1)
        net_buf_simple_add_u8(b, (uint8_t)cot);
    else
        net_buf_simple_add_le16(b, (uint16_t)cot);

    if (conf->casdu_len == 1)
        net_buf_simple_add_u8(b, (uint8_t)(casdu & 0xFF));
    else
        net_buf_simple_add_le16(b, casdu);
}

static void _encode_ioa(struct net_buf_simple *b, uint32_t ioa, uint8_t len) {
    if (len == 1)
        net_buf_simple_add_u8(b, (uint8_t)(ioa & 0xFF));
    else if (len == 2)
        net_buf_simple_add_le16(b, (uint16_t)(ioa & 0xFFFF));
    else if (len == 3) {
        uint8_t *p = net_buf_simple_add(b, 3);
        p[0] = (uint8_t)(ioa & 0xFF);
        p[1] = (uint8_t)((ioa >> 8) & 0xFF);
        p[2] = (uint8_t)((ioa >> 16) & 0xFF);
    }
}

static uint32_t _decode_ioa(struct net_buf_simple *b, uint8_t len) {
    if (len == 1) return net_buf_simple_pull_u8(b);
    if (len == 2) return net_buf_simple_pull_le16(b);
    if (len == 3) {
        uint8_t *p = net_buf_simple_pull(b, 3);
        return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16));
    }
    return 0;
}

int iec_asdu_add_m_sp_na_1(net_buf_t *buf, uint32_t ioa, bool val, iec_qds_t qds, bool sq,
                           bool is_first, const iec_addr_config_t *conf) {
    struct net_buf_simple *b = &buf->b;
    if (!sq || is_first) _encode_ioa(b, ioa, conf->ioa_len);
    uint8_t siq = val ? 0x01 : 0x00;
    siq |= (uint8_t)(((*(uint8_t *)&qds) & 0x1F) << 4);
    net_buf_simple_add_u8(b, siq);
    return 0;
}

int iec_asdu_parse_m_sp_na_1(struct net_buf_simple *b, uint32_t *ioa, bool *val, iec_qds_t *qds,
                             bool sq, bool is_first, const iec_addr_config_t *conf) {
    if (!sq || is_first)
        *ioa = _decode_ioa(b, conf->ioa_len);
    else
        (*ioa)++;
    if (b->len < 1) return -1;
    uint8_t siq = net_buf_simple_pull_u8(b);
    *val = (siq & 0x01) != 0;
    *(uint8_t *)qds = (uint8_t)(siq >> 4);
    return 0;
}

int iec_asdu_add_m_dp_na_1(net_buf_t *buf, uint32_t ioa, uint8_t val, iec_qds_t qds, bool sq,
                           bool is_first, const iec_addr_config_t *conf) {
    struct net_buf_simple *b = &buf->b;
    if (!sq || is_first) _encode_ioa(b, ioa, conf->ioa_len);
    uint8_t diq = (uint8_t)(val & 0x03);
    diq |= (uint8_t)(((*(uint8_t *)&qds) & 0x1F) << 4);
    net_buf_simple_add_u8(b, diq);
    return 0;
}

int iec_asdu_add_m_me_nc_1(net_buf_t *buf, uint32_t ioa, float val, iec_qds_t qds, bool sq,
                           bool is_first, const iec_addr_config_t *conf) {
    struct net_buf_simple *b = &buf->b;
    if (!sq || is_first) _encode_ioa(b, ioa, conf->ioa_len);
    union {
        float f;
        uint32_t u;
    } u;
    u.f = val;
    net_buf_simple_add_le32(b, u.u);
    net_buf_simple_add_u8(b, *(uint8_t *)&qds);
    return 0;
}

int iec_asdu_parse_m_me_nc_1(struct net_buf_simple *b, uint32_t *ioa, float *val, iec_qds_t *qds,
                             bool sq, bool is_first, const iec_addr_config_t *conf) {
    if (!sq || is_first)
        *ioa = _decode_ioa(b, conf->ioa_len);
    else
        (*ioa)++;
    if (b->len < 5) return -1;
    union {
        float f;
        uint32_t u;
    } u;
    u.u = net_buf_simple_pull_le32(b);
    *val = u.f;
    *(uint8_t *)qds = net_buf_simple_pull_u8(b);
    return 0;
}

int iec_asdu_add_c_ic_na_1(net_buf_t *buf, uint32_t ioa, uint8_t qoi,
                           const iec_addr_config_t *conf) {
    _encode_ioa(&buf->b, ioa, conf->ioa_len);
    net_buf_simple_add_u8(&buf->b, qoi);
    return 0;
}

int iec_asdu_add_c_sc_na_1(net_buf_t *buf, uint32_t ioa, bool val, bool select,
                           const iec_addr_config_t *conf) {
    _encode_ioa(&buf->b, ioa, conf->ioa_len);
    uint8_t sco = val ? 0x01 : 0x00;
    if (select) sco |= 0x80;
    net_buf_simple_add_u8(&buf->b, sco);
    return 0;
}

int iec_asdu_add_c_dc_na_1(net_buf_t *buf, uint32_t ioa, uint8_t dcs, bool select,
                           const iec_addr_config_t *conf) {
    _encode_ioa(&buf->b, ioa, conf->ioa_len);
    uint8_t dco = (uint8_t)(dcs & 0x03);
    if (select) dco |= 0x80;
    net_buf_simple_add_u8(&buf->b, dco);
    return 0;
}

int iec_asdu_add_c_se_nc_1(net_buf_t *buf, uint32_t ioa, float val, uint8_t qos, bool select,
                           const iec_addr_config_t *conf) {
    _encode_ioa(&buf->b, ioa, conf->ioa_len);
    union {
        float f;
        uint32_t u;
    } u;
    u.f = val;
    net_buf_simple_add_le32(&buf->b, u.u);
    uint8_t qlv = (uint8_t)(qos & 0x7F);
    if (select) qlv |= 0x80;
    net_buf_simple_add_u8(&buf->b, qlv);
    return 0;
}

int iec_asdu_add_any(net_buf_t *buf, uint8_t type_id, uint32_t ioa, double val, uint64_t ts_ms,
                     uint32_t flags, iec_qds_t qds, const iec_addr_config_t *conf) {
    (void)flags;  // 暂时未用
    switch (type_id) {
        case IEC_TYPE_M_SP_NA_1:
            return iec_asdu_add_m_sp_na_1(buf, ioa, val > 0.5, qds, false, true, conf);
        case IEC_TYPE_M_ME_NC_1:
            return iec_asdu_add_m_me_nc_1(buf, ioa, (float)val, qds, false, true, conf);
        case IEC_TYPE_M_SP_TB_1: {
            iec_asdu_add_m_sp_na_1(buf, ioa, val > 0.5, qds, false, true, conf);
            iec_cp56time_t ts;
            if (ts_ms > 0)
                iec_cp56time_from_ms(&ts, ts_ms);
            else
                iec_cp56time_now(&ts);
            net_buf_simple_add_mem(&buf->b, &ts, 7);
            return 0;
        }
        case IEC_TYPE_M_ME_TF_1: {
            iec_asdu_add_m_me_nc_1(buf, ioa, (float)val, qds, false, true, conf);
            iec_cp56time_t ts;
            if (ts_ms > 0)
                iec_cp56time_from_ms(&ts, ts_ms);
            else
                iec_cp56time_now(&ts);
            net_buf_simple_add_mem(&buf->b, &ts, 7);
            return 0;
        }
        default:
            LOG_ERROR(LOG_CAT_IO, "IEC ASDU: Unsupported TypeID %d", type_id);
            return -1;
    }
}

int iec_asdu_parse_header(struct net_buf_simple *b, iec_type_id_t *type, bool *sq, uint8_t *count,
                          iec_cot_t *cot, uint16_t *casdu, const iec_addr_config_t *conf) {
    if (b->len < 4) return -1;
    *type = (iec_type_id_t)net_buf_simple_pull_u8(b);
    uint8_t vsq = net_buf_simple_pull_u8(b);
    *sq = (vsq & 0x80) != 0;
    *count = vsq & 0x7F;
    if (conf->cot_len == 1)
        *cot = (iec_cot_t)net_buf_simple_pull_u8(b);
    else
        *cot = (iec_cot_t)net_buf_simple_pull_le16(b);
    if (conf->casdu_len == 1)
        *casdu = (uint16_t)net_buf_simple_pull_u8(b);
    else
        *casdu = net_buf_simple_pull_le16(b);
    return 0;
}

int iec_asdu_parse_c_sc_na_1(struct net_buf_simple *b, uint32_t *ioa, uint8_t *scs, const iec_addr_config_t *conf) {
    if (b->len < (size_t)(conf->ioa_len + 1)) return -1;
    if (conf->ioa_len == 1) {
        *ioa = net_buf_simple_pull_u8(b);
    } else if (conf->ioa_len == 2) {
        *ioa = net_buf_simple_pull_le16(b);
    } else {
        uint32_t addr = net_buf_simple_pull_le16(b);
        addr |= ((uint32_t)net_buf_simple_pull_u8(b)) << 16;
        *ioa = addr;
    }
    *scs = net_buf_simple_pull_u8(b);
    return 0;
}

int iec_asdu_parse_c_se_nc_1(struct net_buf_simple *b, uint32_t *ioa, float *val, uint8_t *qos,
                             const iec_addr_config_t *conf) {
    if (b->len < (size_t)(conf->ioa_len + 5)) return -1;
    if (conf->ioa_len == 1) {
        *ioa = net_buf_simple_pull_u8(b);
    } else if (conf->ioa_len == 2) {
        *ioa = net_buf_simple_pull_le16(b);
    } else {
        uint32_t addr = net_buf_simple_pull_le16(b);
        addr |= ((uint32_t)net_buf_simple_pull_u8(b)) << 16;
        *ioa = addr;
    }
    union {
        float f;
        uint32_t u;
    } u;
    u.u = net_buf_simple_pull_le32(b);
    *val = u.f;
    *qos = net_buf_simple_pull_u8(b);
    return 0;
}