#ifndef DLMS_HDLC_H
#define DLMS_HDLC_H

#include <stdint.h>

#include "net_buf.h"

typedef enum {
    HDLC_STATE_DISCONNECTED,
    HDLC_STATE_CONNECTING,
    HDLC_STATE_CONNECTED,
    HDLC_STATE_WAIT_ACK,
    HDLC_STATE_RECOVERY
} dlms_hdlc_role_state_t;

typedef struct {
    dlms_hdlc_role_state_t state;
    uint8_t server_addr;
    uint8_t client_addr;
    uint8_t ns;                 // 发送序列号 N(S)
    uint8_t nr;                 // 接收序列号 N(R)
    net_buf_t *last_tx_frame;   // [专家级] 用于重传的备份缓冲区
    net_buf_t *reassembly_buf;  // [高质量] 用于分帧重组
} dlms_hdlc_manager_t;

void dlms_hdlc_reset(dlms_hdlc_manager_t *mgr);
int dlms_hdlc_build_snrm(dlms_hdlc_manager_t *mgr, net_buf_t *out);
int dlms_hdlc_send_iframe(dlms_hdlc_manager_t *mgr, net_buf_t *apdu, net_buf_t *out);
int dlms_hdlc_handle_ack(dlms_hdlc_manager_t *mgr, uint8_t control_byte);
int dlms_hdlc_packetize(dlms_hdlc_manager_t *mgr, net_buf_t *apdu, size_t offset, size_t mtu,
                        net_buf_t *out);
int dlms_hdlc_depacketize(dlms_hdlc_manager_t *mgr, struct net_buf_simple *in, net_buf_pool_t *pool,
                          net_buf_t **out_apdu);

#endif
