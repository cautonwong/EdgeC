/**
 * @file driver_dlt698.c
 * @brief 面向对象规约 DLT698.45 驱动 —— 纯 codec（ADR-002 S3-c）
 *
 * 与 645 同族，点位标识天然是 **OAD**（不需要像 Modbus 那样编码地址+长度），
 * 所以产品适配层不用改：`oad_to_proto("DLT698", oad)` 返回的就是 OAD。
 *
 * 这个文件里没有 lookup_by_tag / lookup_by_key / read_shadow / tag_id：
 *   - 主站：只认 oad，只回原始值；标识与量纲由调用方给
 *   - 从站：解 GET 请求 → src->resolve(oad) → 组应答，驱动不认识点位模型
 */

#include "driver_dlt698.h"

#include <stdint.h>
#include <string.h>

#include "dlt645_codec.h"
#include "dlt698_codec.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "edge_slave_plane.h"
#include "net_buf.h"

#define DLT698_ADDR_LEN 6

/* 上报未确认的有界解锁时间：确认丢了不能把后续上报永远卡死（ADR-0017）。 */
#define DLT698_REPORT_TIMEOUT_MS 5000u

/* A-XDR 编码缓冲：单线程事件循环，一问一答串行复用；不进栈、不引入动态内存。
 * 同一线程并行处理多路 698 从站时，需改为调用方/池缓冲（ADR-0013 §3）。 */
static _Thread_local uint8_t s_axdr_buf[EDGE_SLAVE_AXDR_MAX];

/* 逐字节提升到 uint32_t 再移位：0xF2 高字节左移 24 位在 int 里是 UB。 */
static uint32_t _read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/** 请求私有数据：驱动认识的全部信息 */
typedef struct {
    uint32_t oad; /**< 对象标识，698 原生点位 */
    uint8_t piid; /**< 服务序号 */
    uint8_t sa_addr[DLT698_ADDR_LEN];
    uint8_t sa_len;
    int32_t raw;        /**< 解码出的原始值 */
    bool is_write;      /**< 本次是 SET 还是 GET */
    uint8_t write_type; /**< A-XDR 类型标签 */
    double write_value; /**< 写入值（raw） */
} dlt698_req_t;

/* PIID 是协议层序号，不依赖任何全局服务 */
static uint8_t s_piid;

static void _fill_sa(dlt698_req_t *p, uint32_t slave_id) {
    memset(p->sa_addr, 0, sizeof(p->sa_addr));
    dlt645_u64_to_bcd_addr(slave_id, p->sa_addr);
    p->sa_len = DLT698_ADDR_LEN;
}

/* ==================== 主站侧 ==================== */

static int dlt698_req_prepare(void *priv, uint32_t slave_id, uint32_t point_id,
                              const void *meta_list, uint32_t meta_list_len, const void *point_desc,
                              size_t point_desc_len, bool is_write, double value) {
    (void)meta_list;
    (void)meta_list_len;
    dlt698_req_t *p = (dlt698_req_t *)priv;
    if (!p) return -1;

    if (is_write) {
        /* 类型由产品给：同一个 OAD 在不同装置上可能是 long-unsigned 也可能是
         * double-long，驱动猜不了。缺描述或类型不支持就拒绝。 */
        if (!point_desc || point_desc_len != sizeof(dlt698_write_desc_t)) {
            LOG_WARN(LOG_CAT_IO, "DLT698: 写请求缺少 A-XDR 类型描述");
            return -1;
        }
        uint8_t type = ((const dlt698_write_desc_t *)point_desc)->type;
        /* 值域先校验：截断写入比失败危险得多 */
        if (type == DLT698_WRITE_LONG_UNSIGNED) {
            if (value < 0.0 || value > 65535.0) {
                LOG_WARN(LOG_CAT_IO, "DLT698: long-unsigned 越界 %.0f", value);
                return -1;
            }
        } else if (type == DLT698_WRITE_DOUBLE_LONG) {
            if (value < -2147483648.0 || value > 2147483647.0) {
                LOG_WARN(LOG_CAT_IO, "DLT698: double-long 越界 %.0f", value);
                return -1;
            }
        } else {
            LOG_WARN(LOG_CAT_IO, "DLT698: 不支持的 A-XDR 类型 0x%02X", type);
            return -1;
        }
        p->is_write = true;
        p->write_type = type;
        p->write_value = value;
    } else {
        p->is_write = false;
        p->write_type = 0;
        p->write_value = 0.0;
    }

    p->oad = point_id;
    _fill_sa(p, slave_id);
    p->piid = ++s_piid;
    p->raw = 0;
    return 0;
}

static int dlt698_req_encode(void *priv, void *session, net_buf_t *out) {
    (void)session; /* 无会话状态：这一帧自成一体 */
    dlt698_req_t *p = (dlt698_req_t *)priv;
    if (!p || !out) return -1;

    dlt698_addr_t sa = {.len = p->sa_len, .type = 0};
    memcpy(sa.addr, p->sa_addr, p->sa_len);

    if (!p->is_write) return dlt698_encode_get_request(&sa, p->piid, p->oad, out);

    /* SET：A-XDR 数据段用 codec 里已有的编码器，不在这里手搓字节序 */
    uint8_t axdr[8];
    uint16_t off = 0;
    int rc;
    if (p->write_type == DLT698_WRITE_LONG_UNSIGNED) {
        rc = dlt698_encode_long_unsigned(axdr, sizeof(axdr), &off, (uint16_t)p->write_value);
    } else if (p->write_type == DLT698_WRITE_DOUBLE_LONG) {
        rc = dlt698_encode_double_long(axdr, sizeof(axdr), &off, (int32_t)p->write_value);
    } else {
        return -1;
    }
    if (rc != 0 || off == 0) return -1;

    return dlt698_encode_set_request(&sa, p->piid, p->oad, axdr, off, out);
}

