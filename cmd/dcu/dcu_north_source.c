/**
 * @file dcu_north_source.c
 * @brief DCU 北向取数端口的实现（ADR-0011 族 A / N2-3）
 *
 * 北向驱动只做 payload 编码，两样东西必须由产品给：
 *   1. **值**——`peek(tag_id)`，从 datahub 影子表取（产品读自己的点位模型是合法的）
 *   2. **名字**——`name(tag_id)`，metric 名。驱动不认识 698 OAD，也不该去反查元数据
 *
 * 这两条就是 ADR-002 删掉的 `read_shadow` / `get_tag_name` 的正确替代：
 * 端口**每次调用现传**，不进 `edge_driver_ctx_t`。
 */

#include "dcu_north_source.h"

#include <stddef.h>

#include "datahub.h"
#include "dcu_698_var.h"
#include "edge_log.h"
#include "mqtt_json_codec.h"

/** 从影子表取一条待发送消息。ctx 是 `datahub_t*`。 */
static int _dcu_north_peek(void *ctx, uint32_t tag_id, pipe_msg_t *out) {
    datahub_t *hub = (datahub_t *)ctx;
    if (!hub || !out) return -1;
    return datahub_read_shadow(hub, tag_id, out);
}

/** 影子 tag → metric 名。名字随变量点表定义，这里只转发。 */
static const char *_dcu_north_name(void *ctx, uint32_t tag_id) {
    (void)ctx;
    return dcu_698_var_metric_name(tag_id);
}

/**
 * @brief metric 名 → OAD（下行要用：云端只给名字）。
 *
 * 与 `_dcu_north_name` 互为反函数。**只覆盖北向推送的实时点位**——
 * 控制类 OI（8000）不在这个表里，因为它的下行语义是 ACTION 而不是写属性。
 */
static int _dcu_north_resolve_name(void *ctx, const char *name, uint32_t *tag_id) {
    (void)ctx;
    return dcu_698_var_metric_resolve(name, tag_id);
}

/**
 * @brief 收一条北向下行报文。
 *
 * ## 现在能做什么、不能做什么
 *
 * 能：把 `{"p":"...","v":...}` 解出来，确认字段合法（证明链路和解析都通）。
 * 不能：**执行**。DCU 的下行控制是 698 的 `ACTION.request`（OI 8000）；A.9 参数
 * 已能解码，但云端下行没有 OAD→继电器/电表地址的映射，猜一个"看起来对"的
 * 执行路径比不执行更危险 —— 拉合闸是要动物理继电器的。
 *
 * 所以这里返回 -1（明确拒绝），编排层会记日志。等继电器映射落地后，
 * 在变量点表之上补一张 OAD → 聚合根方法的表，再返回 0。
 *
 * 为什么解析放这里而不是 `northbound_mgr`：见 `edge_north_plane.h` 的分层纪律。
 */
static int _dcu_north_on_downlink(void *ctx, const char *address, const uint8_t *payload,
                                  size_t len) {
    (void)ctx;

    char name[64];
    double value = 0.0;
    if (mqtt_json_decode_downlink(payload, len, name, sizeof(name), &value) != 0) {
        LOG_WARN(LOG_CAT_NB,
                 "DCU downlink on '%s': 不是 {p,v} 帧（698 ACTION 待报文样例）", address);
        return -1;
    }

    LOG_WARN(LOG_CAT_NB,
             "DCU downlink on '%s': 解析出 %s=%.6g，但**未执行** —— "
             "缺 OAD→聚合根方法映射（待 698 ACTION 语义确认）",
             address, name, value);
    return -1;
}

const edge_north_source_ops_t *dcu_north_source_ops(void) {
    static const edge_north_source_ops_t ops = {
        .peek = _dcu_north_peek,
        .name = _dcu_north_name,
        .resolve_name = _dcu_north_resolve_name,
        .accept = NULL, /* 见 _dcu_north_on_downlink 的说明：执行路径待 698 样例 */
        .on_downlink = _dcu_north_on_downlink,
    };
    return &ops;
}
