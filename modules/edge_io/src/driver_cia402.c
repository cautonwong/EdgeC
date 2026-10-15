/**
 * @file driver_cia402.c
 * @brief CANopen CiA 402 Servo Drive Profile Driver
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cia402.h"
#include "can_codec.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"

typedef struct {
    uint32_t node_id;
    uint16_t statusword;
    uint16_t controlword;
    int32_t position_actual;
    int32_t position_target;
    cia402_state_t state;
    
    // Mapping from tag_id to CiA 402 objects
    uint32_t tag_statusword;
    uint32_t tag_controlword;
    uint32_t tag_pos_actual;
    uint32_t tag_pos_target;
} cia402_ctx_t;

static int cia402_on_init(edge_driver_ctx_t *ctx) {
    cia402_ctx_t *priv = (cia402_ctx_t *)ctx->priv;
    priv->node_id = ctx->device_id;
    priv->state = CIA402_STATE_UNKNOWN;
    
    // Default tag mapping (should be injected via contract)
    priv->tag_statusword = 6001;
    priv->tag_controlword = 6002;
    priv->tag_pos_actual = 6003;
    priv->tag_pos_target = 6004;
    
    LOG_INFO(LOG_CAT_IO, "CiA 402: Driver initialized for Node %u", priv->node_id);
    return 0;
}

static int cia402_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                            pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)resp_out;
    cia402_ctx_t *priv = (cia402_ctx_t *)ctx->priv;
    edge_can_frame_t frame;
    
    if (can_decode_frame(&in->b, &frame) != 0) {
        return EDGE_ERR_INCOMPLETE;
    }
    *consumed = 16; // SocketCAN frame size
    
    uint32_t cob_id = frame.can_id & 0x7FF;
    *async_count = 0;

    // 1. Check if frame belongs to this Node
    // TPDO1 (Statusword)
    if (cob_id == (0x180 + priv->node_id)) {
        priv->statusword = (uint16_t)(frame.data[0] | (frame.data[1] << 8));
        priv->state = cia402_get_state(priv->statusword);
        
        pipe_msg_init_scalar(&async_msgs[(*async_count)++], priv->tag_statusword, (double)priv->statusword, uv_hrtime());
        LOG_DEBUG(LOG_CAT_IO, "CiA 402: Node %u Statusword 0x%04X -> State %d", priv->node_id, priv->statusword, priv->state);
    }
    // TPDO2 (Actual Position)
    else if (cob_id == (0x280 + priv->node_id)) {
        memcpy(&priv->position_actual, frame.data, 4);
        pipe_msg_init_scalar(&async_msgs[(*async_count)++], priv->tag_pos_actual, (double)priv->position_actual, uv_hrtime());
    }
    
    return 0;
}

static int cia402_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    cia402_ctx_t *priv = (cia402_ctx_t *)ctx->priv;

    // Read Controlword and Target Position via consumer-defined driver port
    pipe_msg_t msg;
    bool needs_tx = false;
    
    // RPDO1 (Controlword)
    if (edge_driver_read_shadow(ctx, priv->tag_controlword, &msg) == 0) {
        uint16_t new_ctrl = (uint16_t)msg.payload.val.f;
        if (new_ctrl != priv->controlword) {
            priv->controlword = new_ctrl;
            
            edge_can_frame_t frame = {
                .can_id = 0x200 + priv->node_id,
                .can_dlc = 2
            };
            frame.data[0] = (uint8_t)(priv->controlword & 0xFF);
            frame.data[1] = (uint8_t)(priv->controlword >> 8);
            
            *out_tx = net_buf_alloc(ctx->pool, 0);
            can_encode_frame(&frame, *out_tx);
            needs_tx = true;
        }
    }
    
    // RPDO2 (Target Position) - If not already sent a Controlword
    if (!needs_tx && edge_driver_read_shadow(ctx, priv->tag_pos_target, &msg) == 0) {
        int32_t new_pos = (int32_t)msg.payload.val.f;
        if (new_pos != priv->position_target) {
            priv->position_target = new_pos;
            
            edge_can_frame_t frame = {
                .can_id = 0x300 + priv->node_id,
                .can_dlc = 4
            };
            memcpy(frame.data, &priv->position_target, 4);
            
            *out_tx = net_buf_alloc(ctx->pool, 0);
            can_encode_frame(&frame, *out_tx);
            needs_tx = true;
        }
    }

    return needs_tx ? 0 : -1;
}

edge_driver_t canopen_cia402_driver = {
    .name = "CiA402",
    .ctx_size = sizeof(cia402_ctx_t),
    .on_init = cia402_on_init,
    .handle_rx = cia402_handle_rx,
    .on_tick = cia402_on_tick
};

int cia402_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &canopen_cia402_driver);
}
