#include <stdio.h>
#include <string.h>
#include <time.h>

#include "contract_data_types.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "shmring.h"

extern edge_tag_id_t edge_lookup_modbus(uint16_t reg, float *out_scale);

static int modbus_rtu_init(void *ctx) {
    (void)ctx;
    return 0;
}

static int modbus_rtu_build_batch_req(edge_poll_group_t *group, uint8_t *tx_buf, size_t *tx_len) {
    if (!group || !tx_buf) return -1;
    tx_buf[0] = (uint8_t)group->slave_id;
    tx_buf[1] = 0x03;
    tx_buf[2] = (uint8_t)(group->start_addr >> 8);
    tx_buf[3] = (uint8_t)(group->start_addr & 0xFF);
    tx_buf[4] = (uint8_t)(group->count >> 8);
    tx_buf[5] = (uint8_t)(group->count & 0xFF);
    *tx_len = 6;
    return 0;
}

static int modbus_rtu_parse_batch_res(edge_poll_group_t *group, uint8_t *rx_buf, size_t rx_len,
                                      void (*on_msg)(pipe_msg_t *msg, void *user_data),
                                      void *user_data) {
    if (rx_len < 3 || rx_buf[1] != 0x03) return -1;
    uint64_t now = (uint64_t)time(NULL) * 1000;

    for (uint16_t i = 0; i < group->count; i++) {
        uint16_t current_reg = group->start_addr + i;
        float scale = 1.0f;
        edge_tag_id_t tid = edge_lookup_modbus(current_reg, &scale);
        if (tid == 0) continue;

        uint16_t raw_val = (uint16_t)((rx_buf[3 + i * 2] << 8) | rx_buf[4 + i * 2]);
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, tid, (double)raw_val * (double)scale, now);
        on_msg(&msg, user_data);
    }
    return 0;
}

static int modbus_rtu_build_req(uint32_t slave_id, edge_point_config_t *point, uint8_t *tx_buf,
                                size_t *tx_len) {
    edge_poll_group_t g = {(uint16_t)point->reg_addr, point->reg_count, 0x03, slave_id};
    return modbus_rtu_build_batch_req(&g, tx_buf, tx_len);
}

static int modbus_rtu_parse_res(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                                pipe_msg_t *out_msg) {
    if (rx_len < 5) return -1;
    float scale = 1.0f;
    edge_tag_id_t tid = edge_lookup_modbus((uint16_t)point->reg_addr, &scale);
    if (tid == 0) return -1;
    pipe_msg_init_scalar(out_msg, tid, (double)((rx_buf[3] << 8) | rx_buf[4]) * (double)scale,
                         (uint64_t)time(NULL) * 1000);
    return 0;
}

static void modbus_rtu_cleanup(void *ctx) {
    (void)ctx;
}

edge_driver_t modbus_rtu_driver = {.name = "ModbusRTU_V2",
                                   .init = modbus_rtu_init,
                                   .build_batch_req = modbus_rtu_build_batch_req,
                                   .parse_batch_res = modbus_rtu_parse_batch_res,
                                   .build_request = modbus_rtu_build_req,
                                   .parse_response = modbus_rtu_parse_res,
                                   .cleanup = modbus_rtu_cleanup};
