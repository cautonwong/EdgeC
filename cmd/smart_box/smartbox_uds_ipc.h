/**
 * @file smartbox_uds_ipc.h
 * @brief SmartBox → Go 网关 UDS 二进制帧发送端口
 */

#ifndef SMARTBOX_UDS_IPC_H
#define SMARTBOX_UDS_IPC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 发送一帧 UDS 报文（连接失败只告警，不影响主循环）。
 */
void smartbox_uds_send(uint8_t msg_type, const uint8_t *payload, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif  // SMARTBOX_UDS_IPC_H
