#include <stdio.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"

static int can_build_request(uint32_t slave_id, edge_point_config_t *point, uint8_t *tx_buf,
                             size_t *tx_len) {
    (void)slave_id;
    (void)point;
    (void)tx_buf;
    (void)tx_len;
    return 0;
}

static int can_parse_response(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                              PipeMsg *out_msg) {
    (void)point;
    (void)rx_buf;
    (void)rx_len;
    (void)out_msg;
    return 0;
}

static edge_driver_t can_driver = {.name = "CAN",
                                   .init = NULL,
                                   .build_request = can_build_request,
                                   .parse_response = can_parse_response,
                                   .cleanup = NULL};

void driver_can_init(void) {
    driver_manager_register(&can_driver);
}
