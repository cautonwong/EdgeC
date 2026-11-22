/**
 * @file dcu_698_object_model.h
 * @brief DCU 698 对象表：静态表 + 查表门面
 *
 * 设计见 `docs/design/DCU_698_OBJECT_MODEL.md` 4.1 到 4.5。
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

/** 表 A.9 的 OI 8000 方法范围（127…131）。 */
#define DCU_ACTION_METHOD_FIRST 127
#define DCU_ACTION_METHOD_LAST 131

/** 表 A.9 中 8000 方法 129/130/131 的解码结果；127/128 参数为 NULL，不填。 */
typedef struct {
    uint8_t count;           /* 本次请求的继电器项数；执行落地前只保留首项 */
    uint32_t relay_oad;
    uint8_t alarm_delay_min; /* 129: unsigned，分钟 */
    uint16_t limit_minutes;  /* 129: long-unsigned，0 = 永久限电 */
    bool auto_close;         /* 129: bool */
    uint8_t command;         /* 130/131: enum 0=合闸允许 1=直接合闸 */
} dcu_action_param_t;

struct dcu_698_object_s {
    uint16_t oi;
    uint8_t class_id;
    dcu_obj_scope_t scope;
    dcu_698_read_fn read;     /* NULL 表示属性不可读 */
    dcu_698_write_fn write;   /* NULL 表示只读 */
    dcu_698_method_fn method; /* NULL 表示无方法 */
    const void *instance;     /* 聚合根或实体指针；不拥有、不释放 */
    uint8_t attr_mask[32];
    uint8_t method_mask[32];
};

/**
 * @brief 装配对象表的实例指针。表本身是文件作用域静态存储，init 后只读。
 *
 * 可重复调用；每次先把全部 instance 清空再按参数填入。
 */
int dcu_698_object_model_init(dcu_inventory_t *inv, dcu_poller_t *poller,
                              router_sink_ops_t sink_ops, dcu_esam_t *esam,
                              dcu_discovery_t *discovery, dcu_event_obj_t *evt_3000,
                              dcu_event_obj_t *evt_3005, router_bus_ops_t bus_ops);

/** @brief 清空实例指针并回到未初始化状态。 */
void dcu_698_object_model_reset(void);

/** @brief 注入遥控动作下沉端口（转发到 8000 处理器）。 */
void dcu_698_object_model_set_control_sink(dcu_control_sink_t fn, void *ctx);

/** @brief 按 OI 查表；未初始化或不存在返回 NULL。 */
const dcu_698_object_t *dcu_698_find(uint16_t oi);

/** @brief 对象表行数。 */
size_t dcu_698_object_count(void);

/** @brief 取第 index 行；越界返回 NULL。 */
const dcu_698_object_t *dcu_698_object_at(size_t index);

/** @brief 接口类定义（附录 A）是否包含该属性号；属性号从 1 起。 */
bool dcu_698_class_defines_attr(uint8_t class_id, uint8_t attr_id);

/** @brief 类表或该 OI 的附录 A 实例定义是否包含该属性号（6002 的 A5..A10）。 */
bool dcu_698_oi_defines_attr(uint16_t oi, uint8_t class_id, uint8_t attr_id);

/** @brief 取当前注入的影子读取端口（变量类处理器用）；未初始化时为全零。 */
const router_bus_ops_t *dcu_698_object_model_bus_ops(void);

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
