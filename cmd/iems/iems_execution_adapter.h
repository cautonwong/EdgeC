#ifndef IEMS_EXECUTION_ADAPTER_H
#define IEMS_EXECUTION_ADAPTER_H

#include "iems_node.h"

#ifdef __cplusplus
extern "C" {
#endif

iems_execution_port_t iems_execution_port_create(void *ctx);

/* 控制下发的点位翻译：tag → 原生标识 + 原始值 + 点位描述。纯函数，便于单测
 * （执行口的 write_tag 没有完成回调，端到端测试观测不到成败）。
 * 协议决定原生标识（Modbus 地址+个数、CAN ID、DLT645 DI），传 NULL 时按 tag 原样。
 * DLT645 写必须带 2/4 字节宽度描述；点表缺失或宽度非法返回 -1，调用方在提交前拒绝。 */
int iems_exec_translate_write(const char *proto, uint32_t tag_id, double value,
                              uint32_t *native_id, double *raw, const void **point_desc,
                              size_t *point_desc_len);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_EXECUTION_ADAPTER_H */
