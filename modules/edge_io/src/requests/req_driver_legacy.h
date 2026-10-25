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
edge_req_t *req_legacy_driver_create(driver_manager_t *dm, const char *protocol, uint32_t slave_id,
                                     cmd_action_t action, uint32_t tag_id, double value,
                                     void *lookup_by_tag, void *lookup_by_key,
                                     const void *meta_list, uint32_t meta_list_len,
                                     legacy_on_complete_cb user_cb, void *user_data);

#endif
