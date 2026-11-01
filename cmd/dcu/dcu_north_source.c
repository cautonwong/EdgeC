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
#include <string.h>

#include "datahub.h"
#include "dcu_router.h"
#include "edge_log.h"
#include "mqtt_json_codec.h"

/* OAD → 北向 metric 名。
 *
 * 只列实时测点（OI 0xxx/2xxx）——那些是北向要持续上报的量。
 * 档案/任务/控制类 OI（6000/6012/8000）走 698 的 GET/ACTION，不经推送通道。
 *
 * 名字风格与 IEMS 的 `edge_tag_meta_t.name` 保持一致（大写蛇形），
 * 这样两台设备上云后字段名不会一个驼峰一个蛇形。 */
static const struct {
    uint32_t oad;
    const char *name;
} s_dcu_north_names[] = {
    {OAD_ACTIVE_ENERGY, "ACTIVE_ENERGY"},
    {OAD_REACTIVE_ENERGY, "REACTIVE_ENERGY"},
    {OAD_VOLTAGE, "VOLTAGE_A"},
    {OAD_VOLTAGE_B, "VOLTAGE_B"},
    {OAD_VOLTAGE_C, "VOLTAGE_C"},
    {OAD_CURRENT, "CURRENT_A"},
    {OAD_CURRENT_B, "CURRENT_B"},
    {OAD_CURRENT_C, "CURRENT_C"},
    {OAD_ZERO_SEQ_CURRENT, "ZERO_SEQ_CURRENT"},
    {OAD_ACTIVE_POWER_TOTAL, "ACTIVE_POWER_TOTAL"},
    {OAD_ACTIVE_POWER_A, "ACTIVE_POWER_A"},
    {OAD_ACTIVE_POWER_B, "ACTIVE_POWER_B"},
    {OAD_ACTIVE_POWER_C, "ACTIVE_POWER_C"},
    {OAD_REACTIVE_POWER_TOTAL, "REACTIVE_POWER_TOTAL"},
    {OAD_POWER_FACTOR_TOTAL, "POWER_FACTOR_TOTAL"},
    {OAD_GRID_FREQUENCY, "GRID_FREQUENCY"},
};

static const size_t s_dcu_north_names_count =
    sizeof(s_dcu_north_names) / sizeof(s_dcu_north_names[0]);

/** 从影子表取一条待发送消息。ctx 是 `datahub_t*`。 */
static int _dcu_north_peek(void *ctx, uint32_t tag_id, pipe_msg_t *out) {
    datahub_t *hub = (datahub_t *)ctx;
    if (!hub || !out) return -1;
    return datahub_read_shadow(hub, tag_id, out);
}

/** OAD → metric 名。找不到返回 NULL，驱动会回落成数字标识。 */
static const char *_dcu_north_name(void *ctx, uint32_t tag_id) {
    (void)ctx;
    for (size_t i = 0; i < s_dcu_north_names_count; i++) {
        if (s_dcu_north_names[i].oad == tag_id) return s_dcu_north_names[i].name;
    }
    return NULL;
}

/**
 * @brief metric 名 → OAD（下行要用：云端只给名字）。
 *
 * 与 `_dcu_north_name` 互为反函数。**只覆盖北向推送的实时点位**——
 * 控制类 OI（8000）不在这个表里，因为它的下行语义是 ACTION 而不是写属性。
 */
static int _dcu_north_resolve_name(void *ctx, const char *name, uint32_t *tag_id) {
    (void)ctx;
    if (!name || !tag_id) return -1;
    for (size_t i = 0; i < s_dcu_north_names_count; i++) {
        if (strcmp(s_dcu_north_names[i].name, name) == 0) {
            *tag_id = s_dcu_north_names[i].oad;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief 收一条北向下行报文。
 *
 * ## 现在能做什么、不能做什么
 *
 * 能：把 `{"p":"...","v":...}` 解出来，确认字段合法（证明链路和解析都通）。
 * 不能：**执行**。DCU 的下行控制是 698 的 `ACTION.request`（OI 8000），
 * 报文语义（方法 ID、操作模式、参数编码）还没有样例，猜一个"看起来对"的
 * 执行路径比不执行更危险 —— 拉合闸是要动物理继电器的。
 *
 * 所以这里返回 -1（明确拒绝），编排层会记日志。等 698 报文样例到位后，
 * 在 `s_dcu_north_names` 之上补一张 OAD → 聚合根方法的表，再返回 0。
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
