/**
 * @file driver_ccsds.c
 * @brief 航空航天级 CCSDS 空间数据包遥测驱动
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "edge_driver.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "datahub.h"

// CCSDS 133.0-B-1 Primary Header (6 bytes)
typedef struct __attribute__((packed)) {
    uint16_t stream_id;      // Version (3), Type (1), Sec Hdr (1), APID (11)
    uint16_t sequence;       // Seq Flags (2), Seq Count (14)
    uint16_t data_length;    // Length of packet data field - 1
} ccsds_primary_header_t;

typedef struct {
    uint32_t node_id;
    datahub_t *hub;
    uint32_t telemetry_tag;
    uint16_t seq_count;
} ccsds_ctx_t;

static int ccsds_on_init(edge_driver_ctx_t *ctx) {
    ccsds_ctx_t *priv = (ccsds_ctx_t *)ctx->priv;
    priv->node_id = ctx->device_id;
    priv->hub = (datahub_t *)ctx->user_data;
    
    priv->telemetry_tag = 200; // Default output tag from AHRS or similar
    priv->seq_count = 0;

    LOG_INFO(LOG_CAT_IO, "CCSDS Telemetry Driver: Initialized for APID %u", priv->node_id);
    return 0;
}

static int ccsds_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    ccsds_ctx_t *priv = (ccsds_ctx_t *)ctx->priv;
    if (!priv->hub && ctx->user_data) {
        priv->hub = (datahub_t *)ctx->user_data;
    }
    if (!priv->hub) return -1;

    pipe_msg_t msg;
    bool needs_tx = false;

    // Read latest telemetry from DataHub
    if (datahub_read_shadow(priv->hub, priv->telemetry_tag, &msg) == 0) {
        // Serialize telemetry to CCSDS Packet
        // Suppose it's a vector (e.g. Quaternion) or a scalar
        uint32_t payload_len = 0;
        uint8_t payload_buf[128];
        
        if (msg.type == PIPEMSG_TYPE_BATCH) {
            payload_len = msg.payload.batch.count * sizeof(double);
            memcpy(payload_buf, msg.payload.batch.ptr, payload_len);
        } else if (msg.type == PIPEMSG_TYPE_SCALAR) {
            payload_len = sizeof(double);
            memcpy(payload_buf, &msg.payload.val.f, payload_len);
        } else {
            return -1; // Unhandled format
        }

        // Construct Primary Header
        ccsds_primary_header_t hdr;
        
        // Version=000 (0), Type=0 (Telemetry), SecHdr=0 (Absent), APID=node_id
        uint16_t stream_id = (priv->node_id & 0x07FF);
        hdr.stream_id = htons(stream_id);
        
        // SeqFlags=11 (Unsegmented, 3), SeqCount=incrementing
        uint16_t seq = (3 << 14) | (priv->seq_count & 0x3FFF);
        hdr.sequence = htons(seq);
        priv->seq_count++;
        
        // Length = Data Field Length - 1
        hdr.data_length = htons(payload_len - 1);
        
        // Allocate buffer and pack
        *out_tx = net_buf_alloc(ctx->pool, sizeof(ccsds_primary_header_t) + payload_len);
        memcpy((*out_tx)->b.data, &hdr, sizeof(ccsds_primary_header_t));
        memcpy((*out_tx)->b.data + sizeof(ccsds_primary_header_t), payload_buf, payload_len);
        (*out_tx)->b.len = sizeof(ccsds_primary_header_t) + payload_len;
        
        needs_tx = true;
    }

    return needs_tx ? 0 : -1;
}

edge_driver_t ccsds_telemetry_driver = {
    .name = "CCSDS",
    .ctx_size = sizeof(ccsds_ctx_t),
    .on_init = ccsds_on_init,
    .on_tick = ccsds_on_tick
};

int ccsds_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &ccsds_telemetry_driver);
}
