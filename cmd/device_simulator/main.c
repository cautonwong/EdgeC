#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "edge_log.h"
#include "net_buf.h"
#include "contracts.h"
#include "edge_common.h"
#include "dlms_cosem.h"
#include "dlms_hdlc.h"

static dlms_hdlc_manager_t g_hdlc_mgr;

int handle_dlms_request(net_buf_t *in, net_buf_t *resp_out) {
    struct net_buf_simple *b = &in->b;
    if (b->len < 7) return -1;
    
    uint8_t control = b->data[5];
    if (control == 0x83) { // SNRM
        struct net_buf_simple *rb = &resp_out->b;
        net_buf_simple_add_u8(rb, 0x7E);
        net_buf_simple_add_be16(rb, 0xA007);
        net_buf_simple_add_u8(rb, 0x03); net_buf_simple_add_u8(rb, 0x01);
        net_buf_simple_add_u8(rb, 0x63); // UA
        uint16_t fcs = edge_crc16_ccitt(rb->data + 1, 5);
        net_buf_simple_add_le16(rb, fcs); net_buf_simple_add_u8(rb, 0x7E);
        return 0;
    }

    net_buf_t *apdu = NULL;
    int ret = dlms_hdlc_depacketize(&g_hdlc_mgr, b, in->pool, &apdu);
    if (ret != 0 || !apdu) return (ret == 1) ? 0 : -1;

    net_buf_t *resp_apdu = net_buf_alloc(in->pool, 0);
    uint8_t tag = apdu->b.data[0];
    
    if (tag == (uint8_t)DLMS_SERVICE_AARQ) {
        LOG_INFO(LOG_CAT_CORE, "Simulator: Received AARQ, building PROPER AARE...");
        dlms_encoder_t enc; dlms_encoder_init(&enc, &resp_apdu->b);
        net_buf_simple_add_u8(enc.buf, (uint8_t)DLMS_SERVICE_AARE);
        dlms_encode_begin(&enc, DLMS_TYPE_STRUCTURE);
        dlms_encode_u8(&enc, 0); 
        dlms_encode_end(&enc);
    } else if (tag == (uint8_t)DLMS_SERVICE_GET_REQUEST) {
        LOG_INFO(LOG_CAT_CORE, "Simulator: Received GET, responding with data...");
        struct net_buf_simple *r = &resp_apdu->b;
        net_buf_simple_add_u8(r, (uint8_t)DLMS_SERVICE_GET_RESPONSE);
        net_buf_simple_add_u8(r, 0x01); // Normal
        net_buf_simple_add_u8(r, 0x00); // Success
        dlms_encoder_t enc; dlms_encoder_init(&enc, r);
        dlms_encode_u16(&enc, 2305); // 230.5V
    }

    dlms_hdlc_packetize(&g_hdlc_mgr, resp_apdu, 0, 128, resp_out);
    net_buf_unref(resp_apdu);
    net_buf_unref(apdu);
    return 0;
}

static void on_alloc(uv_handle_t *h, size_t suggested, uv_buf_t *buf) {
    (void)h; buf->base = malloc(suggested); buf->len = (unsigned int)suggested;
}
static void _on_write_free_cb(uv_write_t *req, int status) {
    (void)status;
    free(req->data);
    free(req);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {

    net_buf_pool_t *pool = (net_buf_pool_t *)stream->data;
    if (nread > 0) {
        net_buf_t *req = net_buf_alloc(pool, 0);
        net_buf_t *resp = net_buf_alloc(pool, 0);
        memcpy(req->b.data, buf->base, (size_t)nread); req->b.len = (uint16_t)nread;
        if (req->b.data[0] == 0x7E) handle_dlms_request(req, resp);
        if (resp->b.len > 0) {
            uv_write_t *wr = malloc(sizeof(uv_write_t));
            char *copy = malloc(resp->b.len); memcpy(copy, resp->b.data, resp->b.len);
            uv_buf_t w_buf = uv_buf_init(copy, resp->b.len);
            wr->data = copy; uv_write(wr, stream, &w_buf, 1, _on_write_free_cb);
        }
        net_buf_unref(req); net_buf_unref(resp);
    }
    if (buf->base) free(buf->base);
}
static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    client->data = server->data;
    uv_accept(server, (uv_stream_t *)client);
    uv_read_start((uv_stream_t *)client, on_alloc, on_read);
}

int main(int argc, char *argv[]) {
    edge_log_init(NULL, false);
    g_hdlc_mgr.server_addr = 0x03; g_hdlc_mgr.client_addr = 0x01;
    const char *path = "/tmp/edge_sim.sock";
    for (int i = 1; i < argc; i++) { if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) path = argv[++i]; }
    uv_loop_t *loop = uv_default_loop(); net_buf_pool_t pool;
    net_buf_pool_init(&pool, 16, 4096, 0, NULL, NULL);
    unlink(path); uv_pipe_t *server = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, server, 0); server->data = &pool;
    uv_pipe_bind(server, path); uv_listen((uv_stream_t *)server, 128, on_new_connection);
    LOG_INFO(LOG_CAT_CORE, "Universal Simulator started on %s", path);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}