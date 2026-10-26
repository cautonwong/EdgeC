/**
 * @file driver_sparkplug_b.c
 * @brief MQTT Sparkplug B 2.2 北向驱动实现 (Foundation L1)
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <MQTTAsync.h>

#include "edge_driver.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "sparkplug_b.h"

typedef struct {
    char group_id[64];
    char edge_node_id[64];
    uint64_t seq;
    bool birth_sent;
} spb_ctx_t;

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
 */
static int _spb_encode_payload(edge_driver_ctx_t *driver_ctx, spb_ctx_t *ctx, uint32_t tag_id, double val, uint64_t ts, bool is_birth, net_buf_t *out) {
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
    if (tag_id > 0) {
        const char *name = edge_driver_get_tag_name(driver_ctx, tag_id);
        if (!name) name = "unknown_tag";
        _encode_metric_float(b, name, (float)val, ts);
    } else if (is_birth) {
        // [NBIRTH] 发送节点元数据
        _encode_metric_float(b, "bdSeq", 0.0f, ts); // bdSeq should be long, but float for test
    }
    
    return 0;
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
    priv->seq = 0;
    priv->birth_sent = false;
    
    // 主题身份走 env：同一台机器并发跑多个节点时，硬编码会让它们撞主题。
    // 缺省值保持原样，未设置 env 的产品（iems）主题不变。
    _copy_topic_level(priv->group_id, sizeof(priv->group_id), getenv("MQTT_GROUP_ID"),
                      "EdgeOS_IEMS");
    _copy_topic_level(priv->edge_node_id, sizeof(priv->edge_node_id), getenv("MQTT_NODE_ID"),
                      "IEMS_Node_A");
    
    LOG_INFO(LOG_CAT_NB, "Sparkplug B: Driver initialized for Node %s", priv->edge_node_id);
    return 0;
}

static void spb_on_cleanup(edge_driver_ctx_t *ctx) {
    (void)ctx;
    // [Nexus] priv 由 driver_instance_destroy 统一释放
}

static int spb_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    spb_ctx_t *priv = (spb_ctx_t *)ctx->priv;
    // [Nexus] 如果还没发过 BIRTH 报文，在这里尝试
    if (!priv->birth_sent) {
        *out_tx = net_buf_alloc(ctx->pool, 0);
        if (*out_tx) {
            _spb_encode_payload(ctx, priv, 0, 0, uv_hrtime()/1000000, true, *out_tx);
            priv->birth_sent = true;
            return 0;
        }
    }
    return -1; 
}

static int spb_encode_write(edge_driver_ctx_t *ctx, uint32_t tag_id, double value, net_buf_t *out) {
    spb_ctx_t *priv = (spb_ctx_t *)ctx->priv;
    return _spb_encode_payload(ctx, priv, tag_id, value, uv_hrtime() / 1000000, false, out);
}

edge_driver_t sparkplug_b_driver = {
    .name = "SparkplugB",
    .ctx_size = sizeof(spb_ctx_t),
    .on_init = spb_on_init,
    .on_cleanup = spb_on_cleanup,
    .on_tick = spb_on_tick,
    .encode_write = spb_encode_write
};

int sparkplug_b_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &sparkplug_b_driver);
}
