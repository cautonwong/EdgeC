#ifndef EDGE_DLT645_CODEC_H
#define EDGE_DLT645_CODEC_H

#include <stdint.h>

#include "net_buf.h"

/**
 * @brief DLT645 报文控制码定义
 */
#define DLT645_CTRL_READ_DATA 0x11
#define DLT645_CTRL_READ_RESPONSE 0x91
#define DLT645_CTRL_ERROR_RESPONSE 0xD1

/**
 * @brief DLT645 纯协议内核接口 (Zero-Dependency)
 */

/* --- 基础工具 --- */
uint8_t dlt645_calculate_cs(const uint8_t *buf, size_t len);
double dlt645_bcd_to_f64(const uint8_t *bcd, int bytes);
void dlt645_u64_to_bcd_addr(uint64_t addr, uint8_t *out_bcd);

/* --- 编解码逻辑 --- */

/**
 * @brief 构造读取请求帧
 * @param addr_bcd 6字节地址
 * @param di 数据标识 (内部自动执行 +0x33 处理)
 * @param out 输出 buffer
 * @return 0 成功
 */
int dlt645_encode_read_request(const uint8_t *addr_bcd, uint32_t di, net_buf_t *out);

/**
 * @brief 解析响应帧
 * @param in 输入 buffer (内部自动执行 -0x33 处理)
 * @param out_di 解析出的数据标识
 * @param out_val 解析出的 BCD 原始数值 (需要外部结合 scale 处理)
 * @return 0 成功, 1 数据不足, <0 校验失败或非法格式
 */
int dlt645_decode_response(struct net_buf_simple *in, uint32_t *out_di, double *out_val);

#endif  // EDGE_DLT645_CODEC_H
