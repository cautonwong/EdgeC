/**
 * @file driver_can.c
 * @brief 工业级 CAN 规约驱动 —— 纯 codec（ADR-002）
 *
 * 与 645 的差别：645 的点位标识天然是 DI，CAN 的点位标识是 **CAN ID**，
 * 与产品的 tag_id 不是一回事。所以翻译由调用方（产品点位模型）做完，
 * 驱动只收 can_id，只回原始 float。
 *
 * 这个文件里没有 lookup_by_tag / lookup_by_key / read_shadow / tag_id / scaler：
 *   - 不知道值属于哪个点位 → 回原始值，标识由请求层贴；
 *   - 不知道量纲 → 不乘 scaler，标定由调用方给（edge_api_read_tag_ex 的 scale）。
 */

#include <stdint.h>
#include <string.h>

#include "can_codec.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "net_buf.h"

#define CAN_RTR_FLAG 0x40000000U
#define CAN_ID_MASK 0x1FFFFFFFU
#define CAN_MAX_DLC 8

/** 请求私有数据：驱动认识的全部信息就是"哪一帧" */
typedef struct {
    uint32_t can_id;           /**< 原生点位标识（CAN ID，不含标志位） */
    uint8_t dlc;               /**< 数据长度；请求阶段为 0（RTR） */
    float raw;                 /**< 解码出的原始值 */
    uint8_t data[CAN_MAX_DLC]; /**< 解码出的原始字节，留给上层按 DBC 再解释 */
} can_req_t;

static int can_req_prepare(void *priv, uint32_t slave_id, uint32_t point_id, const void *meta_list,
                              uint32_t meta_list_len, const void *point_desc,
                              size_t point_desc_len) {
    (void)point_desc;
    (void)point_desc_len;
    (void)slave_id;
    (void)meta_list;
    (void)meta_list_len;
    can_req_t *p = (can_req_t *)priv;
    if (!p) return -1;
    p->can_id = point_id & CAN_ID_MASK;
    p->dlc = 0;
    p->raw = 0.0f;
    memset(p->data, 0, sizeof(p->data));
    return 0;
}

static int can_req_encode(void *priv, void *session, net_buf_t *out) {
    (void)session; /* 无会话状态：这一帧自成一体 */
    can_req_t *p = (can_req_t *)priv;
    if (!p || !out) return -1;
    /* 读请求 = 远程请求帧（RTR），不带数据 */
    edge_can_frame_t frame = {
        .can_id = p->can_id | CAN_RTR_FLAG,
        .can_dlc = 0,
    };
    return can_encode_frame(&frame, out);
}

static int can_req_decode(void *priv, void *session, const uint8_t *buf, size_t len, size_t *consumed,
                          double *out_value) {
    (void)session; /* 无会话状态：这一帧自成一体 */
    can_req_t *p = (can_req_t *)priv;
    if (!p || !buf || !consumed) return -1;
    *consumed = 0;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, (void *)buf, len);

    edge_can_frame_t frame;
    if (can_decode_frame(&nb, &frame) != 0) return EDGE_ERR_INCOMPLETE;

    /* can_decode_frame 消费固定 16 字节的归一化帧 */
    *consumed = 16;

    /* 不是本请求要的那一帧（总线上的其它报文）：不认，让调用方继续找 */
    if ((frame.can_id & CAN_ID_MASK) != p->can_id) return EDGE_ERR_INCOMPLETE;

    p->dlc = frame.can_dlc > CAN_MAX_DLC ? CAN_MAX_DLC : frame.can_dlc;
    memcpy(p->data, frame.data, p->dlc);
    /* 原始 float 在前 4 字节；具体含义（电压/电流/温度）由上层按 DBC 解释 */
    memcpy(&p->raw, frame.data, 4);
    if (out_value) *out_value = (double)p->raw;
    return 0;
}

const edge_driver_t can_driver = {
    .name = "CAN",
    .ctx_size = 0, /* request-native：驱动不需要 per-session 上下文 */
    .req_priv_size = sizeof(can_req_t),
    .req_prepare = can_req_prepare,
    .req_encode = can_req_encode,
    .req_decode = can_req_decode,
};

int can_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &can_driver);
}