static int dlt698_req_decode(void *priv, void *session, const uint8_t *buf, size_t len,
                             size_t *consumed, double *out_value) {
    (void)session; /* 无会话状态：这一帧自成一体 */
    dlt698_req_t *p = (dlt698_req_t *)priv;
    if (!p || !buf || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)buf, len);
    nb.len = (uint16_t)len; /* init_with_data 只设 size，len 留给写入用 */

    dlt698_addr_t sa_rx;
    uint8_t svc = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t payload_len = 0;

    int ret = dlt698_decode_frame(&nb, &sa_rx, &svc, &piid, &payload, &payload_len);
    if (ret == 1) return EDGE_ERR_INCOMPLETE;
    if (ret < 0) {
        *consumed = 1;
        return EDGE_ERR_INCOMPLETE;
    }
    *consumed = (size_t)(payload + payload_len + 3 - buf);

    if (svc != DLT698_SVC_GET_RES || payload_len < 5) return -1;

    uint32_t oad_rx = _read_be32(payload);
    /* 不是本请求的对象：不认，让调用方继续找 */
    if (oad_rx != p->oad) return EDGE_ERR_INCOMPLETE;

    uint16_t offset = 4;
    dlt698_data_t res;
    if (dlt698_decode_data(payload, (uint16_t)payload_len, &offset, &res) != 0) return -1;

    double val = 0.0;
    switch (res.tag) {
        case DLT698_TAG_UNSIGNED:
            val = (double)res.val.u8;
            break;
        case DLT698_TAG_LONG_UNSIGNED:
            val = (double)res.val.u16;
            break;
        case DLT698_TAG_DOUBLE_LONG_UNSIGNED:
            val = (double)res.val.u32;
            break;
        case DLT698_TAG_INTEGER:
            val = (double)res.val.i8;
            break;
        case DLT698_TAG_LONG:
            val = (double)res.val.i16;
            break;
        case DLT698_TAG_DOUBLE_LONG:
            val = (double)res.val.i32;
            break;
        default:
            dlt698_free_data(&res);
            return -1;
    }
    dlt698_free_data(&res);

    /* 只回原始值；量纲由调用方按点位表给 */
    p->raw = (int32_t)val;
    if (out_value) *out_value = val;
    return 0;
}

/* ==================== 从站侧 ==================== */

/**
 * @brief 把一个点位值编成 A-XDR Data（含类型标签），写进调用方给的 8 字节缓冲。
 *
 * 类型标签由产品给（v.type）；没给才按宽度兜底 —— `width=2` 到底是 long 还是
 * long-unsigned 只有点位模型知道，猜错主站解出的值就是错的且不报错。
 * 没有对应编码器的类型**不发**：发一个"看起来对"的字节序比明确失败更糟。
 */
static int _encode_axdr_value(const edge_point_value_t *v, uint8_t buf[8], size_t *dlen) {
    uint8_t tag = v->type ? v->type
                          : (v->width >= 4 ? (uint8_t)DLT698_TAG_DOUBLE_LONG
                                           : (uint8_t)DLT698_TAG_LONG_UNSIGNED);
    if (tag == DLT698_TAG_LONG_UNSIGNED) {
        buf[0] = tag;
        buf[1] = (uint8_t)(((uint16_t)v->raw) >> 8);
        buf[2] = (uint8_t)(((uint16_t)v->raw) & 0xFF);
        *dlen = 3;
    } else if (tag == DLT698_TAG_DOUBLE_LONG) {
        uint32_t bits = (uint32_t)v->raw;
        buf[0] = tag;
        buf[1] = (uint8_t)(bits >> 24);
        buf[2] = (uint8_t)(bits >> 16);
        buf[3] = (uint8_t)(bits >> 8);
        buf[4] = (uint8_t)(bits & 0xFF);
        *dlen = 5;
    } else if (tag == DLT698_TAG_FLOAT32) {
        float f = (float)v->value;
        uint32_t bits = 0;
        memcpy(&bits, &f, sizeof(bits));
        buf[0] = tag;
        buf[1] = (uint8_t)(bits >> 24);
        buf[2] = (uint8_t)(bits >> 16);
        buf[3] = (uint8_t)(bits >> 8);
        buf[4] = (uint8_t)(bits & 0xFF);
        *dlen = 5;
    } else {
        return -1;
    }
    return 0;
}

