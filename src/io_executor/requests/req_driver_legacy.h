#ifndef REQ_DRIVER_LEGACY_H
#define REQ_DRIVER_LEGACY_H

#include "driver_manager.h"
#include "edge_request.h"

// Forward declare the original user callback for scalar values
typedef void (*legacy_on_complete_cb)(int status, const pipe_msg_t *result);

/**
 * @brief Creates a request object that bridges the new request model
 *        to the old driver-based implementation.
 *
 * @param dm The driver manager instance.
 * @param protocol The name of the protocol to use (e.g., "ModbusRTU").
 * @param tag_id The physical tag ID for the request.
 * @param user_cb The final user callback to invoke with the result.
 * @return A fully constructed edge_req_t object.
 */
edge_req_t *req_legacy_driver_create(driver_manager_t *dm, const char *protocol, uint32_t slave_id,
                                     uint32_t tag_id, void *lookup_by_tag, void *lookup_by_key,
                                     legacy_on_complete_cb user_cb);

#endif
