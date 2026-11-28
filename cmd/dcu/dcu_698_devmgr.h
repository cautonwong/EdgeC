/**
 * @file dcu_698_devmgr.h
 * @brief Class 19 (0x4300 电气设备) 设备管理软件面领域引擎
 */

#ifndef DCU_698_DEVMGR_H
#define DCU_698_DEVMGR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DCU_DEVMGR_MAX_PROTOCOLS 8u
#define DCU_DEVMGR_PROTOCOL_LEN 32u

/**
 * @brief Class 19 设备管理静态设备信息（零动态内存）。
 */
typedef struct {
    char descriptor[64];
    char vendor_code[5];  /* 4B visible-string */
    char sw_version[5];   /* 4B visible-string */
    char sw_date[7];      /* 6B visible-string */
    char hw_version[5];   /* 4B visible-string */
    char hw_date[7];      /* 6B visible-string */
    char vendor_ext[9];   /* 8B visible-string */
    uint16_t prod_year;   /* date_time_s 年 */
    uint8_t prod_month;   /* date_time_s 月 */
    uint8_t prod_day;     /* date_time_s 日 */
    uint8_t prod_hour;    /* date_time_s 时 */
    uint8_t prod_min;     /* date_time_s 分 */
    uint8_t prod_sec;     /* date_time_s 秒 */
    uint8_t protocol_count;
    char protocols[DCU_DEVMGR_MAX_PROTOCOLS][DCU_DEVMGR_PROTOCOL_LEN];
} dcu_devmgr_info_t;

/**
 * @brief Class 19 软件初始化端口。
 */
typedef struct {
    int (*data_init)(void *ctx);
    int (*event_init)(void *ctx);
    int (*demand_init)(void *ctx);
} dcu_devmgr_ops_t;

/** @brief 获取当前静态设备信息只读视图。 */
const dcu_devmgr_info_t *dcu_698_devmgr_get_info(void);

/** @brief 覆盖静态设备信息（info 为 NULL 时忽略）。 */
void dcu_698_devmgr_set_info(const dcu_devmgr_info_t *info);

/** @brief 重置静态设备信息为出厂默认值。 */
void dcu_698_devmgr_reset_info(void);

/**
 * @brief 编码 Class 19 属性 A2/A3/A4/A5 为 A-XDR 字节流。
 *
 * @param attr_id 属性号（2=设备描述符, 3=版本信息, 4=生产日期, 5=规约列表）
 * @param buf     输出缓冲区
 * @param cap     缓冲区容量
 * @param out_len 输出编码字节数
 * @return 0 成功；-1 参数无效或缓冲不足；EDGE_SLAVE_SRC_UNDEFINED 未知属性
 */
int dcu_698_devmgr_encode_attr(uint8_t attr_id, uint8_t *buf, size_t cap, size_t *out_len);

/**
 * @brief 分派 Class 19 方法调用。
 *
 * M3/M5/M6 转发给 ops 对应回调（未注入返回 EDGE_SLAVE_SRC_CLASS_MISMATCH）；
 * M1/M4 无硬件/平台复位端口时诚实返回 EDGE_SLAVE_SRC_CLASS_MISMATCH。
 */
int dcu_698_devmgr_call(uint8_t method_id, const dcu_devmgr_ops_t *ops, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_DEVMGR_H */
