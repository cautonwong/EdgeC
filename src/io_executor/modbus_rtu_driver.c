#include <stdio.h>
#include <string.h>
#include <time.h>

#include "contract_data_types.h"
#include "edge_driver.h"

// --- 外部契约接口声明 (由生成的 contracts.c 实现) ---
// 原则：解耦头文件包含，仅保留符号引用
extern edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale);

static int modbus_rtu_init(void *ctx) {
    (void)ctx;
    return 0;
}

static int modbus_rtu_build_req(uint32_t slave_id, edge_point_config_t *point, uint8_t *tx_buf,
                                size_t *tx_len) {
    tx_buf[0] = (uint8_t)slave_id;
    tx_buf[1] = 0x03;
    tx_buf[2] = (uint8_t)(point->reg_addr >> 8);
    tx_buf[3] = (uint8_t)(point->reg_addr & 0xFF);
    tx_buf[4] = (uint8_t)(point->reg_count >> 8);
    tx_buf[5] = (uint8_t)(point->reg_count & 0xFF);
    *tx_len = 6;
    return 0;
}

static int modbus_rtu_parse_res(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                                PipeMsg *out_msg) {
    if (rx_len < 5 || rx_buf[1] != 0x03) return -1;

    // 动态查表：链接器将负责寻找对应产品的 edge_lookup_modbus 实现
    float contract_scale = 1.0f;
    edge_tag_id_t tag_id = edge_lookup_modbus((uint16_t)point->reg_addr, &contract_scale);

    if (tag_id == 0) {  // TAG_INVALID 映射为 0
        return -1;
    }

    uint16_t raw_val = (uint16_t)((rx_buf[3] << 8) | rx_buf[4]);

    out_msg->tag_id = tag_id;
    out_msg->type = PIPEMSG_SCALAR_F64;
    out_msg->payload.val.f = (double)raw_val * (double)contract_scale;
    out_msg->ts = (uint64_t)time(NULL) * 1000;

    return 0;
}

static void modbus_rtu_cleanup(void *ctx) {
    (void)ctx;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU_V2",
                                   .init = modbus_rtu_init,
                                   .build_request = modbus_rtu_build_req,
                                   .parse_response = modbus_rtu_parse_res,
                                   .cleanup = modbus_rtu_cleanup};
