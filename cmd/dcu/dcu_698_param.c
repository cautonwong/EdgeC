/**
 * @file dcu_698_param.c
 * @brief Class 8 白名单参数：类型化值表 + 严格 A-XDR + 保存端口
 */

#include "dcu_698_param.h"

#include <string.h>
#include <time.h>

#include "dlt698_codec.h"
#include "edge_log.h"
#include "edge_slave_plane.h"

#define OI_DATETIME 0x4000u

#define P_BLOB_MAX 64

typedef enum {
    P_DATETIME_S,
    P_ENUM,
    P_UNSIGNED5,
    P_OCTETS,
    P_GEO,
    P_DLU,
    P_DLU2,
    P_DLU3
} dcu_param_kind_t;

typedef struct {
    uint16_t year;
    uint8_t month, day, hour, min, sec;
} dcu_param_when_t;

typedef struct {
    uint8_t dir, deg, min, sec;
} dcu_param_coord_t;

typedef struct {
    dcu_param_coord_t lon, lat;
    uint32_t height_cm;
} dcu_param_geo_t;

typedef struct {
    uint16_t oi;
    uint8_t attr;
    dcu_param_kind_t kind;
    uint8_t octet_max;
    union {
        dcu_param_when_t when;
        uint8_t enum8;
        uint16_t u16[5];
        struct {
            uint8_t len;
            uint8_t bytes[31];
        } octets;
        dcu_param_geo_t geo;
        uint32_t dlu[3];
    } v;
} dcu_param_t;

static dcu_param_t g_params[] = {
    {.oi = OI_DATETIME, .attr = 2, .kind = P_DATETIME_S},
    {.oi = OI_DATETIME, .attr = 3, .kind = P_ENUM},
    {.oi = OI_DATETIME, .attr = 4, .kind = P_UNSIGNED5},
    {.oi = 0x4001, .attr = 2, .kind = P_OCTETS, .octet_max = 31},
    {.oi = 0x4002, .attr = 2, .kind = P_OCTETS, .octet_max = 31},
    {.oi = 0x4003, .attr = 2, .kind = P_OCTETS, .octet_max = 31},
    {.oi = 0x4004, .attr = 2, .kind = P_GEO},
    {.oi = 0x401C, .attr = 2, .kind = P_DLU},
    {.oi = 0x401D, .attr = 2, .kind = P_DLU},
    {.oi = 0x401E, .attr = 2, .kind = P_DLU2},
    {.oi = 0x401F, .attr = 2, .kind = P_DLU3},
    {.oi = 0x4020, .attr = 2, .kind = P_DLU2},
    {.oi = 0x4021, .attr = 2, .kind = P_DLU3},
};

#define PARAM_COUNT (sizeof(g_params) / sizeof(g_params[0]))

static dcu_param_store_t g_store;

static const dcu_param_t *_find(uint16_t oi, uint8_t attr) {
    for (size_t i = 0; i < PARAM_COUNT; i++) {
        if (g_params[i].oi == oi && g_params[i].attr == attr) return &g_params[i];
    }
    return NULL;
}

static dcu_param_t *_find_mut(uint16_t oi, uint8_t attr) {
    for (size_t i = 0; i < PARAM_COUNT; i++) {
        if (g_params[i].oi == oi && g_params[i].attr == attr) return &g_params[i];
    }
    return NULL;
}

static int _default_rc(uint16_t oi, uint8_t attr) {
    if (!dcu_698_param_is_oi(oi)) return EDGE_SLAVE_SRC_NOT_FOUND;
    /* 类 8 定义属性 1/2；属性 1 实例没实现 → 5，属性 3+ 类里没有 → 4。 */
    return attr == 1 ? EDGE_SLAVE_SRC_CLASS_MISMATCH : EDGE_SLAVE_SRC_UNDEFINED;
}

