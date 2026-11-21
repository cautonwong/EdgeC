#ifndef DCU_698_AXDR_H
#define DCU_698_AXDR_H

#include <stddef.h>
#include <stdint.h>

#include "dcu_698_object_model.h"

#ifdef __cplusplus
extern "C" {
#endif

int dcu_north_resolve_axdr(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
int dcu_north_accept_axdr(void *ctx, uint32_t oad, const uint8_t *axdr, size_t len);

/**
 * @brief 解 A.9 的 OI 8000 方法参数。
 *
 * 127/128 要求 NULL；129/130/131 要求 array<structure>，每项字段数与类型必须精确匹配。
 * 形状不符返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`；方法不在 127…131 返回 NOT_FOUND。
 */
int dcu_698_decode_action_param(uint8_t method, const uint8_t *buf, size_t len,
                                dcu_action_param_t *out);

#ifdef __cplusplus
}
#endif

#endif /* DCU_698_AXDR_H */
