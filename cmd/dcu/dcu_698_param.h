/**
 * @file dcu_698_param.h
 * @brief Class 8 白名单参数的 A-XDR 编解码、RAM 值表与持久化端口
 *
 * 设计见 `docs/design/DCU_698_OBJECT_MODEL.md` 的 Class 8 一节：属性 2 是标准参数值，
 * 4000 另有 OI 专属的属性 3（时钟模式）与属性 4（精确时间参数）。4000 A2 通过
 * `dcu_clock_port_t` 直接读写系统时钟，其余参数落 `dcu_params`。
 */

#ifndef DCU_698_PARAM_H
#define DCU_698_PARAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dcu_clock_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 产品白名单（标准 7.3.8 / A.5）；对象表与 Class 20 ACL 都从这里派生。 */
#define DCU_698_PARAM_OIS(X) \
    X(0x4000)                \
    X(0x4001)                \
    X(0x4002) X(0x4003) X(0x4004) X(0x401C) X(0x401D) X(0x401E) X(0x401F) X(0x4020) X(0x4021)

/** 持久化端口：产品提供 SQLite 适配，模块不认识存储。 */
typedef struct {
    void *ctx;
    int (*save)(void *ctx, uint16_t oi, uint8_t attr, const uint8_t *buf, size_t len);
    int (*load)(void *ctx, uint16_t oi, uint8_t attr, uint8_t *buf, size_t cap, size_t *len);
} dcu_param_store_t;

/**
 * @brief 复制端口并把已落盘的参数读回 RAM。
 *
 * 未落盘或落盘值解不开时保留默认值；4000 A2 不在 `dcu_params` 里，GET 总读时钟端口。
 */
void dcu_698_param_init(const dcu_param_store_t *store);

/** @brief 注入时钟端口；4000 A2 的 GET/SET 只走这个端口。 */
void dcu_698_param_set_clock(const dcu_clock_port_t *clock);

/** @brief 恢复默认值并清掉存储端口（对象表 reset 用于测试隔离）；时钟端口保留。 */
void dcu_698_param_reset(void);

bool dcu_698_param_is_oi(uint16_t oi);

/** @brief 该 OI/属性是否有参数行（4000 的 2/3/4，其余只有 2）。 */
bool dcu_698_param_has_attr(uint16_t oi, uint8_t attr);

/**
 * @brief 编码一个参数属性为 A-XDR Data。
 *
 * 返回 0 或 `EDGE_SLAVE_SRC_*`（5 类定义但本对象未实现、4 未定义、6 无此 OI、7 缓冲不足）。
 */
int dcu_698_param_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len);

/**
 * @brief 严格解码并提交一次参数 SET。
 *
 * 保存失败返回 `EDGE_SLAVE_SRC_TEMPORARY` 且旧值不变。4000 A2 调时钟端口，失败同样返回
 * `EDGE_SLAVE_SRC_TEMPORARY`，时间保持旧值。
 */
int dcu_698_param_accept_axdr(uint32_t oad, const uint8_t *axdr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_PARAM_H */
