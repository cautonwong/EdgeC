#ifndef IEMS_EXECUTION_ADAPTER_H
#define IEMS_EXECUTION_ADAPTER_H

#include "iems_node.h"

#ifdef __cplusplus
extern "C" {
#endif

iems_execution_port_t iems_execution_port_create(void *ctx);

/* 控制下发的点位翻译：tag → 原生标识 + 原始值。纯函数，便于单测
 * （执行口的 write_tag 没有完成回调，端到端测试观测不到成败）。 */
/* 协议决定原生标识（Modbus 地址+个数、CAN ID、DLT645 DI），传 NULL 时按 tag 原样。 */
void iems_exec_translate_write(const char *proto, uint32_t tag_id, double value,
                               uint32_t *native_id, double *raw);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_EXECUTION_ADAPTER_H */
