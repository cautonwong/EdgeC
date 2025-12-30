#include <stdio.h>
#include <string.h>
#include <time.h>

#include "contract_data_types.h"
#include "edge_driver.h"

extern edge_tag_id_t edge_lookup_iec104(int ioa);

static int iec104_parse_response(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                                 PipeMsg *out_msg) {
    (void)rx_buf;
    (void)rx_len;

    // IEC104 特性：基于 IOA (Information Object Address) 查表
    edge_tag_id_t tag_id = edge_lookup_iec104((int)point->reg_addr);

    if (tag_id == 0) return -1;

    out_msg->tag_id = tag_id;
    out_msg->type = PIPEMSG_SCALAR_F64;
    out_msg->payload.val.f = 50.01;  // 示例频率
    out_msg->ts = (uint64_t)time(NULL) * 1000;

    return 0;
}

edge_driver_t iec104_relay_driver = {.name = "IEC104_NariRelay",
                                     .parse_response = iec104_parse_response};
