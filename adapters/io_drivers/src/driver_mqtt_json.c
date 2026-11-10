/**
 * @file driver_mqtt_json.c
 * @brief 裸 JSON 北向驱动 —— 第二个协议，用来证明"换协议不改编排"（ADR-0011 N4-4）
 *
 * ## 为什么要有它
 *
 * 只有一个协议时，"可插拔"是没法验证的：注册表里只有一行，写死和查表看不出区别。
 * 这个驱动存在的意义是**对照样本**：它的地址规则、载荷格式、BIRTH 语义
 * 都和 Sparkplug B 完全不同，但编排层一行都不用改。
 *
 * ## 与 Sparkplug B 的差异（都是刻意的）
 *
 * | 维度 | Sparkplug B | 裸 JSON（本驱动） |
 * |---|---|---|
 * | 地址 | `spBv1.0/{group}/{kind}/{node}` | `edge/{node}/{kind}` |
 * | 点位名 | 编进 protobuf 的 metric name | 编进 JSON 字段 |
 * | 载荷 | protobuf（二进制） | JSON（文本） |
 * | BIRTH | NBIRTH + metric 清单 | `{"online":true,...}` |
 *
 * ## 边界
 *
 * 只做「值 → 字节」。不知道 topic、不知道 datahub、不知道点位表。
 * metric 名由北向端口经 `point_desc` 传进来。
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "mqtt_json_codec.h"

/** 设备级会话：只记"本会话是否已发过上线帧" */
typedef struct {
    bool birth_sent;
} mj_ctx_t;

/** 请求级 scratch：这一次要发的点位 */
typedef struct {
    char name[64];
    double value;
} mj_req_t;

/**
 * @brief metric 名是否安全（只允许标识符字符）。
 *
 * 名字会被直接拼进 JSON 字符串。名字来自产品点位表，正常情况下是
 * `VOLTAGE_A` 这种；但配置写错、或者有人从外部塞了带引号的名字，
 * 就会拼出**语法正确的坏 JSON**（主站解析失败但报错指向别处）。
 *
 * 与其转义（转义规则容易写漏），不如直接拒绝——名字本来就该是标识符。
 */
static bool _name_is_safe(const char *name) {
    if (!name || !name[0]) return false;
    for (const char *p = name; *p; p++) {
        char c = *p;
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                  c == '_' || c == '-' || c == '.';
        if (!ok) return false;
    }
    return true;
}

static int mj_req_prepare(void *priv_v, uint32_t slave_id, uint32_t point_id,
                          const void *meta_list, uint32_t meta_list_len, const void *point_desc,
                          size_t point_desc_len, bool is_write, double value) {
    (void)slave_id;
    (void)meta_list;
    (void)meta_list_len;
    (void)point_desc_len;
    (void)is_write; /* 北向只有"发"这一个方向 */

    mj_req_t *req = (mj_req_t *)priv_v;
    if (!req) return -1;

    if (point_desc && _name_is_safe((const char *)point_desc)) {
        snprintf(req->name, sizeof(req->name), "%s", (const char *)point_desc);
    } else if (point_desc) {
        /* 名字不安全：明确拒绝，不要拼出一个坏 JSON。 */
        LOG_WARN(LOG_CAT_NB, "MqttJson: metric name rejected (not an identifier)");
        return -1;
    } else {
        snprintf(req->name, sizeof(req->name), "tag_%u", point_id);
    }

    req->value = value;
    return 0;
}

static int mj_req_encode(void *priv_v, void *session, net_buf_t *out) {
    (void)session;
    mj_req_t *req = (mj_req_t *)priv_v;
    if (!req || !out) return -1;

    /* %.15g：double 的往返精度，避免 %.2f 把 0.123456 截成 0.12 */
    char tmp[128];
    int n = snprintf(tmp, sizeof(tmp), "{\"p\":\"%s\",\"v\":%.15g}", req->name, req->value);
    if (n <= 0 || (size_t)n >= sizeof(tmp)) return -1;

    if (out->b.len + (size_t)n > out->b.size) return -1;
    net_buf_simple_add_mem(&out->b, tmp, (size_t)n);
    return 0;
}

/**
 * @brief 上线帧：每个会话一次。
 *
 * 与 Sparkplug 的 NBIRTH 同构（主站要知道"这个节点上线了"），
 * 但载荷是 JSON 而不是 protobuf——正好用来验证编排层不关心载荷格式。
 */
static int mj_req_tick(void *priv_v, const edge_north_source_ops_t *north, void *north_ctx,
                       const edge_slave_source_ops_t *slave, void *slave_ctx, net_buf_pool_t *pool,
                       net_buf_t **out_tx) {
    (void)north; /* 上线帧不带点位清单（JSON 版由主站按需查询） */
    (void)north_ctx;
    (void)slave;
    (void)slave_ctx;

    mj_ctx_t *priv = (mj_ctx_t *)priv_v;
    if (!priv || !pool || !out_tx) return -1;
    if (priv->birth_sent) return -1;

    net_buf_t *nb = net_buf_alloc(pool, 0);
    if (!nb) return -1;

    char tmp[96];
    int n = snprintf(tmp, sizeof(tmp), "{\"online\":true,\"ts\":%llu}",
                     (unsigned long long)((uint64_t)time(NULL) * 1000ULL));
    if (n <= 0 || (size_t)n >= sizeof(tmp) || nb->b.len + (size_t)n > nb->b.size) {
        net_buf_unref(nb);
        return -1;
    }
    net_buf_simple_add_mem(&nb->b, tmp, (size_t)n);

    priv->birth_sent = true;
    *out_tx = nb;
    return 0;
}

