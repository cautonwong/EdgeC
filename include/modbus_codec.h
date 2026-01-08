#ifndef EDGE_MODBUS_CODEC_H
#define EDGE_MODBUS_CODEC_H

#include <stdint.h>

#include "net_buf.h"

/**
 * @brief Modbus 纯协议内核接口 (Zero-Dependency)
 */

/* --- CRC16 Modbus --- */
uint16_t modbus_calculate_crc(const uint8_t *buf, size_t len);

/* --- 封包逻辑 --- */

/**
 * @brief 构造读保持寄存器请求 (03)
 */
int modbus_encode_read_holding(uint8_t slave_id, uint16_t addr, uint16_t quantity, net_buf_t *out);

/**
 * @brief 构造单寄存器写入响应 (06)
 */
int modbus_encode_write_single_resp(uint8_t slave_id, uint16_t addr, uint16_t value,
                                    net_buf_t *out);

/* --- 解析逻辑 --- */

/**
 * @brief 验证并解析响应帧头
 * @return 0 成功, 1 数据不足, <0 校验失败或功能码错误
 */
int modbus_decode_rtu_header(struct net_buf_simple *in, uint8_t *slave_id, uint8_t *func_code);

#endif  // EDGE_MODBUS_CODEC_H
