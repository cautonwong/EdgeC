#include <stdio.h>
#include <string.h>
#include <time.h>

#include "contract_data_types.h"
#include "edge_driver.h"

// 外部契约接口 (由各产品生成的 contracts.c 实现)
extern edge_tag_id_t edge_lookup_dlms(const char *obis, float *out_scale);

static int dlms_parse_response(edge_point_config_t *point, uint8_t *rx_buf, size_t rx_len,
                               PipeMsg *out_msg) {
    (void)rx_buf;
    (void)rx_len;

    // DLMS 特性：基于 OBIS 字符串查表
    float contract_scale = 1.0f;
    // 假设 point->reg_addr 存储的是 OBIS 的某种索引或直接转换
    // 此处演示核心逻辑：通过 OBIS 字符串获取语义
    edge_tag_id_t tag_id = edge_lookup_dlms("1.0.32.7.0.255", &contract_scale);

    if (tag_id == 0) return -1;

    out_msg->tag_id = tag_id;
    out_msg->type = PIPEMSG_SCALAR_F64;
    out_msg->payload.val.f = 220.5 * (double)contract_scale;
    out_msg->ts = (uint64_t)time(NULL) * 1000;

    return 0;
}

edge_driver_t dlms_standard_driver = {.name = "DLMS_SmartMeter",
                                      .parse_response = dlms_parse_response};
