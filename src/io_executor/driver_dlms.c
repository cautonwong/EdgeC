#include <string.h>
#include <time.h>

#include "dlms_codec.h"
#include "dlms_cosem.h"
#include "dlms_hdlc.h"
#include "driver_manager.h"
#include "edge_common.h"
#include "edge_driver.h"
#include "edge_log.h"

typedef struct {
    dlms_hdlc_manager_t hdlc;
    dlms_cosem_ctx_t cosem;
    bool association_pending;
    uint32_t current_tag_id;
} dlms_driver_ctx_t;

static int dlms_v3_on_init(edge_driver_ctx_t *ctx) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    priv->hdlc.client_addr = 0x01;
    priv->hdlc.server_addr = 0x03;
    dlms_hdlc_reset(&priv->hdlc);
    dlms_cosem_reset(&priv->cosem);
    return 0;
}

static int dlms_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids, int count,
                                  net_buf_t *out) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    if (priv->hdlc.state == HDLC_STATE_DISCONNECTED) {
        dlms_hdlc_build_snrm(&priv->hdlc, out);
        return 0;
    }
    (void)tag_ids;
    (void)count;
    return 0;
}

static int dlms_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, pipe_msg_t *out_msgs,
                                   int *msg_count, size_t *consumed) {
    dlms_driver_ctx_t *priv = (dlms_driver_ctx_t *)ctx->priv;
    *consumed = in->b.len;
    (void)priv;
    (void)out_msgs;
    (void)msg_count;
    return 0;
}

static int dlms_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                             pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)ctx;
    (void)resp_out;
    (void)async_msgs;
    (void)async_count;
    *consumed = in->b.len;
    return 0;
}

edge_driver_t dlms_driver = {.name = "DLMS",
                             .ctx_size = sizeof(dlms_driver_ctx_t),
                             .on_init = dlms_v3_on_init,
                             .encode_request = dlms_v3_encode_request,
                             .decode_response = dlms_v3_decode_response,
                             .handle_rx = dlms_v3_handle_rx};

int dlms_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &dlms_driver);
}
