/**
 * @file iec_asdu.h
 * @brief IEC 60870-5-101/104 应用服务数据单元 (ASDU) 归一化定义
 */

#ifndef EDGE_IEC_ASDU_H
#define EDGE_IEC_ASDU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IEC 101/104 类型标识 (TypeID) 枚举
 * 涵盖了电力系统最常用的监控、控制及系统信息类型
 */
typedef enum {
    /* --- 监控方向过程信息 (Monitoring) --- */
    IEC_TYPE_M_SP_NA_1 = 1,  /**< 单点信息 (遙信) */
    IEC_TYPE_M_DP_NA_1 = 3,  /**< 双点信息 */
    IEC_TYPE_M_ST_NA_1 = 5,  /**< 步位置信息 (档位) */
    IEC_TYPE_M_ME_NA_1 = 9,  /**< 测量值, 规一化值 (遙測) */
    IEC_TYPE_M_ME_NB_1 = 11, /**< 测量值, 标度值 */
    IEC_TYPE_M_ME_NC_1 = 13, /**< 测量值, 短浮点数 */
    IEC_TYPE_M_IT_NA_1 = 15, /**< 累计量 (遙脈) */

    /* --- 带长时标的监控信息 (Time Tagged CP56Time2a) --- */
    IEC_TYPE_M_SP_TB_1 = 30, /**< 带时标 CP56Time2a 的单点信息 */
    IEC_TYPE_M_DP_TB_1 = 31, /**< 带时标 CP56Time2a 的双点信息 */
    IEC_TYPE_M_ME_TF_1 = 36, /**< 带时标 CP56Time2a 的短浮点数测量值 */

    /* --- 控制方向过程信息 (Control) --- */
    IEC_TYPE_C_SC_NA_1 = 45, /**< 单命令 (遙控) */
    IEC_TYPE_C_DC_NA_1 = 46, /**< 双命令 */
    IEC_TYPE_C_RC_NA_1 = 47, /**< 升降命令 */
    IEC_TYPE_C_SE_NC_1 = 50, /**< 设点命令, 短浮点数 (遙调) */

    /* --- 系统信息 --- */
    IEC_TYPE_C_IC_NA_1 = 100, /**< 总召唤命令 (GI) */
    IEC_TYPE_C_CS_NA_1 = 103, /**< 时钟同步命令 */
    IEC_TYPE_C_RP_NA_1 = 105, /**< 复位进程命令 */
} iec_type_id_t;

/**
 * @brief 传输原因 (COT) 定义
 */
typedef enum {
    IEC_COT_NOT_USED = 0,
    IEC_COT_PER_CYC = 1,   /**< 周期/循环 */
    IEC_COT_BACK = 2,      /**< 背景扫描 */
    IEC_COT_SPONT = 3,     /**< 突发 (自发) */
    IEC_COT_INIT = 4,      /**< 初始化完成 */
    IEC_COT_REQ = 5,       /**< 请求或查询 */
    IEC_COT_ACT = 6,       /**< 激活 */
    IEC_COT_ACT_CON = 7,   /**< 激活确认 */
    IEC_COT_DEACT = 8,     /**< 停止激活 */
    IEC_COT_DEACT_CON = 9, /**< 停止激活确认 */
    IEC_COT_ACT_TERM = 10, /**< 激活终止 */
    IEC_COT_INTROGEN = 20, /**< 响应站召唤 (总传) */
} iec_cot_t;

/**
 * @brief 质量描述词 (QDS)
 */
typedef struct {
    uint8_t iv : 1;  /**< 无效 (Invalid) */
    uint8_t nt : 1;  /**< 非当前值 (Not Topical) */
    uint8_t sb : 1;  /**< 取代 (Substituted) */
    uint8_t bl : 1;  /**< 封锁 (Blocked) */
    uint8_t res : 3; /**< 预留 */
    uint8_t ov : 1;  /**< 溢出 (Overflow) */
} iec_qds_t;

/**
 * @brief 点位标识编码（ADR-002）
 *
 * IEC 101/104 组 ASDU 需要「类型标识 + 信息对象地址」两个协议原生概念，
 * 而 native_id 只有一个 32 位数，所以按位拼起来：
 *
 *     native_id = (type_id << 24) | (ioa & 0xFFFFFF)
 *
 * IOA 是 3 字节、类型标识 1 字节，正好 32 位不丢信息。
 * **产品侧与驱动侧必须用同一套宏**，否则点位对不上——所以放在公开头里，
 * 而不是各自在 .c 里定义一份。
 */
#define IEC_NATIVE_ID(type_id, ioa) \
    ((((uint32_t)(type_id) & 0xFFu) << 24) | ((uint32_t)(ioa) & 0xFFFFFFu))
#define IEC_NATIVE_TYPE(id) ((uint8_t)(((id) >> 24) & 0xFFu))
#define IEC_NATIVE_IOA(id) ((uint32_t)((id) & 0xFFFFFFu))