static void _set_defaults(void) {
    /* 只清值，不动 oi/attr/kind/count：这些是描述表，reset 不能把行本身抹掉。 */
    for (size_t i = 0; i < PARAM_COUNT; i++) memset(&g_params[i].v, 0, sizeof(g_params[i].v));

    time_t now = time(NULL);
    struct tm tm_now;
    memset(&tm_now, 0, sizeof(tm_now));
#if defined(_WIN32)
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    dcu_param_t *dt = _find_mut(OI_DATETIME, 2);
    if (dt) {
        dt->v.when.year = (uint16_t)(tm_now.tm_year + 1900);
        dt->v.when.month = (uint8_t)(tm_now.tm_mon + 1);
        dt->v.when.day = (uint8_t)tm_now.tm_mday;
        dt->v.when.hour = (uint8_t)tm_now.tm_hour;
        dt->v.when.min = (uint8_t)tm_now.tm_min;
        dt->v.when.sec = (uint8_t)tm_now.tm_sec;
    }

    dcu_param_t *ct = _find_mut(0x401C, 2);
    if (ct) ct->v.dlu[0] = 1;
    dcu_param_t *pt = _find_mut(0x401D, 2);
    if (pt) pt->v.dlu[0] = 1;
}

void dcu_698_param_reset(void) {
    g_store = (dcu_param_store_t){0};
    _set_defaults();
}

bool dcu_698_param_is_oi(uint16_t oi) {
    switch (oi) {
#define X(v) \
    case v:  \
        return true;
        DCU_698_PARAM_OIS(X)
#undef X
        default:
            return false;
    }
}

bool dcu_698_param_has_attr(uint16_t oi, uint8_t attr) {
    return _find(oi, attr) != NULL;
}

static int _put8(uint8_t *buf, size_t cap, size_t *off, uint8_t v) {
    if (*off >= cap) return -1;
    buf[(*off)++] = v;
    return 0;
}

static int _put_be32(uint8_t *buf, size_t cap, size_t *off, uint32_t v) {
    if (*off + 4 > cap) return -1;
    buf[(*off)++] = (uint8_t)(v >> 24);
    buf[(*off)++] = (uint8_t)(v >> 16);
    buf[(*off)++] = (uint8_t)(v >> 8);
    buf[(*off)++] = (uint8_t)v;
    return 0;
}

static int _put_coord(uint8_t *buf, size_t cap, size_t *off, const dcu_param_coord_t *c) {
    if (_put8(buf, cap, off, DLT698_TAG_STRUCTURE) != 0 || _put8(buf, cap, off, 4) != 0 ||
        _put8(buf, cap, off, DLT698_TAG_ENUM) != 0 || _put8(buf, cap, off, c->dir) != 0 ||
        _put8(buf, cap, off, DLT698_TAG_UNSIGNED) != 0 || _put8(buf, cap, off, c->deg) != 0 ||
        _put8(buf, cap, off, DLT698_TAG_UNSIGNED) != 0 || _put8(buf, cap, off, c->min) != 0 ||
        _put8(buf, cap, off, DLT698_TAG_UNSIGNED) != 0 || _put8(buf, cap, off, c->sec) != 0) {
        return -1;
    }
    return 0;
}

