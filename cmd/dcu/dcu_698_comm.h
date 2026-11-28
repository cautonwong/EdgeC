/**
 * @file dcu_698_comm.h
 * @brief Class 25/26 通信参数（4500..4501 无线公网、4510..4517 以太网）纯软件配置领域引擎
 */

#ifndef DCU_698_COMM_H
#define DCU_698_COMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DCU_COMM_WIRELESS_OI_FIRST 0x4500u
#define DCU_COMM_WIRELESS_OI_LAST 0x4501u
#define DCU_COMM_WIRELESS_COUNT 2u

#define DCU_COMM_ETH_OI_FIRST 0x4510u
#define DCU_COMM_ETH_OI_LAST 0x4517u
#define DCU_COMM_ETH_COUNT 8u

#define DCU_COMM_APN_MAX 32u
#define DCU_COMM_USER_MAX 32u
#define DCU_COMM_PASS_MAX 32u

typedef enum {
    DCU_ETH_MODE_DHCP = 0,
    DCU_ETH_MODE_STATIC = 1,
    DCU_ETH_MODE_PPPOE = 2
} dcu_eth_ip_mode_t;

typedef struct {
    char apn[DCU_COMM_APN_MAX];
    char user[DCU_COMM_USER_MAX];
    char password[DCU_COMM_PASS_MAX];
    uint8_t master_ip[4];
    uint16_t master_port;
    uint8_t backup_ip[4];
    uint16_t backup_port;
    uint16_t heartbeat_s;
} dcu_comm_wireless_cfg_t;

typedef struct {
    uint8_t ip_mode; /* 0 = DHCP, 1 = 静态, 2 = PPPoE */
    uint8_t ip[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
    uint8_t master_ip[4];
    uint16_t master_port;
} dcu_comm_eth_cfg_t;

/** @brief 可选持久化端口：按 (oi, attr) 存取 A-XDR 载荷；模块不感知底层存储实现。 */
typedef struct {
    void *ctx;
    int (*save)(void *ctx, uint16_t oi, uint8_t attr, const uint8_t *buf, size_t len);
    int (*load)(void *ctx, uint16_t oi, uint8_t attr, uint8_t *buf, size_t cap, size_t *len);
} dcu_comm_store_t;

/** @brief 重置全部 4500..4501 与 4510..4517 配置为默认值；store 非空时从持久化端口加载已存配置。 */
void dcu_698_comm_init(const dcu_comm_store_t *store);

/** @brief 判断 OI 是否属于 Class 25 (4500..4501) 或 Class 26 (4510..4517)。 */
bool dcu_698_comm_is_oi(uint16_t oi);

/** @brief 写入无线公网配置（4500..4501）；主站端口为 0 拒绝；save 失败时回滚旧值。 */
int dcu_698_comm_set_wireless(uint16_t oi, const dcu_comm_wireless_cfg_t *cfg);

/** @brief 读取无线公网配置（4500..4501）。 */
int dcu_698_comm_get_wireless(uint16_t oi, dcu_comm_wireless_cfg_t *out);

/** @brief 写入以太网配置（4510..4517）；主站端口为 0 或模式非法拒绝；save 失败时回滚旧值。 */
int dcu_698_comm_set_eth(uint16_t oi, const dcu_comm_eth_cfg_t *cfg);

/** @brief 读取以太网配置（4510..4517）。 */
int dcu_698_comm_get_eth(uint16_t oi, dcu_comm_eth_cfg_t *out);

/**
 * @brief 编码 Class 25/26 属性为 A-XDR Data。
 *
 * 缓冲区 `cap` 不足时返回 -1；未知 OI 返回 `EDGE_SLAVE_SRC_NOT_FOUND`；
 * 类定义但未实现属性返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`；未定义属性返回 `EDGE_SLAVE_SRC_UNDEFINED`。
 */
int dcu_698_comm_encode_attr(uint16_t oi, uint8_t attr_id, uint8_t *buf, size_t cap,
                             size_t *out_len);

/**
 * @brief 严格解码 Class 25/26 属性的 A-XDR SET 载荷并提交。
 *
 * 错 tag / 截断 / 尾随字节 / 越界端口返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`（DAR 7）；
 * 持久化 save 失败时回滚内存配置并返回 `EDGE_SLAVE_SRC_TEMPORARY`（DAR 2）。
 */
int dcu_698_comm_decode_set(uint16_t oi, uint8_t attr_id, const uint8_t *axdr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_COMM_H */
