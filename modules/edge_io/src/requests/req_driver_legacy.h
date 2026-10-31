#ifndef REQ_DRIVER_LEGACY_H
#define REQ_DRIVER_LEGACY_H

#include "arbiter.h"
#include "driver_manager.h"
#include "edge_request.h"

// Forward declare the original user callback for scalar values
typedef void (*legacy_on_complete_cb)(int status, const pipe_msg_t *result, void *user_data);

/**
 * @brief Creates a request object that bridges the new request model
 *        to the old driver-based implementation.
 *
 * @param dm The driver manager instance.
 * @param protocol The name of the protocol to use (e.g., "ModbusRTU").
 * @param action The command action (Read/Write).
 * @param tag_id The physical tag ID for the request.
 * @param value The value to write (for Write action).
 * @param user_cb The final user callback to invoke with the result.
 * @param user_data Context passed back to user_cb.
 * @return A fully constructed edge_req_t object.
 */
/**
 * @param native_id     驱动认识的协议原生点位标识（645 的 DI / CAN 的 CAN ID / 寄存器地址）
 * @param result_tag_id 回调里 result->tag_id 用的标识（调用方自己的 tag_id）
 * @param scale         解码值 × scale 后交给回调；<=0 视为 1.0
 */
edge_req_t *req_legacy_driver_create(driver_manager_t *dm, const char *protocol, uint32_t slave_id,
                                     cmd_action_t action, uint32_t native_id, uint32_t result_tag_id,
                                     double scale, double value, const void *meta_list,
                                     uint32_t meta_list_len, const void *point_desc,
                                     size_t point_desc_len, legacy_on_complete_cb user_cb,
                                     void *user_data);

#endif