/**
 * @brief 写（遥控/遥调）请求的点位标识约定
 *
 * 控制点位的 `type_id` 是**命令类型**，不是测量类型：
 *   - `IEC_TYPE_C_SC_NA_1` (45) 单点遥控
 *   - `IEC_TYPE_C_DC_NA_1` (46) 双点遥控
 *   - `IEC_TYPE_C_SE_NC_1` (50) 设定值（短浮点遥调）
 *
 * 产品组 native_id 时用命令类型：
 *
 *     IEC_NATIVE_ID(IEC_TYPE_C_SC_NA_1, ioa)
 *
 * 驱动在 `req_prepare` 里只接受这三种类型；拿到测量类型就拒绝——
 * 用一个测量 ASDU 去"写"是协议错误，不是"没实现"。
 */
#define IEC_IS_COMMAND_TYPE(t) \
    ((t) == IEC_TYPE_C_SC_NA_1 || (t) == IEC_TYPE_C_DC_NA_1 || (t) == IEC_TYPE_C_SE_NC_1)

/**
 * @brief 电力规约地址长度配置 (101/104 归一化关键)
 */
typedef struct {
    uint8_t cot_len;   /**< 传输原因字节数: 1 (101) or 2 (104) */
    uint8_t casdu_len; /**< 公共地址字节数: 1 (101) or 2 (101/104) */
    uint8_t ioa_len;   /**< 信息体地址字节数: 1, 2 (101) or 3 (104) */
} iec_addr_config_t;

/**
 * @brief 默认 104 配置 (对齐标准)
 */
static const iec_addr_config_t IEC_ADDR_CONF_104 = {.cot_len = 2, .casdu_len = 2, .ioa_len = 3};

/* --- ASDU Codec APIs --- */

void iec_asdu_init(net_buf_t *buf, iec_type_id_t type, bool sq, uint8_t count, iec_cot_t cot,
                   uint16_t casdu, const iec_addr_config_t *conf);

int iec_asdu_parse_header(struct net_buf_simple *b, iec_type_id_t *type, bool *sq, uint8_t *count,
                          iec_cot_t *cot, uint16_t *casdu, const iec_addr_config_t *conf);

/* M_SP_NA_1 (单点遙信) */

int iec_asdu_add_m_sp_na_1(net_buf_t *buf, uint32_t ioa, bool val, iec_qds_t qds,

                           bool sq, bool is_first, const iec_addr_config_t *conf);

int iec_asdu_parse_m_sp_na_1(struct net_buf_simple *b, uint32_t *ioa, bool *val, iec_qds_t *qds,

                             bool sq, bool is_first, const iec_addr_config_t *conf);

/* M_ME_NC_1 (短浮点数遙測) */

int iec_asdu_add_m_me_nc_1(net_buf_t *buf, uint32_t ioa, float val, iec_qds_t qds, bool sq,
                           bool is_first, const iec_addr_config_t *conf);
int iec_asdu_parse_m_me_nc_1(struct net_buf_simple *b, uint32_t *ioa, float *val, iec_qds_t *qds,
                             bool sq, bool is_first, const iec_addr_config_t *conf);

/* C_IC_NA_1 (总召唤) */
int iec_asdu_add_c_ic_na_1(net_buf_t *buf, uint32_t ioa, uint8_t qoi,
                           const iec_addr_config_t *conf);

/* C_SC_NA_1 (单命令) */
int iec_asdu_add_c_sc_na_1(net_buf_t *buf, uint32_t ioa, bool val, bool select,
                           const iec_addr_config_t *conf);

/* C_DC_NA_1 (双命令) */
int iec_asdu_add_c_dc_na_1(net_buf_t *buf, uint32_t ioa, uint8_t dcs, bool select,
                           const iec_addr_config_t *conf);

int iec_asdu_parse_c_sc_na_1(struct net_buf_simple *b, uint32_t *ioa, uint8_t *scs,
                             const iec_addr_config_t *conf);

int iec_asdu_parse_c_se_nc_1(struct net_buf_simple *b, uint32_t *ioa, float *val, uint8_t *qos,
                             const iec_addr_config_t *conf);

/* C_SE_NC_1 (设点命令, 短浮点数) */
int iec_asdu_add_c_se_nc_1(net_buf_t *buf, uint32_t ioa, float val, uint8_t qos, bool select,
                           const iec_addr_config_t *conf);

int iec_asdu_parse_c_sc_na_1(struct net_buf_simple *b, uint32_t *ioa, uint8_t *scs, const iec_addr_config_t *conf);

/**
 * @brief 契约驱动的万能数据填充函数 (支持时标自动注入)
 */
int iec_asdu_add_any(net_buf_t *buf, uint8_t type_id, uint32_t ioa, double val, uint64_t ts_ms,
                     uint32_t flags, iec_qds_t qds, const iec_addr_config_t *conf);

#ifdef __cplusplus
}
#endif

#endif  // EDGE_IEC_ASDU_H