/* 产品取数回调的失败原因 → DL/T 698.45 6.3.3.11 的 DAR。 */
static uint8_t _dar_for_source_rc(int rc) {
    switch (rc) {
        case EDGE_SLAVE_SRC_CLASS_MISMATCH:
            return DLT698_DAR_OBJECT_CLASS_MISMATCH;
        case EDGE_SLAVE_SRC_TYPE_MISMATCH:
            return DLT698_DAR_TYPE_MISMATCH;
        case EDGE_SLAVE_SRC_UNDEFINED:
            return DLT698_DAR_OBJECT_UNDEFINED;
        case EDGE_SLAVE_SRC_TEMPORARY:
            return DLT698_DAR_TEMPORARY_FAILURE;
        case EDGE_SLAVE_SRC_DENIED:
            return DLT698_DAR_DENIED;
        case EDGE_SLAVE_SRC_NOT_FOUND:
        default:
            return DLT698_DAR_OBJECT_NOT_FOUND;
    }
}

static uint8_t _dar_for_action_rc(int rc) {
    if (rc == 0) return DLT698_DAR_SUCCESS;
    /* ACTION 的通用失败用 DAR 2；端口给出的具体原因仍按统一映射。 */
    if (rc == EDGE_SLAVE_SRC_TEMPORARY || rc == -1) return DLT698_DAR_TEMPORARY_FAILURE;
    return _dar_for_source_rc(rc);
}

/* ---- Class 20 应用连接：会话状态与对象列表 ACL（ADR-0015） ---- */

#define DLT698_OI_CONN 0x4400u
#define DLT698_OI_CONN_OBJECT_LIST 0x44000200u
#define DLT698_ACL_MAX_OBJECTS 28
#define DLT698_ACL_MAX_ATTRS 16
#define DLT698_ACL_MAX_METHODS 8
#define DLT698_ATTR_RIGHT_READ 1u
#define DLT698_ATTR_RIGHT_WRITE 2u
#define DLT698_ATTR_RIGHT_RW 3u

/* 28 个对象 × 16 B = 448 B；旧的内联数组（20 × 44 B）在 28 个对象时会超
 * DRIVER_PRIV_SIZE(1024)。属性权限用每位 2 bit 压进 32 位，方法只存已允许的 id。 */
typedef struct {
    uint16_t oi;
    uint8_t n_attrs;
    uint8_t n_methods;
    uint32_t attr_rights; /* 属性 id 1..16，各 2 bit */
    uint8_t method_ids[DLT698_ACL_MAX_METHODS];
} dlt698_acl_object_t;

typedef struct {
    bool loaded;
    uint8_t count;
    dlt698_acl_object_t objects[DLT698_ACL_MAX_OBJECTS];
} dlt698_acl_t;

typedef struct {
    bool connected;
    uint8_t client_addr;
    /* CONNECT 时主站用的 SA：主动上报没有入站帧可抄地址，必须记下来（ADR-0017）。 */
    uint8_t sa_addr[16];
    uint8_t sa_len;
    bool report_pending;
    uint8_t report_piid;
    uint64_t report_sent_ms;
    dlt698_acl_t acl;
} dlt698_session_t;

_Static_assert(sizeof(dlt698_session_t) <= 1024, "DLT698 session exceeds DRIVER_PRIV_SIZE");

/* 对象列表结构（7.3.20 表182）：array { structure { OI, structure { attr[], method[] } } }，
 * 字段是 A-XDR Data；越界即不可用。 */
static int _acl_take(const uint8_t *buf, size_t len, size_t *off, uint8_t tag) {
    if (*off >= len || buf[*off] != tag) return -1;
    (*off)++;
    return 0;
}

static bool _acl_parse(const uint8_t *buf, size_t len, dlt698_acl_t *acl) {
    size_t off = 0;
    if (_acl_take(buf, len, &off, 0x01) != 0 || off >= len) return false;
    uint8_t n = buf[off++];
    if (n > DLT698_ACL_MAX_OBJECTS) return false;

    for (uint8_t i = 0; i < n; i++) {
        dlt698_acl_object_t *obj = &acl->objects[i];
        if (_acl_take(buf, len, &off, 0x02) != 0 || off >= len || buf[off++] != 2) return false;
        if (_acl_take(buf, len, &off, 0x50) != 0 || len - off < 2) return false;
        obj->oi = (uint16_t)((buf[off] << 8) | buf[off + 1]);
        off += 2;

        if (_acl_take(buf, len, &off, 0x02) != 0 || off >= len || buf[off++] != 2) return false;
        if (_acl_take(buf, len, &off, 0x01) != 0 || off >= len) return false;
        uint8_t na = buf[off++];
        if (na > DLT698_ACL_MAX_ATTRS) return false;
        obj->n_attrs = na;
        obj->attr_rights = 0;
        for (uint8_t a = 0; a < na; a++) {
            if (_acl_take(buf, len, &off, 0x02) != 0 || off >= len || buf[off++] != 2) return false;
            if (_acl_take(buf, len, &off, 0x11) != 0 || off >= len) return false;
            uint8_t attr_id = buf[off++];
            if (attr_id == 0 || attr_id > DLT698_ACL_MAX_ATTRS) return false;
            if (_acl_take(buf, len, &off, 0x16) != 0 || off >= len) return false;
            uint32_t rights = (uint32_t)(buf[off++] & 3u);
            obj->attr_rights |= rights << ((attr_id - 1u) * 2u);
        }

        if (_acl_take(buf, len, &off, 0x01) != 0 || off >= len) return false;
        uint8_t nm = buf[off++];
        if (nm > DLT698_ACL_MAX_METHODS) return false;
        obj->n_methods = 0;
        for (uint8_t m = 0; m < nm; m++) {
            if (_acl_take(buf, len, &off, 0x02) != 0 || off >= len || buf[off++] != 2) return false;
            if (_acl_take(buf, len, &off, 0x11) != 0 || off >= len) return false;
            uint8_t method_id = buf[off++];
            if (_acl_take(buf, len, &off, 0x03) != 0 || off >= len) return false;
            if (buf[off++] != 0) {
                obj->method_ids[obj->n_methods++] = method_id;
            }
        }
    }
    if (off != len) return false;
    acl->count = n;
    acl->loaded = true;
    return true;
}

