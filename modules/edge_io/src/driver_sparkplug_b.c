/**
 * @file driver_sparkplug_b.c
 * @brief MQTT Sparkplug B 2.2 北向驱动 —— 纯 codec（ADR-002 S6）
 *
 * ## 定位
 *
 * 北向生产者：把「点位 + 值」编成 NBIRTH / NDATA 报文，topic 由北向管理器拼。
 * 驱动不认识 datahub、不认识点位表——metric 名由北向端口 `name()` 给，
 * 通过 `point_desc` 传进来。
 *
 * ## 两个 priv
 *
 * - `spb_ctx_t`（`ctx_size`）：会话，主题身份 + 序号 + 是否已发过 BIRTH
 * - `spb_req_t`（`req_priv_size`）：请求 scratch，这一次要发的 metric
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <MQTTAsync.h>

#include "edge_driver.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "sparkplug_b.h"

/** 设备级会话：主题身份与序号必须跨帧保留 */
typedef struct {
    char group_id[64];
    char edge_node_id[64];
    uint64_t seq;
    bool birth_sent;
} spb_ctx_t;

/** 请求级 scratch：只放这一次要发的 metric */
typedef struct {
    uint32_t tag_id;
    char name[64];
    double value;
} spb_req_t;

/**
 * @brief 编码一个 Metric 子消息 (Protobuf Nested)
 */
static void _encode_metric_float(struct net_buf_simple *b, const char *name, float val, uint64_t ts) {
    struct net_buf_simple metric_b;
    uint8_t tmp_buf[256];
    net_buf_simple_init_with_data(&metric_b, tmp_buf, sizeof(tmp_buf));

    // 1. Name
    spb_encode_string(&metric_b, SPB_METRIC_NAME, name);
    // 2. Timestamp
    spb_encode_tag(&metric_b, SPB_METRIC_TIMESTAMP, 0);
    spb_encode_varint(&metric_b, ts);
    // 3. DataType (Float = 9)
    spb_encode_tag(&metric_b, SPB_METRIC_DATATYPE, 0);
    spb_encode_varint(&metric_b, SPB_TYPE_FLOAT);
    // 4. Value
    spb_encode_tag(&metric_b, SPB_METRIC_FLOAT_VAL, 5); // 32-bit
    net_buf_simple_add_le32(&metric_b, *(uint32_t *)&val);

    // Wrap as length-delimited in parent
    spb_encode_tag(b, SPB_FIELD_METRICS, 2);
    spb_encode_varint(b, metric_b.len);
    net_buf_simple_add_mem(b, metric_b.data, metric_b.len);
}

/**
 * @brief 编码 NDATA/NBIRTH Payload
 *
 * @param name  metric 名；NBIRTH 传 NULL（只发 bdSeq 节点元数据）
 */
static void _spb_encode_payload(spb_ctx_t *ctx, const char *name, double val, uint64_t ts,
                                bool is_birth, net_buf_t *out) {
    struct net_buf_simple *b = &out->b;

    // 1. Root Timestamp
    spb_encode_tag(b, SPB_FIELD_TIMESTAMP, 0);
    spb_encode_varint(b, ts);

    // 2. Seq (NBIRTH seq must be 0)
    spb_encode_tag(b, SPB_FIELD_SEQ, 0);
    if (is_birth) {
        spb_encode_varint(b, 0);
        ctx->seq = 1;
    } else {
        spb_encode_varint(b, ctx->seq++);
        if (ctx->seq > 255) ctx->seq = 0;
    }

    // 3. Metric
    if (name) {
        _encode_metric_float(b, name, (float)val, ts);
    } else if (is_birth) {
        // [NBIRTH] 发送节点元数据
        _encode_metric_float(b, "bdSeq", 0.0f, ts); // bdSeq should be long, but float for test
    }
}

/* 主题层级只允许可安全拼进 `spBv1.0/<group>/NDATA/<node>` 的字符集：
 * 空值、含 '/' '+' '#' 或空格的值会让主题被拆层或变成通配，一律回落缺省并告警。 */
