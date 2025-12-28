#ifndef WS_PROTO_H
#define SVC_WS_PROTO_H

#include <stddef.h>
#include <stdint.h>
#include <uv.h>

// WebSocket 帧操作码
typedef enum {
    WS_OP_CONT = 0x0,
    WS_OP_TEXT = 0x1,
    WS_OP_BIN = 0x2,
    WS_OP_CLOSE = 0x8,
    WS_OP_PING = 0x9,
    WS_OP_PONG = 0xA
} ws_opcode_t;

/**
 * @brief 计算 WebSocket 握手回复 Key
 * @param client_key 客户端发来的 Sec-WebSocket-Key
 * @param out_accept 用于存储生成的 Sec-WebSocket-Accept
 */
int ws_gen_accept_key(const char *client_key, char *out_accept);

/**
 * @brief 构造 WebSocket 数据帧 (Server -> Client 无需掩码)
 * @param payload 数据载荷
 * @param len 数据长度
 * @param out_frame 缓冲区 (应至少为 len + 10)
 * @param out_len 生成的帧总长度
 */
int ws_build_frame(ws_opcode_t opcode, const uint8_t *payload, size_t len, uint8_t *out_frame,
                   size_t *out_len);

#endif
