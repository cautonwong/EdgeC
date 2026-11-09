/**
 * @file northbound_driver_adapter.h
 * @brief 北向协议驱动的装配适配器（组合根侧，组件不做对象构造）
 */

#ifndef NORTHBOUND_DRIVER_ADAPTER_H
#define NORTHBOUND_DRIVER_ADAPTER_H

#include "northbound_mgr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 按管理器当前的协议名建好驱动并交给它。
 *
 * 必须在 northbound_mgr_start() 之前调用。所有权转移给管理器，
 * northbound_mgr_destroy() 会一并释放驱动管理器与驱动实例。
 */
int northbound_attach_default_driver(northbound_mgr_t *mgr);

#ifdef __cplusplus
}
#endif

#endif  // NORTHBOUND_DRIVER_ADAPTER_H