/* 取产品定义的对象列表；失败即"不可用"，绝不回退成放行。 */
static bool _acl_load(dlt698_session_t *s, const edge_slave_source_ops_t *src, void *src_ctx) {
    if (!s) return false;
    s->acl.loaded = false;
    s->acl.count = 0;
    if (!src || !src->object_list) return false;
    size_t len = 0;
    if (src->object_list(src_ctx, DLT698_OI_CONN_OBJECT_LIST, s_axdr_buf, sizeof(s_axdr_buf),
                         &len) != 0 ||
        len == 0 || len > sizeof(s_axdr_buf)) {
        return false;
    }
    return _acl_parse(s_axdr_buf, len, &s->acl);
}

static const dlt698_acl_object_t *_acl_find(const dlt698_session_t *s, uint16_t oi) {
    if (!s || !s->acl.loaded) return NULL;
    for (uint8_t i = 0; i < s->acl.count; i++) {
        if (s->acl.objects[i].oi == oi) return &s->acl.objects[i];
    }
    return NULL;
}

static bool _acl_attr_allowed(const dlt698_acl_object_t *obj, uint8_t attr_id, bool write) {
    if (!obj || attr_id == 0 || attr_id > DLT698_ACL_MAX_ATTRS) return false;
    uint8_t r = (uint8_t)((obj->attr_rights >> ((attr_id - 1u) * 2u)) & 3u);
    return write ? (r == DLT698_ATTR_RIGHT_WRITE || r == DLT698_ATTR_RIGHT_RW)
                 : (r == DLT698_ATTR_RIGHT_READ || r == DLT698_ATTR_RIGHT_RW);
}

static bool _acl_method_allowed(const dlt698_acl_object_t *obj, uint8_t method_id) {
    if (!obj) return false;
    for (uint8_t i = 0; i < obj->n_methods; i++) {
        if (obj->method_ids[i] == method_id) return true;
    }
    return false;
}

/* 数据请求的授权入口：一次 OAD 的读写，或一次 OMD 的方法。 */
static bool _acl_check(dlt698_session_t *s, const edge_slave_source_ops_t *src, void *src_ctx,
                       uint32_t oad_or_omd, bool write, bool method) {
    if (!s) return false;
    uint16_t oi = (uint16_t)(oad_or_omd >> 16);
    uint8_t id = (uint8_t)((oad_or_omd >> 8) & 0xFF);
    /* 预连接窗口：允许读类 20 自己的对象列表，否则主站无从得知窗口。 */
    if (!method && !write && oi == DLT698_OI_CONN && id == 2) return true;

    if (!s->acl.loaded && !_acl_load(s, src, src_ctx)) return false;
    const dlt698_acl_object_t *obj = _acl_find(s, oi);
    if (!obj) return false;
    return method ? _acl_method_allowed(obj, id) : _acl_attr_allowed(obj, id, write);
}

static int _handle_connect(const dlt698_addr_t *sa, uint8_t piid, const uint8_t *body,
                           size_t body_len, const edge_slave_source_ops_t *src, void *src_ctx,
                           void *session, net_buf_t *out) {
    /* CONNECT-Request（表59 / 附录 H.2）：version(2) conf(8) func(16) send(2) recv(2)
     * window(1) apdu(2) timeout(4) mechanism(1) [时间标签]。 */
    dlt698_session_t *s = (dlt698_session_t *)session;
    if (!sa || !out) return -1;
    uint8_t result = 255; /* 其他错误 */
    uint16_t version = 0;
    uint32_t timeout = 0;
    const uint8_t *conf = NULL;
    const uint8_t *func = NULL;

    if (body && body_len >= 2 + 8 + 16 + 2 + 2 + 1 + 2 + 4 + 1) {
        size_t off = 0;
        version = (uint16_t)((body[off] << 8) | body[off + 1]);
        off += 2;
        conf = body + off;
        off += 8;
        func = body + off;
        off += 16;
        off += 2 + 2; /* 客户机收发帧最大尺寸 */
        uint8_t window = body[off++];
        uint16_t apdu_max = (uint16_t)((body[off] << 8) | body[off + 1]);
        off += 2;
        timeout = ((uint32_t)body[off] << 24) | ((uint32_t)body[off + 1] << 16) |
                  ((uint32_t)body[off + 2] << 8) | body[off + 3];
        off += 4;
        uint8_t mechanism = body[off++];
        size_t used = off;

        bool params_ok = window > 0 && apdu_max > 0 &&
                         (body_len == used || (body_len == used + 1 && body[used] == 0));
        if (!params_ok) {
            result = 255;
        } else if (version != 0x0010) {
            result = 5; /* 协议版本不匹配（表43） */
        } else if (mechanism != 0) {
            result = 255; /* 仅支持公共连接 [0] */
        } else if (!s || !_acl_load(s, src, src_ctx)) {
            result = 255;
        } else {
            s->connected = true;
            s->client_addr = sa->len ? sa->addr[sa->len - 1] : 0;
            s->sa_len = sa->len <= sizeof(s->sa_addr) ? sa->len : (uint8_t)sizeof(s->sa_addr);
            memcpy(s->sa_addr, sa->addr, s->sa_len);
            result = 0;
        }
    }

    dlt698_connect_params_t params = {
        .version = version,
        .conf = conf,
        .func = func,
        .timeout = timeout,
        .result = result,
    };
    return dlt698_encode_connect_response(sa, piid, &params, out);
}

