#include "can_codec.h"
#include <string.h>

int can_decode_frame(struct net_buf_simple *in, edge_can_frame_t *out) {
    if (in->len < 16) return 1; // SocketCAN frame size is typically 16 bytes (struct can_frame)
    
    // 我们假设 net_buf 里存的是 Linux 标准 struct can_frame
    // id(4) + len(1) + pad(3) + data(8)
    uint8_t *p = in->data;
    out->can_id = (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
    out->can_dlc = p[4];
    memcpy(out->data, &p[8], 8);
    
    net_buf_simple_pull(in, 16);
    return 0;
}

int can_encode_frame(const edge_can_frame_t *in, net_buf_t *out) {
    uint8_t *p = net_buf_add(out, 16);
    memset(p, 0, 16);
    p[0] = (uint8_t)(in->can_id & 0xFF);
    p[1] = (uint8_t)((in->can_id >> 8) & 0xFF);
    p[2] = (uint8_t)((in->can_id >> 16) & 0xFF);
    p[3] = (uint8_t)((in->can_id >> 24) & 0xFF);
    p[4] = in->can_dlc;
    memcpy(&p[8], in->data, 8);
    return 0;
}