static int _encode_value(const dcu_param_t *p, uint8_t *buf, size_t cap, size_t *len) {
    size_t off = 0;

    switch (p->kind) {
        case P_DATETIME_S: {
            if (cap > UINT16_MAX) cap = UINT16_MAX;
            uint16_t off16 = (uint16_t)off;
            if (dlt698_encode_date_time_s(buf, (uint16_t)cap, &off16, p->v.when.year,
                                          p->v.when.month, p->v.when.day, p->v.when.hour,
                                          p->v.when.min, p->v.when.sec) != 0) {
                return -1;
            }
            off = off16;
            break;
        }
        case P_ENUM:
            if (_put8(buf, cap, &off, DLT698_TAG_ENUM) != 0 ||
                _put8(buf, cap, &off, p->v.enum8) != 0) {
                return -1;
            }
            break;
        case P_UNSIGNED5:
            if (_put8(buf, cap, &off, DLT698_TAG_STRUCTURE) != 0 || _put8(buf, cap, &off, 5) != 0) {
                return -1;
            }
            for (size_t i = 0; i < 5; i++) {
                if (p->v.u16[i] > UINT8_MAX) return -1;
                if (_put8(buf, cap, &off, DLT698_TAG_UNSIGNED) != 0 ||
                    _put8(buf, cap, &off, (uint8_t)p->v.u16[i]) != 0) {
                    return -1;
                }
            }
            break;
        case P_OCTETS:
            if (p->v.octets.len > p->octet_max) return -1;
            if (_put8(buf, cap, &off, DLT698_TAG_OCTET_STRING) != 0 ||
                _put8(buf, cap, &off, p->v.octets.len) != 0 || off + p->v.octets.len > cap) {
                return -1;
            }
            memcpy(buf + off, p->v.octets.bytes, p->v.octets.len);
            off += p->v.octets.len;
            break;
        case P_GEO:
            if (_put8(buf, cap, &off, DLT698_TAG_STRUCTURE) != 0 || _put8(buf, cap, &off, 3) != 0 ||
                _put_coord(buf, cap, &off, &p->v.geo.lon) != 0 ||
                _put_coord(buf, cap, &off, &p->v.geo.lat) != 0 ||
                _put8(buf, cap, &off, DLT698_TAG_DOUBLE_LONG_UNSIGNED) != 0 ||
                _put_be32(buf, cap, &off, p->v.geo.height_cm) != 0) {
                return -1;
            }
            break;
        case P_DLU:
            if (_put8(buf, cap, &off, DLT698_TAG_DOUBLE_LONG_UNSIGNED) != 0 ||
                _put_be32(buf, cap, &off, p->v.dlu[0]) != 0) {
                return -1;
            }
            break;
        case P_DLU2:
        case P_DLU3: {
            uint8_t n = p->kind == P_DLU2 ? 2 : 3;
            if (_put8(buf, cap, &off, DLT698_TAG_STRUCTURE) != 0 || _put8(buf, cap, &off, n) != 0) {
                return -1;
            }
            for (uint8_t i = 0; i < n; i++) {
                if (_put8(buf, cap, &off, DLT698_TAG_DOUBLE_LONG_UNSIGNED) != 0 ||
                    _put_be32(buf, cap, &off, p->v.dlu[i]) != 0) {
                    return -1;
                }
            }
            break;
        }
        default:
            return -1;
    }

    *len = off;
    return 0;
}

static int _decode_coord(const dlt698_data_t *d, dcu_param_coord_t *out, uint8_t max_deg) {
    if (d->tag != DLT698_TAG_STRUCTURE || d->val.container.count != 4 || !d->val.container.items) {
        return -1;
    }
    const dlt698_data_t *f = d->val.container.items;
    if (f[0].tag != DLT698_TAG_ENUM || f[1].tag != DLT698_TAG_UNSIGNED ||
        f[2].tag != DLT698_TAG_UNSIGNED || f[3].tag != DLT698_TAG_UNSIGNED) {
        return -1;
    }
    if (f[0].val.u8 > 1 || f[1].val.u8 > max_deg || f[2].val.u8 > 59 || f[3].val.u8 > 59) {
        return -1;
    }
    out->dir = f[0].val.u8;
    out->deg = f[1].val.u8;
    out->min = f[2].val.u8;
    out->sec = f[3].val.u8;
    return 0;
}

