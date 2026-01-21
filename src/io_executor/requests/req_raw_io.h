#ifndef REQ_RAW_IO_H
#define REQ_RAW_IO_H

#include "edge_request.h"

// Define the specific callback signature for this raw I/O scenario
typedef void (*raw_io_on_complete_cb)(int status, net_buf_t *rx_buf);

/**
 * @brief Creates a request object for a raw data passthrough scenario.
 * 
 * @param tx_buf The buffer containing the raw data to be sent.
 * @param user_cb The callback to invoke with the raw response buffer.
 * @return A fully constructed edge_req_t object.
 */
edge_req_t* req_raw_io_create(net_buf_t *tx_buf, raw_io_on_complete_cb user_cb);


#endif
