/**
 * @file iec101_link.h
 * @brief IEC 60870-5-101 链路层 (FT1.2) 定义
 */

#ifndef EDGE_IEC101_LINK_H
#define EDGE_IEC101_LINK_H

#include <stdbool.h>
#include <stdint.h>

#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 101 控制域定义 (Control Field)
 */
typedef struct {
    uint8_t func : 4; /**< 功能码 */
    uint8_t fcv : 1;  /**< 帧计数有效位 (Frame Count Valid) */
    uint8_t fcb : 1;  /**< 帧计数位 (Frame Count Bit) */
    uint8_t prm : 1;  /**< 启动报文标志 (Primary) */
    uint8_t res : 1;  /**< 预留 */
} iec101_ctrl_t;

/**
 * @brief 101 链路管理器
 */
typedef struct {
    uint16_t link_addr;
    uint8_t addr_len; /**< 链路地址长度 (1 or 2) */
    bool last_fcb;    /**< 上次发送的 FCB 状态 */
} iec101_link_mgr_t;

typedef enum {
    IEC101_FRAME_INCOMPLETE = 1,
    IEC101_FRAME_OK = 0,
    IEC101_FRAME_ERR_CS = -1,
    IEC101_FRAME_ERR_FORMAT = -2,
} iec101_parse_ret_t;

/**
 * @brief 构建固定长度帧 (0x10) - 用于请求与确认
 */
int iec101_link_build_fixed(iec101_link_mgr_t *mgr, iec101_ctrl_t ctrl, net_buf_t *out);

/**
 * @brief 构建可变长度帧 (0x68) - 承载 ASDU
 * @param asdu 已包含业务数据的 net_buf
 */
int iec101_link_wrap_asdu(iec101_link_mgr_t *mgr, iec101_ctrl_t ctrl, net_buf_t *asdu);

/**
 * @brief 解析并验证 FT1.2 帧 (Phase 5 Task 4)
 * @param in 输入缓冲区
 * @param consumed 已消耗的字节数 (包括错误或丢弃的字节)
 * @param out_ctrl 输出控制域
 * @param out_asdu 输出 ASDU 数据片段 (指向 in->data 的内部视图)
 */
int iec101_link_parse(iec101_link_mgr_t *mgr, struct net_buf_simple *in, size_t *consumed,
                      iec101_ctrl_t *out_ctrl, struct net_buf_simple *out_asdu);

/**
 * @brief 计算并附加 Sum8 校验和
 */
uint8_t iec101_calc_sum8(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_IEC101_LINK_H