static int _handle_release(const dlt698_addr_t *sa, uint8_t piid, void *session, net_buf_t *out) {
    dlt698_session_t *s = (dlt698_session_t *)session;
    if (s) memset(s, 0, sizeof(*s));
    if (!sa || !out) return -1;
    return dlt698_encode_release_response(sa, piid, out);
}

/* 会话建立/重建时清掉跨帧状态。`session_manager_reset_sessions` 在新连接接管时
 * 调它：旧的 connected / 待确认上报 / ACL 都不能带进新会话（ADR-0017）。 */
static int dlt698_on_init(edge_driver_ctx_t *ctx) {
    if (ctx && ctx->priv) memset(ctx->priv, 0, sizeof(dlt698_session_t));
    return 0;
}

static int _handle_set_request(const dlt698_addr_t *sa, uint8_t piid, const uint8_t *payload,
                               size_t payload_len, const edge_slave_source_ops_t *src,
                               void *src_ctx, void *session, net_buf_t *out) {
    /* SET-Request-Normal：payload = OAD(4) | Data(A-XDR) | 时间标签。 */
    if (!sa || !payload || !out || payload_len < 4) return -1;
    uint32_t oad =
        _read_be32(payload);

    if (!_acl_check((dlt698_session_t *)session, src, src_ctx, oad, true, false)) {
        return dlt698_encode_set_response(sa, piid, oad, DLT698_DAR_DENIED, out);
    }

    int rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (payload_len > 4) {
        dlt698_data_t d;
        memset(&d, 0, sizeof(d));
        uint16_t off = 0;
        if (dlt698_decode_data(payload + 4, (uint16_t)(payload_len - 4), &off, &d) == 0) {
            double value = 0.0;
            bool scalar = false;
            switch (d.tag) {
                case DLT698_TAG_INTEGER:
                    value = (double)d.val.i8;
                    scalar = true;
                    break;
                case DLT698_TAG_UNSIGNED:
                    value = (double)d.val.u8;
                    scalar = true;
                    break;
                case DLT698_TAG_LONG:
                    value = (double)d.val.i16;
                    scalar = true;
                    break;
                case DLT698_TAG_LONG_UNSIGNED:
                    value = (double)d.val.u16;
                    scalar = true;
                    break;
                case DLT698_TAG_ENUM:
                    value = (double)d.val.u8;
                    scalar = true;
                    break;
                case DLT698_TAG_BOOLEAN:
                    value = (double)d.val.u8;
                    scalar = true;
                    break;
                case DLT698_TAG_DOUBLE_LONG:
                    value = (double)d.val.i32;
                    scalar = true;
                    break;
                case DLT698_TAG_DOUBLE_LONG_UNSIGNED:
                    value = (double)d.val.u32;
                    scalar = true;
                    break;
                case DLT698_TAG_LONG64:
                    value = (double)d.val.i64;
                    scalar = true;
                    break;
                case DLT698_TAG_LONG64_UNSIGNED:
                    value = (double)d.val.u64;
                    scalar = true;
                    break;
                case DLT698_TAG_FLOAT32:
                    value = (double)d.val.f32;
                    scalar = true;
                    break;
                case DLT698_TAG_FLOAT64:
                    value = d.val.f64;
                    scalar = true;
                    break;
                default:
                    break;
            }
            dlt698_free_data(&d);

            if (src && src->accept_axdr) {
                rc = src->accept_axdr(src_ctx, oad, payload + 4, off);
            } else if (src && src->accept && scalar) {
                rc = src->accept(src_ctx, oad, value);
            } else if (src && src->accept) {
                rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
            } else {
                rc = EDGE_SLAVE_SRC_NOT_FOUND;
            }
        }
    }

    uint8_t dar = (rc == 0) ? DLT698_DAR_SUCCESS : _dar_for_source_rc(rc);
    dlt698_encode_set_response(sa, piid, oad, dar, out);
    return 0;
}

/* GetRequestRecord：表74 只有整条结果的 DAR，没有行级 DAR；任何一行失败都丢弃
 * 整页，按产品返回码回 CHOICE [0] DAR（ADR-0014）。 */
