/**
 * @file dcu_698_comm.c
 * @brief Class 25/26 通信参数（4500..4501 无线公网、4510..4517 以太网）纯软件配置领域引擎
 */

#include "dcu_698_comm.h"

#include <string.h>

#include "dcu_698_axdr_internal.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define COMM_BLOB_MAX 192u
#define COMM_DEFAULT_HEARTBEAT_S 60u

static dcu_comm_wireless_cfg_t g_wireless[DCU_COMM_WIRELESS_COUNT];
static dcu_comm_eth_cfg_t g_eth[DCU_COMM_ETH_COUNT];
static dcu_comm_store_t g_store;

static int _wireless_idx(uint16_t oi) {
    if (oi >= DCU_COMM_WIRELESS_OI_FIRST && oi <= DCU_COMM_WIRELESS_OI_LAST) {
        return (int)(oi - DCU_COMM_WIRELESS_OI_FIRST);
    }
    return -1;
}

static int _eth_idx(uint16_t oi) {
    if (oi >= DCU_COMM_ETH_OI_FIRST && oi <= DCU_COMM_ETH_OI_LAST) {
        return (int)(oi - DCU_COMM_ETH_OI_FIRST);
    }
    return -1;
}

static void _copy_str_bounded(char *dst, size_t dst_cap, const char *src) {
    if (dst_cap == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t n = strnlen(src, dst_cap - 1u);
    if (n > 0) memcpy(dst, src, n);
    dst[n] = '\0';
}

static bool _ip_nonzero(const uint8_t ip[4]) {
    return (ip[0] | ip[1] | ip[2] | ip[3]) != 0;
}

static int _put_bytes(uint8_t *buf, size_t cap, size_t *off, const uint8_t *src, size_t n) {
    if (*off + n > cap) return -1;
    if (n > 0) memcpy(buf + *off, src, n);
    *off += n;
    return 0;
}

static int _put_octet_string(uint8_t *buf, size_t cap, size_t *off, const uint8_t *data,
                             uint8_t len) {
    if (_put_raw_u8(buf, cap, off, DLT698_TAG_OCTET_STRING) != 0 ||
        _put_raw_u8(buf, cap, off, len) != 0 ||
        _put_bytes(buf, cap, off, data, len) != 0) {
        return -1;
    }
    return 0;
}

static int _put_visible_string(uint8_t *buf, size_t cap, size_t *off, const char *s,
                               size_t max_len) {
    size_t len = s ? strnlen(s, max_len) : 0;
    if (_put_raw_u8(buf, cap, off, DLT698_TAG_VISIBLE_STRING) != 0 ||
        _put_raw_u8(buf, cap, off, (uint8_t)len) != 0 ||
        _put_bytes(buf, cap, off, (const uint8_t *)s, len) != 0) {
        return -1;
    }
    return 0;
}

static int _put_master_entry(uint8_t *buf, size_t cap, size_t *off, const uint8_t ip[4],
                             uint16_t port) {
    if (_put_raw_u8(buf, cap, off, DLT698_TAG_STRUCTURE) != 0 ||
        _put_raw_u8(buf, cap, off, 2) != 0 ||
        _put_octet_string(buf, cap, off, ip, 4) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, port) != 0) {
        return -1;
    }
    return 0;
}

