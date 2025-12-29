#include <stdio.h>
#include <string.h>

#include "edge_driver.h"

static int modbus_rtu_init(void *ctx) {
    (void)ctx;
    return 0;
}

static int modbus_rtu_build_req(uint32_t slave_id, edge_point_config_t *point, uint8_t *tx_buf,
                                size_t *tx_len) {
    tx_buf[0] = (uint8_t)slave_id;
    tx_buf[1] = 0x03;  // Read holding registers
    tx_buf[2] = (uint8_t)(point->reg_addr >> 8);
    tx_buf[3] = (uint8_t)(point->reg_addr & 0xFF);
    tx_buf[4] = (uint8_t)(point->reg_count >> 8);
    tx_buf[5] = (uint8_t)(point->reg_count & 0xFF);
    // CRC ignored for dummy
    *tx_len = 6;
    return 0;
}

static int modbus_rtu_parse_res(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                                PipeMsg *out_msg) {
    (void)rx_buf;
    (void)rx_len;
    out_msg->tag_id = point->tag_id;
    out_msg->type = PIPEMSG_SCALAR_F64;
    out_msg->payload.val.f = 123.45 * point->scale + point->offset;
    return 0;
}

static void modbus_rtu_cleanup(void *ctx) {
    (void)ctx;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU",
                                   .init = modbus_rtu_init,
                                   .build_request = modbus_rtu_build_req,
                                   .parse_response = modbus_rtu_parse_res,
                                   .cleanup = modbus_rtu_cleanup};