static int _handle_get_record(const dlt698_addr_t *sa, uint8_t piid, const uint8_t *payload,
                              size_t payload_len, const edge_slave_source_ops_t *src,
                              void *src_ctx, void *session, net_buf_t *out) {
    uint32_t oad = 0;
    dlt698_record_sel_t sel;
    dlt698_rcsd_t rcsd;
    size_t used = 0;
    int rc = dlt698_decode_get_record_request(payload, payload_len, &oad, &sel, &rcsd, &used);
    if (rc == DLT698_RECORD_UNSUPPORTED_SELECTOR || rc == DLT698_RECORD_UNSUPPORTED_DATA) {
        return dlt698_encode_get_record_response_dar(sa, piid, oad, rcsd.items, rcsd.count,
                                                     DLT698_DAR_TEMPORARY_FAILURE, out);
    }
    if (rc != 0) return -1; /* 报文截断/结构错：不应答 */
    if (!_acl_check((dlt698_session_t *)session, src, src_ctx, oad, false, false)) {
        return dlt698_encode_get_record_response_dar(sa, piid, oad, rcsd.items, rcsd.count,
                                                     DLT698_DAR_DENIED, out);
    }
    if (!src || !src->record.begin || !src->record.row || !src->record.end) {
        return dlt698_encode_get_record_response_dar(sa, piid, oad, rcsd.items, rcsd.count,
                                                     DLT698_DAR_TEMPORARY_FAILURE, out);
    }

    size_t total = 0;
    rc = src->record.begin(src_ctx, oad, payload + 4, used - 4, &total);
    if (rc != 0) {
        return dlt698_encode_get_record_response_dar(sa, piid, oad, rcsd.items, rcsd.count,
                                                     _dar_for_source_rc(rc), out);
    }
    if (total > 0xFF) {
        /* 单帧条数是一字节；没有 GetResponseNext，不能截断成假完整。 */
        src->record.end(src_ctx, oad);
        return dlt698_encode_get_record_response_dar(sa, piid, oad, rcsd.items, rcsd.count,
                                                     DLT698_DAR_TEMPORARY_FAILURE, out);
    }

    dlt698_record_stream_t stream;
    if (dlt698_encode_get_record_response_begin(&stream, sa, piid, oad, rcsd.items, rcsd.count,
                                                total, out) != 0) {
        src->record.end(src_ctx, oad);
        return -1;
    }
    for (size_t i = 0; i < total; i++) {
        size_t row_len = 0;
        rc = src->record.row(src_ctx, oad, i, s_axdr_buf, sizeof(s_axdr_buf), &row_len);
        if (rc != 0 || row_len == 0) {
            src->record.end(src_ctx, oad);
            out->b.len = (uint16_t)stream.start_len;
            uint8_t dar = (rc != 0) ? _dar_for_source_rc(rc) : DLT698_DAR_TYPE_MISMATCH;
            return dlt698_encode_get_record_response_dar(sa, piid, oad, rcsd.items, rcsd.count,
                                                         dar, out);
        }
        if (dlt698_encode_get_record_row(out, s_axdr_buf, row_len) != 0) {
            src->record.end(src_ctx, oad);
            return -1;
        }
    }
    src->record.end(src_ctx, oad);
    return dlt698_encode_get_record_response_end(&stream, out);
}