static int _take_visible_str(axdr_cursor_t *c, char *dst, size_t dst_cap) {
    const uint8_t *hdr = NULL;
    const uint8_t *bytes = NULL;
    if (!_take(c, 2, &hdr) || hdr[0] != DLT698_TAG_VISIBLE_STRING) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    uint8_t slen = hdr[1];
    if (!_take(c, slen, &bytes)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (dst && dst_cap > 0) {
        size_t copy_len = slen < (dst_cap - 1u) ? slen : (dst_cap - 1u);
        if (copy_len > 0) memcpy(dst, bytes, copy_len);
        dst[copy_len] = '\0';
    }
    return 0;
}

static int _take_octet_str(axdr_cursor_t *c, uint8_t *dst, size_t expected_len,
                           bool allow_empty, bool *was_empty) {
    const uint8_t *hdr = NULL;
    const uint8_t *bytes = NULL;
    if (!_take(c, 2, &hdr) || hdr[0] != DLT698_TAG_OCTET_STRING) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    uint8_t olen = hdr[1];
    if (!_take(c, olen, &bytes)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (olen == 0 && allow_empty) {
        if (dst && expected_len > 0) memset(dst, 0, expected_len);
        if (was_empty) *was_empty = true;
        return 0;
    }
    if (olen != expected_len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (dst && expected_len > 0) memcpy(dst, bytes, expected_len);
    if (was_empty) *was_empty = false;
    return 0;
}

static int _take_master_entry(axdr_cursor_t *c, uint8_t ip_out[4], uint16_t *port_out) {
    if (_take_head(c, DLT698_TAG_STRUCTURE, 2) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (_take_octet_str(c, ip_out, 4, false, NULL) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (_take_u16(c, DLT698_TAG_LONG_UNSIGNED, port_out) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (*port_out == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    return 0;
}

static int _encode_wireless_attr(const dcu_comm_wireless_cfg_t *cfg, uint16_t oi, uint8_t attr_id,
                                 uint8_t *buf, size_t cap, size_t *out_len) {
    size_t off = 0;

    if (attr_id == 1) {
        uint8_t logical_name[2] = {(uint8_t)(oi >> 8), (uint8_t)oi};
        if (_put_octet_string(buf, cap, &off, logical_name, 2) != 0) return -1;
        *out_len = off;
        return 0;
    }
    if (attr_id == 2) {
        /* 表197 属性2 通信配置：12 项 structure */
        if (_put_raw_u8(buf, cap, &off, DLT698_TAG_STRUCTURE) != 0 ||
            _put_raw_u8(buf, cap, &off, 12) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_ENUM, 1) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_ENUM, 0) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_ENUM, 0) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_ENUM, 0) != 0 ||
            _put_raw_u8(buf, cap, &off, DLT698_TAG_ARRAY) != 0 ||
            _put_raw_u8(buf, cap, &off, 0) != 0 ||
            _put_visible_string(buf, cap, &off, cfg->apn, DCU_COMM_APN_MAX - 1u) != 0 ||
            _put_visible_string(buf, cap, &off, cfg->user, DCU_COMM_USER_MAX - 1u) != 0 ||
            _put_visible_string(buf, cap, &off, cfg->password, DCU_COMM_PASS_MAX - 1u) != 0 ||
            _put_octet_string(buf, cap, &off, NULL, 0) != 0 ||
            _put_tagged_u16(buf, cap, &off, DLT698_TAG_LONG_UNSIGNED, 0) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_UNSIGNED, 0) != 0 ||
            _put_tagged_u16(buf, cap, &off, DLT698_TAG_LONG_UNSIGNED, cfg->heartbeat_s) != 0) {
            return -1;
        }
        *out_len = off;
        return 0;
    }
    if (attr_id == 3) {
        bool has_backup = (cfg->backup_port != 0) || _ip_nonzero(cfg->backup_ip);
        bool has_primary = (cfg->master_port != 0) || _ip_nonzero(cfg->master_ip) || has_backup;
        uint8_t cnt = has_backup ? 2 : (has_primary ? 1 : 0);
        if (_put_raw_u8(buf, cap, &off, DLT698_TAG_ARRAY) != 0 ||
            _put_raw_u8(buf, cap, &off, cnt) != 0) {
            return -1;
        }
        if (cnt >= 1 &&
            _put_master_entry(buf, cap, &off, cfg->master_ip, cfg->master_port) != 0) {
            return -1;
        }
        if (cnt >= 2 &&
            _put_master_entry(buf, cap, &off, cfg->backup_ip, cfg->backup_port) != 0) {
            return -1;
        }
        *out_len = off;
        return 0;
    }
    if (attr_id >= 4 && attr_id <= 11) return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    return EDGE_SLAVE_SRC_UNDEFINED;
}

static int _encode_eth_attr(const dcu_comm_eth_cfg_t *cfg, uint16_t oi, uint8_t attr_id,
                            uint8_t *buf, size_t cap, size_t *out_len) {
    size_t off = 0;

    if (attr_id == 1) {
        uint8_t logical_name[2] = {(uint8_t)(oi >> 8), (uint8_t)oi};
        if (_put_octet_string(buf, cap, &off, logical_name, 2) != 0) return -1;
        *out_len = off;
        return 0;
    }
    if (attr_id == 2) {
        /* 表200 属性2 通信配置：8 项 structure */
        if (_put_raw_u8(buf, cap, &off, DLT698_TAG_STRUCTURE) != 0 ||
            _put_raw_u8(buf, cap, &off, 8) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_ENUM, 1) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_ENUM, 0) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_ENUM, 0) != 0 ||
            _put_raw_u8(buf, cap, &off, DLT698_TAG_ARRAY) != 0 ||
            _put_raw_u8(buf, cap, &off, 0) != 0 ||
            _put_octet_string(buf, cap, &off, NULL, 0) != 0 ||
            _put_tagged_u16(buf, cap, &off, DLT698_TAG_LONG_UNSIGNED, 0) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_UNSIGNED, 0) != 0 ||
            _put_tagged_u16(buf, cap, &off, DLT698_TAG_LONG_UNSIGNED, COMM_DEFAULT_HEARTBEAT_S) !=
                0) {
            return -1;
        }
        *out_len = off;
        return 0;
    }
    if (attr_id == 3) {
        bool has_master = (cfg->master_port != 0) || _ip_nonzero(cfg->master_ip);
        uint8_t cnt = has_master ? 1 : 0;
        if (_put_raw_u8(buf, cap, &off, DLT698_TAG_ARRAY) != 0 ||
            _put_raw_u8(buf, cap, &off, cnt) != 0) {
            return -1;
        }
        if (cnt == 1 && _put_master_entry(buf, cap, &off, cfg->master_ip, cfg->master_port) != 0) {
            return -1;
        }
        *out_len = off;
        return 0;
    }
    if (attr_id == 4) {
        /* 表200 属性4 网络配置：6 项 structure */
        if (_put_raw_u8(buf, cap, &off, DLT698_TAG_STRUCTURE) != 0 ||
            _put_raw_u8(buf, cap, &off, 6) != 0 ||
            _put_tagged_u8(buf, cap, &off, DLT698_TAG_ENUM, cfg->ip_mode) != 0 ||
            _put_octet_string(buf, cap, &off, cfg->ip, 4) != 0 ||
            _put_octet_string(buf, cap, &off, cfg->netmask, 4) != 0 ||
            _put_octet_string(buf, cap, &off, cfg->gateway, 4) != 0 ||
            _put_visible_string(buf, cap, &off, "", 0) != 0 ||
            _put_visible_string(buf, cap, &off, "", 0) != 0) {
            return -1;
        }
        *out_len = off;
        return 0;
    }
    if (attr_id == 5) return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    return EDGE_SLAVE_SRC_UNDEFINED;
}