static void _copy_topic_level(char *dst, size_t dst_size, const char *env, const char *fallback) {
    const char *src = fallback;
    if (env && env[0]) {
        bool ok = true;
        for (const char *p = env; *p; p++) {
            char c = *p;
            bool legal = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                         (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
            if (!legal) {
                ok = false;
                break;
            }
        }
        if (ok) {
            src = env;
        } else {
            LOG_WARN(LOG_CAT_NB, "Sparkplug: illegal topic level '%s', falling back to '%s'", env,
                     fallback);
        }
    }
    snprintf(dst, dst_size, "%s", src);
}

static int spb_on_init(edge_driver_ctx_t *ctx) {
    spb_ctx_t *priv = (spb_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(*priv));

    // 主题身份走 env：同一台机器并发跑多个节点时，硬编码会让它们撞主题。
    // 缺省值保持原样，未设置 env 的产品（iems）主题不变。
    _copy_topic_level(priv->group_id, sizeof(priv->group_id), getenv("MQTT_GROUP_ID"),
                      "EdgeOS_IEMS");
    _copy_topic_level(priv->edge_node_id, sizeof(priv->edge_node_id), getenv("MQTT_NODE_ID"),
                      "IEMS_Node_A");

    LOG_INFO(LOG_CAT_NB, "Sparkplug B: Driver initialized for Node %s", priv->edge_node_id);
    return 0;
}

/* 主题身份访问器：北向管理器用它拼 topic，不再 cast ctx->priv */
int sparkplug_b_identity(const void *session, char *group, size_t group_sz, char *node,
                         size_t node_sz) {
    const spb_ctx_t *priv = (const spb_ctx_t *)session;
    if (!priv || !group || !node || group_sz == 0 || node_sz == 0) return -1;
    snprintf(group, group_sz, "%s", priv->group_id);
    snprintf(node, node_sz, "%s", priv->edge_node_id);
    return 0;
}

/* ==================== 主站侧：组 NDATA ==================== */

static int spb_req_prepare(void *priv_v, uint32_t slave_id, uint32_t point_id,
                           const void *meta_list, uint32_t meta_list_len, const void *point_desc,
                           size_t point_desc_len, bool is_write, double value) {
    (void)slave_id;
    (void)meta_list;
    (void)meta_list_len;
    (void)point_desc_len;
    (void)is_write; /* 北向只有"发"这一个方向 */
    spb_req_t *req = (spb_req_t *)priv_v;
    if (!req) return -1;

    req->tag_id = point_id;
    req->value = value;

    /* metric 名由北向端口经 point_desc 传进来（NUL 结尾字符串）。
     * 没给就回落成数字标识——不猜、也不去反查 datahub 元数据。 */
    if (point_desc) {
        const char *name = (const char *)point_desc;
        snprintf(req->name, sizeof(req->name), "%s", name);
    } else {
        snprintf(req->name, sizeof(req->name), "tag_%u", point_id);
    }
    return 0;
}

static int spb_req_encode(void *priv_v, void *session, net_buf_t *out) {
    spb_req_t *req = (spb_req_t *)priv_v;
    spb_ctx_t *s = (spb_ctx_t *)session;
    if (!req || !s || !out) return -1;
    _spb_encode_payload(s, req->name, req->value, uv_hrtime() / 1000000, false, out);
    return 0;
}

/* ==================== 周期发帧：NBIRTH ==================== */

static int spb_req_tick(void *priv_v, const edge_north_source_ops_t *north, void *north_ctx,
                        const edge_slave_source_ops_t *slave, void *slave_ctx,
                        net_buf_pool_t *pool, net_buf_t **out_tx) {
    (void)north; /* BIRTH 不带点位，不需要端口 */
    (void)north_ctx;
    (void)slave; /* 北向驱动没有从站面 */
    (void)slave_ctx;
    spb_ctx_t *priv = (spb_ctx_t *)priv_v;
    if (!priv || !pool || !out_tx) return -1;

    if (priv->birth_sent) return -1;
    *out_tx = net_buf_alloc(pool, 0);
    if (!*out_tx) return -1;
    _spb_encode_payload(priv, NULL, 0, uv_hrtime() / 1000000, true, *out_tx);
    priv->birth_sent = true;
    return 0;
}

const edge_driver_t sparkplug_b_driver = {
    .name = "SparkplugB",
    .ctx_size = sizeof(spb_ctx_t),        /* 序号与主题身份必须跨帧 */
    .req_priv_size = sizeof(spb_req_t),   /* 请求 scratch 只放这一个 metric */
    .on_init = spb_on_init,
    .req_prepare = spb_req_prepare,
    .req_encode = spb_req_encode,
    .req_tick = spb_req_tick,
};

int sparkplug_b_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &sparkplug_b_driver);
}
