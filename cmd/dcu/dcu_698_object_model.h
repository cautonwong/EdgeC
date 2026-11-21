/**
 * @file dcu_698_object_model.h
 * @brief DCU 698 对象表：静态表 + 查表门面
 *
 * 设计见 `docs/design/DCU_698_OBJECT_MODEL.md` 4.1 到 4.5。
 *
 * D1-03 是重构：处理器签名与现有 `dcu_object_t` 一代保持一致（`void *` 取值/写入），
 * 行为不变。A-XDR 处理器签名（`dcu_axdr_buf_t`）在 D1-04 与 D1-13 迁移。
 */

#ifndef DCU_698_OBJECT_MODEL_H
#define DCU_698_OBJECT_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dcu_router.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 256 位位图：属性号/方法号 0..255。698 的 Class 10 方法到 134，Class 23 属性到 18。 */
static inline bool dcu_mask_test(const uint8_t mask[32], uint8_t id) {
    return (mask[id >> 3] >> (id & 7)) & 1u;
}

static inline void dcu_mask_set(uint8_t mask[32], uint8_t id) {
    mask[id >> 3] |= (uint8_t)(1u << (id & 7));
}

typedef enum {
    DCU_OBJ_SHARED,    /* 全设备一个实例，例如 6000 档案表 */
    DCU_OBJ_PER_METER, /* 每块表一个实例，例如 2000 电压 */
    DCU_OBJ_SINGLETON  /* 全设备唯一，例如 4000 日期时间 */
} dcu_obj_scope_t;

typedef struct dcu_698_object_s dcu_698_object_t;

typedef int (*dcu_698_read_fn)(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                               void *out_val);
typedef int (*dcu_698_write_fn)(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                                const void *in_val);
typedef int (*dcu_698_method_fn)(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                                 void *result);

struct dcu_698_object_s {
    uint16_t oi;
    uint8_t class_id;
    dcu_obj_scope_t scope;
    dcu_698_read_fn read;     /* NULL 表示属性不可读 */
    dcu_698_write_fn write;   /* NULL 表示只读 */
    dcu_698_method_fn method; /* NULL 表示无方法 */
    const void *instance;     /* 聚合根或实体指针；不拥有、不释放 */
    uint8_t attr_mask[32];    /* 支持的属性号位图 0..255 */
    uint8_t method_mask[32];  /* 支持的方法号位图 0..255 */
};

/**
 * @brief 装配对象表的实例指针。表本身是文件作用域静态存储，init 后只读。
 *
 * 只填 instance 与端口上下文；不再逐个 `obj->oi = ...` 注册。
 * 可重复调用；每次先把全部 instance 清空再按参数填入。
 */
int dcu_698_object_model_init(dcu_inventory_t *inv, dcu_poller_t *poller,
                              router_sink_ops_t sink_ops, dcu_esam_t *esam,
                              dcu_discovery_t *discovery, dcu_event_obj_t *evt_3000,
                              dcu_event_obj_t *evt_3005, router_bus_ops_t bus_ops);

/** @brief 清空实例指针并回到未初始化状态（旧 `dcu_router_destroy` 语义）。 */
void dcu_698_object_model_reset(void);

/** @brief 注入遥控动作下沉端口（转发到 8000 处理器）。 */
void dcu_698_object_model_set_control_sink(dcu_control_sink_t fn, void *ctx);

/** @brief 按 OI 查表；未初始化或不存在返回 NULL。 */
const dcu_698_object_t *dcu_698_find(uint16_t oi);

/**
 * @brief 按 OAD 读/写属性、按 OMD 调方法。
 *
 * 缺 OI、缺处理器、或属性/方法位不在 mask 内一律返回 -1。
 * 命中处理器后，处理器自己的返回码（-2/-3/-4 等）原样上抛。
 */
int dcu_698_read(uint32_t oad, void *out_val);
int dcu_698_write(uint32_t oad, const void *in_val);
int dcu_698_call(uint32_t omd, const void *param, void *result);

/** @brief Class 7 事件对象按 Attr 3 抓拍断面（Generic Snapshot Engine）。 */
int dcu_698_snapshot(uint16_t oi, uint32_t device_id, void *out_msgs, int max_count);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_OBJECT_MODEL_H */
