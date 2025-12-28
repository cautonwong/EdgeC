#include <stdio.h>
#include <string.h>

#include "edge_driver.h"

// CRC16 查找表或计算函数 (Modbus 专用)
static uint16_t crc16(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

static int modbus_build_request(uint32_t slave_id, edge_point_config_t *point, uint8_t *tx_buf,
                                size_t *tx_len) {
    tx_buf[0] = (uint8_t)slave_id;
    tx_buf[1] = 0x03;  // Read Holding Registers
    tx_buf[2] = (uint8_t)(point->reg_addr >> 8);
    tx_buf[3] = (uint8_t)(point->reg_addr & 0xFF);
    tx_buf[4] = (uint8_t)(point->reg_count >> 8);
    tx_buf[5] = (uint8_t)(point->reg_count & 0xFF);

    uint16_t crc = crc16(tx_buf, 6);
    tx_buf[6] = (uint8_t)(crc & 0xFF);
    tx_buf[7] = (uint8_t)(crc >> 8);

    *tx_len = 8;
    return 0;
}

static int modbus_parse_response(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                                 PipeMsg *out_msg) {
    // 最小长度：Addr(1) + FC(1) + ByteCnt(1) + Data(2*N) + CRC(2)
    if (rx_len < 5) return -1;

    // 校验 FC 是否正确
    if (rx_buf[1] != 0x03) return -1;

    uint8_t byte_cnt = rx_buf[2];
    if (rx_len < (size_t)(byte_cnt + 5)) return -1;

    // 简单解析第一个寄存器作为示例 (Float64 输出)
    uint16_t val = (uint16_t)((rx_buf[3] << 8) | rx_buf[4]);
    out_msg->type = PIPEMSG_SCALAR_F64;
    out_msg->payload.val.f = (double)val * point->scale + point->offset;

    return 0;
}

// 导出驱动实例
edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU",
                                   .init = NULL,
                                   .build_request = modbus_build_request,
                                   .parse_response = modbus_parse_response,
                                   .cleanup = NULL};
