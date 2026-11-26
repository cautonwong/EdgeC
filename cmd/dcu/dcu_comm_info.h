/**
 * @file dcu_comm_info.h
 * @brief Class 25/26 通信参数的本地只读来源
 *
 * 产品没有 APN/ICCID/IMSI/SIM 号码/信号强度的配置或存储；平台能给的只有本机
 * 第一个物理单播以太网口的 MAC。读取结果是快照，由对象模型生命周期持有（init
 * 读取，reset 清空），resolver 按它编码；没有来源的属性不伪造。
 */

#ifndef DCU_COMM_INFO_H
#define DCU_COMM_INFO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    bool mac_present;
    uint8_t mac[6];
} dcu_comm_info_t;

/** 扫描 net_root（sysfs class/net 布局）选平台物理以太网口 MAC；测试传夹具根。永远返回 0。 */
int dcu_comm_info_scan(const char *net_root, dcu_comm_info_t *out);

/** 读取平台第一个物理单播以太网口的 MAC；没有符合判据的接口时 mac_present=false。永远返回 0。 */
int dcu_comm_info_read(dcu_comm_info_t *out);

/** 装上快照；供 init/测试/未来的平台配置生产者使用。 */
void dcu_comm_info_install(const dcu_comm_info_t *info);

void dcu_comm_info_reset(void);

/**
 * 编码 Class 25/26 属性。
 * 返回 EDGE_SLAVE_SRC_*：有来源的属性成功，其余类定义属性 CLASS_MISMATCH（DAR 5），
 * 类未定义 UNDEFINED（DAR 4），非 Class 25/26 的 OI NOT_FOUND（DAR 6）。
 */
int dcu_comm_info_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);

#endif
