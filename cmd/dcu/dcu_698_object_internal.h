#ifndef DCU_698_OBJECT_INTERNAL_H
#define DCU_698_OBJECT_INTERNAL_H

#include <stdint.h>

#include "dcu_698_object_model.h"

/* 类族处理器只由 dcu_698_object_model.c 的对象表与快照分派引用。 */
int dcu_698_acq_6012_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result);
int dcu_698_acq_6002_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val);
int dcu_698_acq_6002_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result);

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