/** 会话重建：新会话要重发上线帧（理由同 sparkplug 的 birth_sent）。 */
static int mj_req_session_reset(void *priv_v) {
    mj_ctx_t *priv = (mj_ctx_t *)priv_v;
    if (!priv) return -1;
    priv->birth_sent = false;
    return 0;
}

const edge_driver_t mqtt_json_driver = {
    .name = "MqttJson",
    .ctx_size = sizeof(mj_ctx_t),
    .req_priv_size = sizeof(mj_req_t),
    .req_prepare = mj_req_prepare,
    .req_encode = mj_req_encode,
    .req_tick = mj_req_tick,
    .req_session_reset = mj_req_session_reset,
};

int mqtt_json_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &mqtt_json_driver);
}

/* ==============================================================================
 * 下行解码（N6-2）
 *
 * 格式：{"p":"<metric 名>","v":<数值>}
 * 键顺序无关、允许空白、允许其它键（忽略）。
 * 只接受扁平对象——嵌套结构在这里没有语义，出现了就是发错了。
 * ========================================================================== */

typedef struct {
    const char *p;
    const char *end;
} _cur_t;

static void _skip_ws(_cur_t *c) {
    while (c->p < c->end && (*c->p == ' ' || *c->p == '\t' || *c->p == '\r' || *c->p == '\n')) {
        c->p++;
    }
}

static int _expect(_cur_t *c, char ch) {
    _skip_ws(c);
    if (c->p >= c->end || *c->p != ch) return -1;
    c->p++;
    return 0;
}

/** 读一个 JSON 字符串（不做转义展开：metric 名按约定是标识符）。 */
static int _read_string(_cur_t *c, char *out, size_t out_sz) {
    if (_expect(c, '"') != 0) return -1;
    size_t n = 0;
    while (c->p < c->end && *c->p != '"') {
        char ch = *c->p++;
        if (ch == '\\') return -2; /* 转义不支持：名字里不该有 */
        if (n + 1 >= out_sz) return -3;
        out[n++] = ch;
    }
    if (c->p >= c->end) return -1;
    c->p++; /* 跳过结尾引号 */
    out[n] = '\0';
    return 0;
}

static int _read_number(_cur_t *c, double *out) {
    _skip_ws(c);
    const char *start = c->p;
    if (c->p < c->end && (*c->p == '-' || *c->p == '+')) c->p++;
    bool any = false;
    while (c->p < c->end) {
        char ch = *c->p;
        if ((ch >= '0' && ch <= '9') || ch == '.' || ch == 'e' || ch == 'E' || ch == '-' ||
            ch == '+') {
            c->p++;
            any = true;
        } else {
            break;
        }
    }
    if (!any) return -1;

    char buf[64];
    size_t n = (size_t)(c->p - start);
    if (n >= sizeof(buf)) return -2;
    memcpy(buf, start, n);
    buf[n] = '\0';
    if (sscanf(buf, "%lf", out) != 1) return -3;
    return 0;
}

int mqtt_json_decode_downlink(const uint8_t *payload, size_t len, char *name, size_t name_sz,
                              double *value) {
    if (!payload || !name || !value || name_sz == 0) return -1;

    _cur_t c = {.p = (const char *)payload, .end = (const char *)payload + len};

    char tmp_name[64];
    tmp_name[0] = '\0';
    double tmp_value = 0.0;
    bool got_name = false, got_value = false;

    if (_expect(&c, '{') != 0) return -1;

    _skip_ws(&c);
    if (c.p < c.end && *c.p == '}') return -1; /* 空对象：没有 p/v */

    for (;;) {
        char key[32];
        if (_read_string(&c, key, sizeof(key)) != 0) return -1;
        if (_expect(&c, ':') != 0) return -1;

        if (strcmp(key, "p") == 0) {
            if (_read_string(&c, tmp_name, sizeof(tmp_name)) != 0) return -1;
            got_name = true;
        } else if (strcmp(key, "v") == 0) {
            if (_read_number(&c, &tmp_value) != 0) return -1;
            got_value = true;
        } else {
            /* 其它键：跳过值（只支持标量，嵌套对象直接判错） */
            _skip_ws(&c);
            if (c.p < c.end && *c.p == '"') {
                char sink[64];
                if (_read_string(&c, sink, sizeof(sink)) != 0) return -1;
            } else if (c.p < c.end && (*c.p == '{' || *c.p == '[')) {
                return -1;
            } else {
                double sink;
                if (_read_number(&c, &sink) != 0) return -1;
            }
        }

        _skip_ws(&c);
        if (c.p < c.end && *c.p == ',') {
            c.p++;
            continue;
        }
        if (c.p < c.end && *c.p == '}') {
            c.p++;
            break;
        }
        return -1;
    }

    /* 两个字段都齐了才写出去 —— 半解析的结果比失败更难查 */
    if (!got_name || !got_value) return -1;
    if (!_name_is_safe(tmp_name)) return -1;
    snprintf(name, name_sz, "%s", tmp_name);
    *value = tmp_value;
    return 0;
}