static int _decode_value(dcu_param_t *next, const uint8_t *axdr, size_t len) {
    if (!axdr || len == 0 || len > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    if (next->kind == P_DATETIME_S) {
        uint16_t off = 0;
        uint16_t year = 0;
        uint8_t month = 0, day = 0, hour = 0, min = 0, sec = 0;
        if (dlt698_decode_date_time_s(axdr, (uint16_t)len, &off, &year, &month, &day, &hour, &min,
                                      &sec) != 0 ||
            (size_t)off != len) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        next->v.when.year = year;
        next->v.when.month = month;
        next->v.when.day = day;
        next->v.when.hour = hour;
        next->v.when.min = min;
        next->v.when.sec = sec;
        return 0;
    }

    uint16_t off = 0;
    dlt698_data_t d;
    memset(&d, 0, sizeof(d));
    if (dlt698_decode_data(axdr, (uint16_t)len, &off, &d) != 0 || (size_t)off != len) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    int rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
    switch (next->kind) {
        case P_ENUM:
            if (d.tag == DLT698_TAG_ENUM &&
                (d.val.u8 == 0 || d.val.u8 == 1 || d.val.u8 == 2 || d.val.u8 == 0xFF)) {
                next->v.enum8 = d.val.u8;
                rc = 0;
            }
            break;
        case P_UNSIGNED5:
            if (d.tag == DLT698_TAG_STRUCTURE && d.val.container.count == 5 &&
                d.val.container.items) {
                rc = 0;
                for (size_t i = 0; i < 5; i++) {
                    const dlt698_data_t *f = &d.val.container.items[i];
                    if (f->tag != DLT698_TAG_UNSIGNED) {
                        rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
                        break;
                    }
                    next->v.u16[i] = f->val.u8;
                }
            }
            break;
        case P_OCTETS:
            if (d.tag == DLT698_TAG_OCTET_STRING && d.val.str.len <= next->octet_max && d.val.str.ptr) {
                next->v.octets.len = (uint8_t)d.val.str.len;
                memcpy(next->v.octets.bytes, d.val.str.ptr, d.val.str.len);
                rc = 0;
            }
            break;
        case P_GEO:
            if (d.tag == DLT698_TAG_STRUCTURE && d.val.container.count == 3 &&
                d.val.container.items) {
                const dlt698_data_t *f = d.val.container.items;
                dcu_param_geo_t geo;
                memset(&geo, 0, sizeof(geo));
                if (_decode_coord(&f[0], &geo.lon, 180) == 0 &&
                    _decode_coord(&f[1], &geo.lat, 90) == 0 &&
                    f[2].tag == DLT698_TAG_DOUBLE_LONG_UNSIGNED) {
                    geo.height_cm = f[2].val.u32;
                    next->v.geo = geo;
                    rc = 0;
                }
            }
            break;
        case P_DLU:
            if (d.tag == DLT698_TAG_DOUBLE_LONG_UNSIGNED) {
                next->v.dlu[0] = d.val.u32;
                rc = 0;
            }
            break;
        case P_DLU2:
        case P_DLU3: {
            uint8_t n = next->kind == P_DLU2 ? 2 : 3;
            if (d.tag == DLT698_TAG_STRUCTURE && d.val.container.count == n &&
                d.val.container.items) {
                rc = 0;
                for (uint8_t i = 0; i < n; i++) {
                    const dlt698_data_t *f = &d.val.container.items[i];
                    if (f->tag != DLT698_TAG_DOUBLE_LONG_UNSIGNED) {
                        rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
                        break;
                    }
                    next->v.dlu[i] = f->val.u32;
                }
            }
            break;
        }
        default:
            break;
    }
    return rc;
}

void dcu_698_param_init(const dcu_param_store_t *store) {
    _set_defaults();
    g_store = store ? *store : (dcu_param_store_t){0};
    if (!g_store.load) return;

    for (size_t i = 0; i < PARAM_COUNT; i++) {
        if (g_params[i].oi == OI_DATETIME) continue;
        uint8_t blob[P_BLOB_MAX];
        size_t n = 0;
        if (g_store.load(g_store.ctx, g_params[i].oi, blob, sizeof(blob), &n) != 0 || n == 0) {
            continue;
        }
        dcu_param_t next = g_params[i];
        if (_decode_value(&next, blob, n) != 0) {
            LOG_WARN(LOG_CAT_NB, "DCU 698 参数 OI=0x%04X 落盘值无法解码，保留默认值",
                     g_params[i].oi);
            continue;
        }
        g_params[i] = next;
    }
}

int dcu_698_param_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr = (uint8_t)((oad >> 8) & 0xFF);
    const dcu_param_t *p = _find(oi, attr);
    if (!p) return _default_rc(oi, attr);

    if (_encode_value(p, buf, cap, len) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    return 0;
}

int dcu_698_param_accept_axdr(uint32_t oad, const uint8_t *axdr, size_t len) {
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr = (uint8_t)((oad >> 8) & 0xFF);

    int rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
    dcu_param_t *row = _find_mut(oi, attr);
    if (!row) {
        rc = _default_rc(oi, attr);
    } else {
        dcu_param_t next = *row;
        rc = _decode_value(&next, axdr, len);
        if (rc == 0 && oi != OI_DATETIME) {
            uint8_t blob[P_BLOB_MAX];
            size_t n = 0;
            if (_encode_value(&next, blob, sizeof(blob), &n) != 0) {
                rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
            } else if (!g_store.save) {
                rc = EDGE_SLAVE_SRC_TEMPORARY;
            } else if (g_store.save(g_store.ctx, oi, blob, n) != 0) {
                rc = EDGE_SLAVE_SRC_TEMPORARY;
            }
        }
        if (rc == 0) *row = next;
    }

    if (rc != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 698 SET OI=0x%04X attr=%u 被拒绝 rc=%d", oi, (unsigned)attr, rc);
    }
    return rc;
}
