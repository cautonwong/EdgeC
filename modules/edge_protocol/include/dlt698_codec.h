#ifndef EDGE_DLT698_CODEC_H
#define EDGE_DLT698_CODEC_H

#include <stdint.h>
#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DLT698.45 (面向对象规约) 基础常量
 */
#define DLT698_HEAD 0x68
#define DLT698_END  0x16

// 服务标识 (Service Tags)
#define DLT698_SVC_GET_REQ      0x05
#define DLT698_SVC_GET_RES      0x85
#define DLT698_SVC_SET_REQ      0x06
#define DLT698_SVC_SET_RES      0x86
#define DLT698_SVC_ACTION_REQ   0x07
#define DLT698_SVC_ACTION_RES   0x87
#define DLT698_SVC_REPORT_RES   0x88

/**
 * @brief DLT698 地址类型
 */
typedef struct {
    uint8_t addr[16];
    uint8_t len;
    uint8_t type; // 0: Single, 1: Group, 2: Broadcast
} dlt698_addr_t;

/* --- 校验工具 --- */
uint16_t dlt698_calculate_fcs(const uint8_t *p, size_t len);

/* --- 编解码逻辑 --- */

/**
 * @brief 构造 GET.Request.Normal
 * @param sa 从站地址
 * @param piid 优先级与服务序列号
 * @param oad 对象属性描述符 (4字节)
 * @param out 输出缓冲区
 */
int dlt698_encode_get_request(const dlt698_addr_t *sa, uint8_t piid, uint32_t oad, net_buf_t *out);

/**
 * @brief 构造 GET-Response.Normal 报文并写入 out 缓冲区 (以接地气的中文添加注释)
 * @param sa 从站地址
 * @param piid 服务序列号
 * @param oad 对象属性描述符
 * @param data 响应数据载荷
 * @param dlen 数据字节长度
 * @param out 输出网络缓冲区
 */
int dlt698_encode_get_response(const dlt698_addr_t *sa, uint8_t piid, uint32_t oad, 
                               const uint8_t *data, size_t dlen, net_buf_t *out);

/**
 * @brief 解析响应报文
 * @param in 输入缓冲区
 * @param sa 输出解析到的地址
 * @param svc 输出服务标识
 * @param piid 输出 PIID
 * @param payload_out 指向数据载荷起始
 * @param payload_len 载荷长度
 * @return 0 成功, 1 数据不足, <0 格式/校验错误
 */
int dlt698_decode_frame(struct net_buf_simple *in, dlt698_addr_t *sa, uint8_t *svc, 
                        uint8_t *piid, uint8_t **payload_out, size_t *payload_len);

#ifdef __cplusplus
}
#endif

#endif
