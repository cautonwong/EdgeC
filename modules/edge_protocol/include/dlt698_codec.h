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

/* --- A-XDR 基础数据类型编解码 (TDD Task 1) --- */

/**
 * @brief DLT698.45 抽象数据类型标识 (Data Tags)
 */
typedef enum {
    DLT698_TAG_NULL = 0,
    DLT698_TAG_ARRAY = 1,
    DLT698_TAG_STRUCTURE = 2,
    DLT698_TAG_DOUBLE_LONG = 5,
    DLT698_TAG_DOUBLE_LONG_UNSIGNED = 6,
    DLT698_TAG_OCTET_STRING = 9,
    DLT698_TAG_VISIBLE_STRING = 10,
    DLT698_TAG_INTEGER = 15,
    DLT698_TAG_LONG = 16,
    DLT698_TAG_UNSIGNED = 17,
    DLT698_TAG_LONG_UNSIGNED = 18,
    DLT698_TAG_LONG64 = 20,
    DLT698_TAG_LONG64_UNSIGNED = 21,
    DLT698_TAG_ENUM = 22,
    DLT698_TAG_FLOAT32 = 23,
    DLT698_TAG_FLOAT64 = 24,
    DLT698_TAG_DATE_TIME_S = 28
} dlt698_data_tag_t;

#define MAX_DLT698_STRING_LEN 64

/**
 * @brief 抽象数据容器 (用于递归解析)
 */
typedef struct dlt698_data_s {
    dlt698_data_tag_t tag;
    union {
        uint8_t u8;
        int8_t i8;
        uint16_t u16;
        int16_t i16;
        uint32_t u32;
        int32_t i32;
        uint64_t u64;
        int64_t i64;
        float f32;
        double f64;
        struct {
            uint8_t *ptr;
            size_t len;
            uint8_t buf[MAX_DLT698_STRING_LEN];
        } str;
        struct {
            struct dlt698_data_s *items;
            uint16_t count;
        } container;
    } val;
} dlt698_data_t;

/**
 * @brief 通用 A-XDR 递归解码器
 */
int dlt698_decode_data(const uint8_t *buf, uint16_t len, uint16_t *offset, dlt698_data_t *res);
void dlt698_free_data(dlt698_data_t *data);

int dlt698_encode_long_unsigned(uint8_t *buf, uint16_t len, uint16_t *offset, uint16_t val);
int dlt698_decode_long_unsigned(const uint8_t *buf, uint16_t len, uint16_t *offset, uint16_t *val);

int dlt698_encode_double_long(uint8_t *buf, uint16_t len, uint16_t *offset, int32_t val);
int dlt698_decode_double_long(const uint8_t *buf, uint16_t len, uint16_t *offset, int32_t *val);

int dlt698_encode_date_time_s(uint8_t *buf, uint16_t len, uint16_t *offset, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec);
int dlt698_decode_date_time_s(const uint8_t *buf, uint16_t len, uint16_t *offset, uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *min, uint8_t *sec);

int dlt698_encode_get_response(const dlt698_addr_t *sa, uint8_t piid, uint32_t oad, const uint8_t *data, size_t dlen, net_buf_t *out);

#ifdef __cplusplus
}
#endif

#endif
