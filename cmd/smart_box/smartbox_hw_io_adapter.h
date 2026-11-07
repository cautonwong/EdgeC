/**
 * @file smartbox_hw_io_adapter.h
 * @brief SmartBox 板载硬件适配器（GPIO / 看门狗 / AXP209 / SOS）
 */

#ifndef SMARTBOX_HW_IO_ADAPTER_H
#define SMARTBOX_HW_IO_ADAPTER_H

#include <uv.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 GPIO 节点与看门狗，并在 loop 上注册硬件轮询与 SOS 事件。
 *
 * 看门狗不存在时静默降级，便于本地非 root 调试。
 */
int smartbox_hw_io_start(uv_loop_t *loop, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif  // SMARTBOX_HW_IO_ADAPTER_H
