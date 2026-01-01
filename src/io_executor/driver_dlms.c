#include <stdio.h>
#include <string.h>

#include "contract_data_types.h"
#include "driver_manager.h"
#include "edge_driver.h"

static int dlms_parse_response(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                               struct pipe_msg_s *out_msg) {
    (void)point;
    (void)rx_buf;
    (void)rx_len;
    (void)out_msg;
    return 0;
}

static int dlms_build_request(uint32_t slave_id, edge_point_config_t *point, uint8_t *tx_buf,
                              size_t *tx_len) {
    (void)slave_id;
    (void)point;
    (void)tx_buf;
    (void)tx_len;
    return 0;
}

static edge_driver_t dlms_driver = {.name = "DLMS_V1",
                                    .init = NULL,
                                    .build_request = dlms_build_request,
                                    .parse_response = dlms_parse_response,
                                    .cleanup = NULL};

int driver_dlms_init(void) {
    return driver_manager_register(&dlms_driver);
}