static int _persist_attr(uint16_t oi, uint8_t attr_id) {
    if (!g_store.save) return 0;
    uint8_t blob[COMM_BLOB_MAX];
    size_t len = 0;
    if (dcu_698_comm_encode_attr(oi, attr_id, blob, sizeof(blob), &len) != 0) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (g_store.save(g_store.ctx, oi, attr_id, blob, len) != 0) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

static int _decode_wireless_a2(axdr_cursor_t *c, dcu_comm_wireless_cfg_t *cfg) {
    if (_take_head(c, DLT698_TAG_STRUCTURE, 12) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint8_t work_mode = 0, online_mode = 0, conn_mode = 0, app_mode = 0;
    if (_take_u8(c, DLT698_TAG_ENUM, &work_mode) != 0 || work_mode > 2 ||
        _take_u8(c, DLT698_TAG_ENUM, &online_mode) != 0 || online_mode > 1 ||
        _take_u8(c, DLT698_TAG_ENUM, &conn_mode) != 0 || conn_mode > 1 ||
        _take_u8(c, DLT698_TAG_ENUM, &app_mode) != 0 || app_mode > 1) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    uint8_t listen_cnt = 0;
    if (_take_array(c, &listen_cnt) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    for (uint8_t i = 0; i < listen_cnt; i++) {
        if (_take_u16(c, DLT698_TAG_LONG_UNSIGNED, NULL) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
    }

    if (_take_visible_str(c, cfg->apn, sizeof(cfg->apn)) != 0 ||
        _take_visible_str(c, cfg->user, sizeof(cfg->user)) != 0 ||
        _take_visible_str(c, cfg->password, sizeof(cfg->password)) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    const uint8_t *proxy_hdr = NULL;
    if (!_take(c, 2, &proxy_hdr) || proxy_hdr[0] != DLT698_TAG_OCTET_STRING ||
        !_take(c, proxy_hdr[1], NULL)) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (_take_u16(c, DLT698_TAG_LONG_UNSIGNED, NULL) != 0 ||
        _take_u8(c, DLT698_TAG_UNSIGNED, NULL) != 0 ||
        _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &cfg->heartbeat_s) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    return _time_tag_absent(c->p, c->off, c->len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static int _decode_wireless_a3(axdr_cursor_t *c, dcu_comm_wireless_cfg_t *cfg) {
    uint8_t cnt = 0;
    if (_take_array(c, &cnt) != 0 || cnt == 0 || cnt > 2) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (_take_master_entry(c, cfg->master_ip, &cfg->master_port) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (cnt == 2) {
        if (_take_master_entry(c, cfg->backup_ip, &cfg->backup_port) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
    } else {
        memset(cfg->backup_ip, 0, sizeof(cfg->backup_ip));
        cfg->backup_port = 0;
    }
    return _time_tag_absent(c->p, c->off, c->len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static int _decode_eth_a3(axdr_cursor_t *c, dcu_comm_eth_cfg_t *cfg) {
    uint8_t cnt = 0;
    if (_take_array(c, &cnt) != 0 || cnt != 1) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (_take_master_entry(c, cfg->master_ip, &cfg->master_port) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    return _time_tag_absent(c->p, c->off, c->len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static int _decode_eth_a4(axdr_cursor_t *c, dcu_comm_eth_cfg_t *cfg) {
    if (_take_head(c, DLT698_TAG_STRUCTURE, 6) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint8_t mode = 0;
    if (_take_u8(c, DLT698_TAG_ENUM, &mode) != 0 || mode > DCU_ETH_MODE_PPPOE) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    bool allow_empty = (mode == DCU_ETH_MODE_DHCP);
    if (_take_octet_str(c, cfg->ip, 4, allow_empty, NULL) != 0 ||
        _take_octet_str(c, cfg->netmask, 4, allow_empty, NULL) != 0 ||
        _take_octet_str(c, cfg->gateway, 4, allow_empty, NULL) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (_take_visible_str(c, NULL, 0) != 0 || _take_visible_str(c, NULL, 0) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    cfg->ip_mode = mode;
    return _time_tag_absent(c->p, c->off, c->len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

void dcu_698_comm_init(const dcu_comm_store_t *store) {
    memset(g_wireless, 0, sizeof(g_wireless));
    memset(g_eth, 0, sizeof(g_eth));
    for (size_t i = 0; i < DCU_COMM_WIRELESS_COUNT; i++) {
        g_wireless[i].heartbeat_s = COMM_DEFAULT_HEARTBEAT_S;
    }
    for (size_t i = 0; i < DCU_COMM_ETH_COUNT; i++) {
        g_eth[i].ip_mode = DCU_ETH_MODE_DHCP;
    }

    g_store = store ? *store : (dcu_comm_store_t){0};
    if (!g_store.load) return;

    uint8_t blob[COMM_BLOB_MAX];
    for (size_t i = 0; i < DCU_COMM_WIRELESS_COUNT; i++) {
        uint16_t oi = (uint16_t)(DCU_COMM_WIRELESS_OI_FIRST + i);
        for (uint8_t attr = 2; attr <= 3; attr++) {
            size_t len = 0;
            if (g_store.load(g_store.ctx, oi, attr, blob, sizeof(blob), &len) == 0 && len > 0) {
                axdr_cursor_t c = {blob, len, 0};
                dcu_comm_wireless_cfg_t tmp = g_wireless[i];
                int rc = (attr == 2) ? _decode_wireless_a2(&c, &tmp)
                                     : _decode_wireless_a3(&c, &tmp);
                if (rc == 0) g_wireless[i] = tmp;
            }
        }
    }
    for (size_t i = 0; i < DCU_COMM_ETH_COUNT; i++) {
        uint16_t oi = (uint16_t)(DCU_COMM_ETH_OI_FIRST + i);
        for (uint8_t attr = 3; attr <= 4; attr++) {
            size_t len = 0;
            if (g_store.load(g_store.ctx, oi, attr, blob, sizeof(blob), &len) == 0 && len > 0) {
                axdr_cursor_t c = {blob, len, 0};
                dcu_comm_eth_cfg_t tmp = g_eth[i];
                int rc = (attr == 3) ? _decode_eth_a3(&c, &tmp) : _decode_eth_a4(&c, &tmp);
                if (rc == 0) g_eth[i] = tmp;
            }
        }
    }
}

bool dcu_698_comm_is_oi(uint16_t oi) {
    return _wireless_idx(oi) >= 0 || _eth_idx(oi) >= 0;
}

int dcu_698_comm_set_wireless(uint16_t oi, const dcu_comm_wireless_cfg_t *cfg) {
    int idx = _wireless_idx(oi);
    if (idx < 0) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (!cfg || cfg->master_port == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (_ip_nonzero(cfg->backup_ip) && cfg->backup_port == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    dcu_comm_wireless_cfg_t prev = g_wireless[idx];
    dcu_comm_wireless_cfg_t next = {0};
    _copy_str_bounded(next.apn, sizeof(next.apn), cfg->apn);
    _copy_str_bounded(next.user, sizeof(next.user), cfg->user);
    _copy_str_bounded(next.password, sizeof(next.password), cfg->password);
    memcpy(next.master_ip, cfg->master_ip, 4);
    next.master_port = cfg->master_port;
    memcpy(next.backup_ip, cfg->backup_ip, 4);
    next.backup_port = cfg->backup_port;
    next.heartbeat_s = cfg->heartbeat_s;

    g_wireless[idx] = next;
    if (_persist_attr(oi, 2) != 0 || _persist_attr(oi, 3) != 0) {
        g_wireless[idx] = prev;
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

int dcu_698_comm_get_wireless(uint16_t oi, dcu_comm_wireless_cfg_t *out) {
    int idx = _wireless_idx(oi);
    if (idx < 0) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (!out) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *out = g_wireless[idx];
    return 0;
}

int dcu_698_comm_set_eth(uint16_t oi, const dcu_comm_eth_cfg_t *cfg) {
    int idx = _eth_idx(oi);
    if (idx < 0) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (!cfg || cfg->ip_mode > DCU_ETH_MODE_PPPOE || cfg->master_port == 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    dcu_comm_eth_cfg_t prev = g_eth[idx];
    g_eth[idx] = *cfg;
    if (_persist_attr(oi, 3) != 0 || _persist_attr(oi, 4) != 0) {
        g_eth[idx] = prev;
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

int dcu_698_comm_get_eth(uint16_t oi, dcu_comm_eth_cfg_t *out) {
    int idx = _eth_idx(oi);
    if (idx < 0) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (!out) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *out = g_eth[idx];
    return 0;
}

int dcu_698_comm_encode_attr(uint16_t oi, uint8_t attr_id, uint8_t *buf, size_t cap,
                             size_t *out_len) {
    if (!buf || !out_len) return -1;
    *out_len = 0;

    int widx = _wireless_idx(oi);
    if (widx >= 0) {
        return _encode_wireless_attr(&g_wireless[widx], oi, attr_id, buf, cap, out_len);
    }
    int eidx = _eth_idx(oi);
    if (eidx >= 0) {
        return _encode_eth_attr(&g_eth[eidx], oi, attr_id, buf, cap, out_len);
    }
    return EDGE_SLAVE_SRC_NOT_FOUND;
}

int dcu_698_comm_decode_set(uint16_t oi, uint8_t attr_id, const uint8_t *axdr, size_t len) {
    int widx = _wireless_idx(oi);
    if (widx >= 0) {
        if (attr_id == 1 || (attr_id >= 4 && attr_id <= 11)) {
            return (attr_id == 1) ? EDGE_SLAVE_SRC_DENIED : EDGE_SLAVE_SRC_CLASS_MISMATCH;
        }
        if (attr_id != 2 && attr_id != 3) return EDGE_SLAVE_SRC_UNDEFINED;
        if (!axdr || len == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

        axdr_cursor_t c = {axdr, len, 0};
        dcu_comm_wireless_cfg_t prev = g_wireless[widx];
        dcu_comm_wireless_cfg_t next = prev;
        int rc = (attr_id == 2) ? _decode_wireless_a2(&c, &next)
                                : _decode_wireless_a3(&c, &next);
        if (rc != 0) return rc;

        g_wireless[widx] = next;
        if (_persist_attr(oi, attr_id) != 0) {
            g_wireless[widx] = prev;
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        return 0;
    }

    int eidx = _eth_idx(oi);
    if (eidx >= 0) {
        if (attr_id == 1 || attr_id == 2 || attr_id == 5) {
            return (attr_id == 1) ? EDGE_SLAVE_SRC_DENIED : EDGE_SLAVE_SRC_CLASS_MISMATCH;
        }
        if (attr_id != 3 && attr_id != 4) return EDGE_SLAVE_SRC_UNDEFINED;
        if (!axdr || len == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

        axdr_cursor_t c = {axdr, len, 0};
        dcu_comm_eth_cfg_t prev = g_eth[eidx];
        dcu_comm_eth_cfg_t next = prev;
        int rc = (attr_id == 3) ? _decode_eth_a3(&c, &next) : _decode_eth_a4(&c, &next);
        if (rc != 0) return rc;

        g_eth[eidx] = next;
        if (_persist_attr(oi, attr_id) != 0) {
            g_eth[eidx] = prev;
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        return 0;
    }

    return EDGE_SLAVE_SRC_NOT_FOUND;
}
