#ifndef IEMS_EXECUTION_ADAPTER_H
#define IEMS_EXECUTION_ADAPTER_H

#include "iems_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 执行适配器组合根配置：ctx 指向本结构，生命周期须覆盖端口；NULL = 正常模式。 */
typedef struct {
    bool shadow_mode;
} iems_execution_config_t;

iems_execution_port_t iems_execution_port_create(void *ctx);

/* 总线 cmd_status_t → 域完成状态（成功/失败/超时）。纯函数，便于单测覆盖所有总线状态：
 * CMD_STATUS_SUCCESS → OK；CMD_STATUS_TIMEOUT → TIMEOUT；其余非零（IO/PROTO/QUEUE_FULL/
 * RATE_LIMITED）→ FAILED。 */
iems_write_done_status_t iems_exec_map_write_status(int cmd_status);

/* 控制下发的点位翻译：tag → 原生标识 + 原始值 + 点位描述。纯函数，便于单测。
 * 协议决定原生标识（Modbus 地址+个数、CAN ID、DLT645 DI），传 NULL 时按 tag 原样。
 * DLT645 写必须带 2/4 字节宽度描述；点表缺失或宽度非法返回 -1，调用方在提交前拒绝。 */
int iems_exec_translate_write(const char *proto, uint32_t tag_id, double value,
                              uint32_t *native_id, double *raw, const void **point_desc,
                              size_t *point_desc_len);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_EXECUTION_ADAPTER_H */
