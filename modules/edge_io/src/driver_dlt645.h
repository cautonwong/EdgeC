/**
 * @file driver_dlt645.h
 * @brief DL/T 645 纯编解码（ADR-002 的 S1 样板）
 *
 * 这个头**不进公开 include**：`dlt645_req_t` 是驱动私有数据，请求层只按
 * `req_priv_size` 分配一块不透明缓冲，不知道里面的字段。
 *
 * 六条边界纪律（见 docs/design/DLT645_REQUEST_CODEC.md）：
 *   1. 驱动里没有 tag_id，只有 di（645 原生标识）；
 *   2. 驱动不查表、不读影子表、不碰 datahub；
 *   3. 驱动不产生 pipe_msg_t，只把值填回 dlt645_req_t；
 *   4. 全部是纯函数，无 ctx、无全局、无副作用；
 *   5. 组帧（CS、+0x33、控制码）留在这一层，cmd 不该知道；
 *   6. 从站取数由调用方给，协议层不认点位模型。
 */
#ifndef DRIVER_DLT645_H
#define DRIVER_DLT645_H

#include <stddef.h>
#include <stdint.h>

#include "net_buf.h"

/* 控制码 */
#define DLT645_CTRL_READ        0x11 /* 主站读请求 */
#define DLT645_CTRL_READ_RSP    0x91 /* 从站读应答 */
#define DLT645_CTRL_WRITE       0x14 /* 主站写请求 */
#define DLT645_CTRL_WRITE_RSP   0x94 /* 从站写应答 */
#define DLT645_CTRL_ABNORMAL    0xD1 /* 从站异常应答 */

#define DLT645_ADDR_LEN 6
#define DLT645_DI_LEN   4

/** 请求私有数据：驱动认识的全部"点位"信息都在这里 */
typedef struct {
    uint32_t di;                     /**< 数据标识（645 原生，低字节先上线） */
    uint8_t  addr_bcd[DLT645_ADDR_LEN]; /**< 表地址，低字节先 */
    uint8_t  ctrl;                   /**< 控制码 */
    uint8_t  data_len;               /**< 应答数据段字节数，由应答回填 */
    int32_t  raw;                    /**< 解码出的原始整数（未标定） */
    double   value;                  /**< 工程量；主站侧解码结果 / 从站侧待写值 */
} dlt645_req_t;

/* --- 主站侧 --- */
int dlt645_encode_read(const dlt645_req_t *req, net_buf_t *out);
int dlt645_encode_write(const dlt645_req_t *req, net_buf_t *out);

/**
 * @return 0=解出完整帧；1=已跳过前导垃圾（*consumed=跳过的字节数）；
 *         EDGE_ERR_INCOMPLETE=帧未收全；-1=校验/格式错误
 */
int dlt645_decode_read_rsp(dlt645_req_t *req, const uint8_t *buf, size_t len, size_t *consumed);
int dlt645_decode_write_rsp(dlt645_req_t *req, const uint8_t *buf, size_t len, size_t *consumed);

/* --- 从站侧 --- */
int dlt645_decode_slave_req(dlt645_req_t *req, const uint8_t *buf, size_t len, size_t *consumed);
int dlt645_encode_slave_rsp(const dlt645_req_t *req, net_buf_t *out);

#endif /* DRIVER_DLT645_H */
