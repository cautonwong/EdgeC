#ifndef DCU_698_OBJECT_INTERNAL_H
#define DCU_698_OBJECT_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "dcu_698_object_model.h"

/* 一帧最多带 8 个配置单元；更多时按 DAR 7 拒绝，不静默截断。 */
#define DCU_ARCHIVE_BATCH_MAX 8

typedef struct {
    uint8_t method;           /* 127–134 */
    uint16_t config_seq;      /* 129/130/131 的配置序号 */
    uint16_t unit_count;      /* 127/128/129/130/132 的单元数 */
    bool has_annex;           /* 附属信息非空：产品没有存储路径 */
    dcu_archive_unit_t units[DCU_ARCHIVE_BATCH_MAX];
} dcu_archive_op_t;

/* 类族处理器只由 dcu_698_object_model.c 的对象表与快照分派引用。 */
int dcu_698_acq_6000_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result);
int dcu_698_decode_archive_op(uint8_t method, const uint8_t *buf, size_t len,
                              dcu_archive_op_t *out);
int dcu_698_accept_archive_action(uint32_t omd, const uint8_t *axdr, size_t axdr_len);
int dcu_698_decode_task_op(uint8_t method, const uint8_t *buf, size_t len, dcu_task_op_t *out);

/* 对象表检查的唯一来源：0 = 可分派且 *obj_out 已填；否则回 DAR 6/5/4。 */
int dcu_698_check_call(uint32_t omd, const dcu_698_object_t **obj_out);
int dcu_698_archive_encode_axdr(const dcu_inventory_t *inv, uint32_t oad, uint8_t *buf, size_t cap,
                                size_t *len);
int dcu_698_acq_6012_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result);
int dcu_698_acq_6002_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val);
int dcu_698_acq_6002_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result);

/**
 * @brief 编码 Class 24 已暴露属性的 A-XDR。
 *
 * A2/A3 输出 array/structure；A4/A11/A12 是同路径的标量属性。A5 未暴露：
 * 表A.4 按 OI 要求不同字段结构，`dcu_event_obj_t` 未存储。
 * 返回 0 或 `EDGE_SLAVE_SRC_*`；buffer 不足返回 TYPE_MISMATCH。
 */
int dcu_698_event_encode_axdr(const dcu_698_object_t *obj, uint32_t oad, uint8_t *buf, size_t cap,
                              size_t *len);
int dcu_698_event_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                       void *out_val);
int dcu_698_event_write(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                        const void *in_val);
int dcu_698_event_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                          void *result);

int dcu_698_control_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result);

/** @brief 演示/组合根私有的拉合闸入口；不是 OI 8000 的标准方法。 */
int dcu_698_control_legacy_action(uint32_t slot, uint8_t action);

int dcu_698_security_f100_execute(const dcu_698_object_t *obj, uint8_t method_id,
                                  const void *param, void *result);

const router_sink_ops_t *dcu_698_internal_sink_ops(void);
dcu_inventory_t *dcu_698_internal_inventory(void);
void dcu_698_internal_control_sink(dcu_control_sink_t *out_fn, void **out_ctx);

#endif /* DCU_698_OBJECT_INTERNAL_H */
