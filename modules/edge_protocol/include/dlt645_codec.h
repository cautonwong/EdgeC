#ifndef EDGE_DLT645_CODEC_H
#define EDGE_DLT645_CODEC_H

#include <stdint.h>
#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DLT645 报文控制码定义
 */
#define DLT645_CTRL_READ_DATA           0x11
#define DLT645_CTRL_READ_RESPONSE       0x91
#define DLT645_CTRL_WRITE_DATA          0x14
#define DLT645_CTRL_WRITE_RESPONSE      0x94
#define DLT645_CTRL_READ_ADDR           0x13
#define DLT645_CTRL_ERROR_RESPONSE      0xD1

/**
 * @brief DLT645 纯协议内核接口 (Zero-Dependency)
 */

/* --- 基础工具 --- */
uint8_t dlt645_calculate_cs(const uint8_t *buf, size_t len);

/**
 * @brief 将 64位地址转换为 6字节 BCD (Little Endian in 645)
 */
void dlt645_u64_to_bcd_addr(uint64_t addr, uint8_t *out_bcd);

/**
 * @brief BCD 转 浮点 (处理 0x33 偏移后的原始值)
 */
double dlt645_bcd_to_f64(const uint8_t *bcd, int bytes, int decimals);

/* --- 编解码逻辑 --- */

/**
 * @brief 构造 DLT645 请求帧 (自动处理 FEFE 前导符和 0x33 偏移)
 * @param addr_bcd 6字节从站地址
 * @param ctrl 控制码
 * @param di 数据标识 (4字节)
 * @param data 附加数据
 * @param data_len 附加数据长度
 * @param out 输出缓冲区
 */
int dlt645_encode_frame(const uint8_t *addr_bcd, uint8_t ctrl, uint32_t di, 
                        const uint8_t *data, size_t data_len, net_buf_t *out);

/**
 * @brief 解析 DLT645 响应帧
 * @param in 输入缓冲区
 * @param addr_bcd 输出解析到的地址
 * @param ctrl 输出解析到的控制码
 * @param di 输出解析到的数据标识
 * @param payload_out 输出数据区指针
 * @param payload_len 输出数据区长度
 * @return 0 成功, 1 数据不足, -1 校验错误, -2 格式错误
 */
int dlt645_decode_frame(struct net_buf_simple *in, uint8_t *addr_bcd, uint8_t *ctrl,
                        uint32_t *di, uint8_t **payload_out, size_t *payload_len);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_DLT645_CODEC_H