static int _handle_get_request(const dlt698_addr_t *sa, uint8_t piid, uint8_t choice,
                               const uint8_t *payload, size_t payload_len,
                               const edge_slave_source_ops_t *src, void *src_ctx, void *session,
                               net_buf_t *out) {
    /* [2] GetRequestNormalList：payload = count(1) | OAD(4)×n | 时间标签(1)。
     * 标准 6.3.8.1.2 表66。逐项都要给结果：读不到的回 DAR，不能少返一项 ——
     * 主站请求 2 个 OAD 只拿回 1 个读数还判通过，是最典型的假绿。 */
    if (choice == DLT698_CHOICE_GET_NORMAL_LIST) {
        if (payload_len < 2) return -1;
        size_t n = payload[0];
        if (n > DLT698_GET_LIST_MAX) return -1;
        /* 长度必须精确匹配：多一字节少一字节都不能"凑着解"，
         * 错位解出来的值是错的，而帧校验完全正常。 */
        if (payload_len != 1 + 4 * n + 1) return -1;
        dlt698_list_stream_t stream;
        if (dlt698_encode_get_response_list_begin(&stream, sa, piid, n, out) != 0) return -1;
        for (size_t k = 0; k < n; k++) {
            const uint8_t *oad_bytes = payload + 1 + 4 * k;
            uint32_t oad = _read_be32(oad_bytes);
            if (!_acl_check((dlt698_session_t *)session, src, src_ctx, oad, false, false)) {
                if (dlt698_encode_get_response_list_item_dar(out, oad, DLT698_DAR_DENIED) != 0)
                    return -1;
                continue;
            }
            size_t dlen = 0;
            if (src && src->resolve_axdr) {
                /* A-XDR 通道：完整值（含类型标签）。实现后不再回落标量；
                 * 失败按产品给的失败原因映射 DAR，不截断。单缓冲逐项复用。 */
                int rc = src->resolve_axdr(src_ctx, oad, s_axdr_buf, sizeof(s_axdr_buf), &dlen);
                if (rc != 0 || dlen == 0 || dlen > sizeof(s_axdr_buf)) {
                    uint8_t dar = (rc != 0) ? _dar_for_source_rc(rc) : DLT698_DAR_TYPE_MISMATCH;
                    if (dlt698_encode_get_response_list_item_dar(out, oad, dar) != 0) return -1;
                    continue;
                }
                if (dlt698_encode_get_response_list_item(out, oad, s_axdr_buf, dlen) != 0) {
                    return -1;
                }
                continue;
            }
            edge_point_value_t v;
            memset(&v, 0, sizeof(v));
            int rc = (!src || !src->resolve) ? EDGE_SLAVE_SRC_NOT_FOUND
                                             : src->resolve(src_ctx, oad, NULL, 0, &v);
            if (rc != 0) {
                if (dlt698_encode_get_response_list_item_dar(out, oad,
                                                             _dar_for_source_rc(rc)) != 0)
                    return -1;
                continue;
            }
            uint8_t scalar[8];
            if (_encode_axdr_value(&v, scalar, &dlen) != 0) {
                if (dlt698_encode_get_response_list_item_dar(out, oad,
                                                             DLT698_DAR_TYPE_MISMATCH) != 0)
                    return -1;
                continue;
            }
            if (dlt698_encode_get_response_list_item(out, oad, scalar, dlen) != 0) return -1;
        }
        return dlt698_encode_get_response_list_end(&stream, out);
    }
    /* [3] GetRequestRecord：payload = OAD(4) | RSD | RCSD | 时间标签（表67）。 */
    if (choice == DLT698_CHOICE_GET_RECORD) {
        return _handle_get_record(sa, piid, payload, payload_len, src, src_ctx, session, out);
    }
    /* [1] GetRequestNormal：payload = OAD(4) | 时间标签(1) */
    if (choice != DLT698_CHOICE_GET_NORMAL) return -1;
    if (payload_len < 4) return -1;
    uint32_t oad = _read_be32(payload);
    if (!_acl_check((dlt698_session_t *)session, src, src_ctx, oad, false, false)) {
        dlt698_encode_get_response_dar(sa, piid, oad, DLT698_DAR_DENIED, out);
        return 0;
    }
    if (src && src->resolve_axdr) {
        size_t dlen = 0;
        int rc = src->resolve_axdr(src_ctx, oad, s_axdr_buf, sizeof(s_axdr_buf), &dlen);
        if (rc != 0 || dlen == 0 || dlen > sizeof(s_axdr_buf)) {
            uint8_t dar = (rc != 0) ? _dar_for_source_rc(rc) : DLT698_DAR_TYPE_MISMATCH;
            dlt698_encode_get_response_dar(sa, piid, oad, dar, out);
            return 0;
        }
        dlt698_encode_get_response(sa, piid, oad, s_axdr_buf, dlen, out);
        return 0;
    }
    edge_point_value_t v;
    memset(&v, 0, sizeof(v));
    int rc = (!src || !src->resolve) ? EDGE_SLAVE_SRC_NOT_FOUND
                                     : src->resolve(src_ctx, oad, NULL, 0, &v);
    if (rc != 0) {
        dlt698_encode_get_response_dar(sa, piid, oad, _dar_for_source_rc(rc), out);
        return 0;
    }
    uint8_t data[8];
    size_t dlen = 0;
    if (_encode_axdr_value(&v, data, &dlen) != 0) {
        dlt698_encode_get_response_dar(sa, piid, oad, DLT698_DAR_TYPE_MISMATCH, out);
        return 0;
    }
    dlt698_encode_get_response(sa, piid, oad, data, dlen, out);
    return 0;
}

