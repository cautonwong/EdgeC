#include "edge_driver.h"

const edge_driver_t can_driver = {.name = "CAN",
                                  .ctx_size = 0,
                                  .on_init = NULL,
                                  .encode_request = NULL,
                                  .decode_response = NULL,
                                  .handle_rx = NULL,
                                  .on_cleanup = NULL};