#include "edge_driver.h"

static int dlms_v3_on_init(edge_driver_ctx_t *ctx) {
    (void)ctx;
    return 0;
}

edge_driver_t dlms_driver = {.name = "DLMS",
                             .ctx_size = 0,
                             .on_init = dlms_v3_on_init,
                             .encode_request = NULL,
                             .decode_response = NULL,
                             .handle_rx = NULL,
                             .on_cleanup = NULL};
