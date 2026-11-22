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
 * @brief 携带长度的原始 ACTION 入口（D1-20a 的 rebase 调用这一行）。
 *
 * 先查对象表（DAR 6/5/4 的单一口径），再严格解码 6012 的 A-XDR 参数
 * （截断/错标签/错个数/尾随字节 → DAR 7），最后把类型化操作交给处理器。
 * 形状错误在触碰 poller/SQL 之前返回。
 */
int dcu_698_accept_action_axdr(uint32_t omd, const uint8_t *axdr, size_t axdr_len);

/** @brief 6012 A2 数组 / 6013 A2 单元（元素索引 = task_id，0 = 最小现有 ID）。 */
int dcu_698_task_encode_axdr(const dcu_poller_t *poller, uint32_t oad, uint8_t *buf, size_t cap,
                             size_t *len);

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
