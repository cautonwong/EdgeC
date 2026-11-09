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

/**
 * @brief 时间标签：本实现不使用，按标准填 0（含义是"没有时间标签"）。
 *
 * 标准 6.3 与附录 H 的**每一个** APDU 末尾都有这个字节。漏掉它不会让报文
 * "明显坏掉"，而是让服务器把最后一个数据字节当成时间标签 —— 表现为
 * "类型不匹配 / 数据错"，查起来像编码错误。所以编 APDU 一律显式写它。
 */
#define DLT698_NO_TIME_TAG 0x00

/** 响应里"没有跟随上报信息"。 */
#define DLT698_NO_FOLLOW_REPORT 0x00

// 服务标识 (Service Tags)
#define DLT698_SVC_GET_REQ      0x05
#define DLT698_SVC_GET_RES      0x85
#define DLT698_SVC_SET_REQ      0x06
#define DLT698_SVC_SET_RES      0x86
#define DLT698_SVC_ACTION_REQ   0x07
#define DLT698_SVC_ACTION_RES   0x87
#define DLT698_SVC_REPORT_RES   0x88

/* GET-Request / GET-Response 的 CHOICE 标签（标准 6.3.8 表64 / 表70）。
 * [1] Normal 读一个对象属性；[2] NormalList 一次读若干个对象属性。
 * 注意：[3]/[4] Record 是"读记录型对象属性"（冻结量/事件，带选择条件），
 * 与"一次读多个 OAD"不是一回事，别用错服务。 */
#define DLT698_CHOICE_GET_NORMAL      0x01
#define DLT698_CHOICE_GET_NORMAL_LIST 0x02
#define DLT698_GET_RESULT_DATA        0x01
/** 一次列表读最多几个 OAD（防御：不让请求里的 count 撑爆栈上暂存）。 */
#define DLT698_GET_LIST_MAX           8

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
 * @brief 构造 SET-Request.Normal
 *
 * APDU：`06 | 01 | PIID | OAD(4) | Data`，Data 是 A-XDR 编码的**带类型标签**的值。
 * 类型与宽度由调用方决定（类型是点位模型的知识），驱动只负责拼帧与校验。
 *
 * @param data  A-XDR 数据段（首字节是类型标签），dlen 为 0 表示无数据段
 */
int dlt698_encode_set_request(const dlt698_addr_t *sa, uint8_t piid, uint32_t oad,
                              const uint8_t *data, size_t dlen, net_buf_t *out);

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
 * @brief GET-Response-NormalList 的一个结果项（标准 6.3.8.2.2 表73）。
 *
 * `A-ResultNormal = SEQUENCE { OAD, Get-Result }`，Get-Result 是 CHOICE：
 * `[0] DAR` 或 `[1] Data`。**读失败的项也要如实回 DAR**，不能悄悄少返一项 ——
 * 主站请求 2 个 OAD 只拿回 1 个读数还判通过，是最典型的假绿。
 */
typedef struct {
    uint32_t oad;
    uint8_t dar;          /* 0 = 成功；非 0 时按标准 6.3.3.11 填错误信息 */
    const uint8_t *data;  /* dar==0 时用，必须含 A-XDR 类型标签 */
    size_t dlen;
} dlt698_get_item_t;

/**
 * @brief 构造 GET-Response-NormalList：
 * `85 | 02 | PIID-ACD | n | (OAD(4) | 01 | Data) | ... | 00 | 00`
 * @param items n 个结果，n ∈ [1, DLT698_GET_LIST_MAX]
 */
int dlt698_encode_get_response_list(const dlt698_addr_t *sa, uint8_t piid,
                                    const dlt698_get_item_t *items, size_t n, net_buf_t *out);

/**
 * @brief 构造 ACTION-Request.Normal（操作一个对象方法）。
 *
 * APDU：`07 | 01 | PIID | OMD(4) | Data | 时间标签`
 * 其中 OMD = `OI(2) | 方法标识(1) | 操作模式(1)`（标准 6.3.3.12，模式缺省 0）。
 *
 * 对照标准附录 H.5.1「执行电能量复位方法」：
 * `07 01 05 00 10 01 00 0F 00 00` —— OI=0x0010、方法=1、模式=0、
 * 参数 Data=`0F 00`（integer 0）、末字节 `00` 是没有时间标签。
 *
 * @param oi      对象标识（2 字节，大端）
 * @param method  方法标识（1…255）
 * @param mode    操作模式，标准里缺省 0
 * @param data    方法参数的 A-XDR 编码（含类型标签），dlen=0 表示无参数
 * @param out     输出缓冲区
 * @return 0 成功，<0 参数非法
 */
int dlt698_encode_action_request(const dlt698_addr_t *sa, uint8_t piid, uint16_t oi,
                                 uint8_t method, uint8_t mode, const uint8_t *data, size_t dlen,
                                 net_buf_t *out);

/**
 * @brief 构造 ACTION-Response.Normal（对一次方法操作的应答）。
 *
 * APDU：`87 | 01 | PIID-ACD | OMD(4) | DAR | Data OPTIONAL | FollowReport | 时间标签`
 *
 * 对照标准附录 H.5.1 的响应：
 * `87 01 05 00 10 01 00 00 00 00 00` —— DAR=0（成功）、Data OPTIONAL=0（无返回数据）、
 * FollowReport=0、时间标签=0。
 *
 * **DAR 必须如实填**：操作失败却回 0（成功）就是"假成功"，主站会以为继电器动了。
 * 常见值：0 成功 / 1 硬件失效 / 2 暂时失效 / 3 拒绝读写 / 15 密码错未授权 / 255 其它。
 *
 * @param dar   数据访问结果（标准 6.3.3.11）
 * @param data  操作返回数据的 A-XDR 编码，可为 NULL
 */
int dlt698_encode_action_response(const dlt698_addr_t *sa, uint8_t piid, uint16_t oi,
                                  uint8_t method, uint8_t mode, uint8_t dar, const uint8_t *data,
                                  size_t dlen, net_buf_t *out);

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

/**
 * @brief 同上，但把 APDU 的 CHOICE 标签一并输出（[1] Normal / [2] NormalList）。
 *
 * 从站要按 choice 分派，靠"payload 长度像不像列表"去猜是不行的：
 * 猜错时帧校验全过，只有值是错的。
 */
int dlt698_decode_frame_ex(struct net_buf_simple *in, dlt698_addr_t *sa, uint8_t *svc,
                           uint8_t *choice, uint8_t *piid, uint8_t **payload_out,
                           size_t *payload_len);

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