static int dlt698_req_slave_exchange(void *priv, void *session, const uint8_t *in, size_t in_len,
                                     const edge_slave_source_ops_t *src, void *src_ctx,
                                     net_buf_t *out, size_t *consumed) {
    dlt698_req_t *p = (dlt698_req_t *)priv;
    if (!p || !in || !out || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)in, in_len);
    nb.len = (uint16_t)in_len;

    dlt698_addr_t sa_rx;
    uint8_t svc = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t payload_len = 0;

    uint8_t choice = 0;
    int ret = dlt698_decode_frame_ex(&nb, &sa_rx, &svc, &choice, &piid, &payload, &payload_len);
    if (ret == 1) return EDGE_ERR_INCOMPLETE;
    if (ret < 0) {
        *consumed = 1;
        return -1;
    }
    *consumed = (size_t)(payload + payload_len + 3 - in);

    /* 只应答本机地址 */
    if (p->sa_len && (sa_rx.len != p->sa_len || memcmp(sa_rx.addr, p->sa_addr, p->sa_len) != 0)) {
        return 1;
    }

    /* 应答必须回**请求里的地址**（sa_rx），不能用 priv 里的 p->sa_addr：
     * 从站交换这条路根本没走过 req_prepare，priv 是未初始化的，sa_len 为 0 时
     * 描述符算成 0xFF，整帧长度与 HCS 全错 —— 主站判成坏帧，而值本身是对的。
     * 实测现象："值解出来对，但帧校验不过"。 */
    dlt698_addr_t sa = sa_rx;

    if (svc == 0x02) {
        return _handle_connect(&sa, piid, payload, payload_len, src, src_ctx, session, out);
    }
    if (svc == 0x03) {
        return _handle_release(&sa, piid, session, out);
    }

    if (svc == DLT698_SVC_REPORT_RES) {
        /* REPORT-Response（表99）：PIID 相等才算确认了本次上报；不等就忽略，
         * 免得一条串会话的响应用错了 pending（ADR-0017）。确认不产生应答帧。 */
        dlt698_session_t *s = (dlt698_session_t *)session;
        if (s && s->report_pending && piid == s->report_piid) s->report_pending = false;
        return 0;
    }

    if (svc == DLT698_SVC_GET_REQ) {
        return _handle_get_request(&sa, piid, choice, payload, payload_len, src, src_ctx, session,
                                   out);
    }

    if (svc == DLT698_SVC_SET_REQ) {
        if (choice != 0x01) return -1;
        return _handle_set_request(&sa, piid, payload, payload_len, src, src_ctx, session, out);
    }

    if (svc == DLT698_SVC_ACTION_REQ) {
        /* ACTION-Request-Normal：payload = OMD(4) | Data | 时间标签（PIID 已由 decode 取出） */
        if (payload_len < 4) return -1;
        uint32_t omd = _read_be32(payload);
        if (!_acl_check((dlt698_session_t *)session, src, src_ctx, omd, false, true)) {
            dlt698_encode_action_response(&sa, piid, (uint16_t)(omd >> 16),
                                          (uint8_t)((omd >> 8) & 0xFF), (uint8_t)(omd & 0xFF),
                                          DLT698_DAR_DENIED, NULL, 0, out);
            return 0;
        }

        double param = 0.0;
        if (payload_len > 4) {
            dlt698_data_t d;
            memset(&d, 0, sizeof(d));
            uint16_t off = 0;
            if (dlt698_decode_data(payload + 4, (uint16_t)(payload_len - 4), &off, &d) == 0) {
                switch (d.tag) {
                    case DLT698_TAG_INTEGER:
                        param = (double)d.val.i8;
                        break;
                    case DLT698_TAG_UNSIGNED:
                        param = (double)d.val.u8;
                        break;
                    case DLT698_TAG_LONG:
                        param = (double)d.val.i16;
                        break;
                    case DLT698_TAG_LONG_UNSIGNED:
                        param = (double)d.val.u16;
                        break;
                    case DLT698_TAG_ENUM:
                        param = (double)d.val.u8;
                        break;
                    default:
                        break;
                }
                dlt698_free_data(&d);
            }
        }

        /* 优先把参数字节原样交给产品：结构参数（如 OI 8000 方法 129 的
         * array<structure>）压不成 double，压了就只能猜。 */
        int rc;
        if (src && src->accept_action) {
            rc = src->accept_action(src_ctx, omd, payload + 4, (size_t)(payload_len - 4));
        } else if (src && src->accept) {
            rc = src->accept(src_ctx, omd, param);
        } else {
            rc = -1;
        }

        /* DAR 必须如实回：执行失败回 0（成功）就是"假成功"，主站会以为继电器动了。 */
        uint8_t dar = _dar_for_action_rc(rc);
        dlt698_encode_action_response(&sa, piid, (uint16_t)(omd >> 16),
                                      (uint8_t)((omd >> 8) & 0xFF), (uint8_t)(omd & 0xFF), dar,
                                      NULL, 0, out);
        return 0;
    }

    return -1;
}

/* 周期发帧：只有活动连接上有待上报内容才发一帧（ADR-0017）。
 * 上一帧没确认前不发下一帧，避免同一会话里堆叠未确认上报。 */
static int dlt698_req_tick(void *priv, const edge_north_source_ops_t *north, void *north_ctx,
                           const edge_slave_source_ops_t *slave, void *slave_ctx,
                           net_buf_pool_t *pool, net_buf_t **out_tx) {
    (void)north;
    (void)north_ctx;
    (void)slave_ctx;
    if (!out_tx) return -1;
    *out_tx = NULL;

    dlt698_session_t *s = (dlt698_session_t *)priv;
    if (!s || !s->connected || !pool) return 0;

    uint64_t now = uv_hrtime() / 1000000ULL;
    if (s->report_pending) {
        if (now - s->report_sent_ms < DLT698_REPORT_TIMEOUT_MS) return 0;
        /* 确认丢了只解锁，不重发同一帧（ADR-0017：重发/SnF 另立）。 */
        s->report_pending = false;
    }
    if (!slave || !slave->report.next || !s->sa_len) return 0;

    size_t blen = 0;
    int rc = slave->report.next(slave->report.ctx, s_axdr_buf, sizeof(s_axdr_buf), &blen);
    if (rc != 0 || blen == 0) return 0;
    if (blen < 2) return -1;            /* 体至少 CHOICE + PIID */

    dlt698_addr_t sa = {.len = s->sa_len, .type = 0};
    memcpy(sa.addr, s->sa_addr, s->sa_len);

    net_buf_t *tx = net_buf_alloc(pool, 0);
    if (!tx) return 0;
    if (dlt698_encode_report_frame(&sa, s_axdr_buf, blen, tx) != 0) {
        net_buf_unref(tx);
        return 0;
    }
    s->report_pending = true;
    s->report_piid = s_axdr_buf[1];
    s->report_sent_ms = now;
    *out_tx = tx;
    return 0;
}

const edge_driver_t dlt698_driver = {
    .name = "DLT698",
    .ctx_size = sizeof(dlt698_session_t), /* Class 20 会话与 ACL（ADR-0015） */
    .on_init = dlt698_on_init,
    .req_priv_size = sizeof(dlt698_req_t),
    .req_prepare = dlt698_req_prepare,
    .req_encode = dlt698_req_encode,
    .req_decode = dlt698_req_decode,
    .req_slave_exchange = dlt698_req_slave_exchange,
    .req_tick = dlt698_req_tick,
};

/* File-local prototypes. */
int dlt698_driver_register(driver_manager_t *mgr);
int dlt698_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlt698_driver);
}
