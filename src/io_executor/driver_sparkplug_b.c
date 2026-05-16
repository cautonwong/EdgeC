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
#include "datahub.h"

typedef struct {
    char group_id[64];
    char edge_node_id[64];
    uint64_t seq;
    datahub_t *hub;
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
static int _spb_encode_payload(spb_ctx_t *ctx, uint32_t tag_id, double val, uint64_t ts, bool is_birth, net_buf_t *out) {
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
        const edge_tag_meta_t *meta = datahub_get_metadata_by_tag(ctx->hub, tag_id);
        const char *name = meta ? meta->name : "unknown_tag";
        _encode_metric_float(b, name, (float)val, ts);
    } else if (is_birth) {
        // [NBIRTH] 发送节点元数据
        _encode_metric_float(b, "bdSeq", 0.0f, ts); // bdSeq should be long, but float for test
    }
    
    return 0;
}

static int spb_on_init(edge_driver_ctx_t *ctx) {
    spb_ctx_t *priv = (spb_ctx_t *)ctx->priv;
    priv->hub = (datahub_t *)ctx->user_data;
    priv->seq = 0;
    priv->birth_sent = false;
    
    strncpy(priv->group_id, "EdgeOS_IEMS", 63);
    strncpy(priv->edge_node_id, "IEMS_Node_A", 63);
    
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
            _spb_encode_payload(priv, 0, 0, uv_hrtime()/1000000, true, *out_tx);
            priv->birth_sent = true;
            return 0;
        }
    }
    return -1; 
}

static int spb_encode_write(edge_driver_ctx_t *ctx, uint32_t tag_id, double value, net_buf_t *out) {
    spb_ctx_t *priv = (spb_ctx_t *)ctx->priv;
    return _spb_encode_payload(priv, tag_id, value, uv_hrtime() / 1000000, false, out);
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
