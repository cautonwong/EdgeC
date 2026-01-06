#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uv.h>

#include "contracts.h"
#include "edge_common.h"
#include "edge_log.h"
#include "net_buf.h"
#include "protocols/edge_dlms.h"

static edge_dlms_context_t g_dlms_ctx;

// [高质量资源字典]：模拟真实电表数据
static edge_error_t _on_get_voltage(const edge_dlms_object_t *obj, edge_dlms_variant_t *val,
                                    void *user) {
    (void)obj;
    (void)user;
    static uint16_t voltage = 2305;  // 230.5V
    val->tag = DLMS_TAG_LONG_UNSIGNED;
    val->length = 2;
    val->data = (uint8_t *)&voltage;  // 注意：此处演示简化，实际应处理字节序
    return EP_OK;
}

static edge_dlms_resource_t g_resources[] = {
    {{3, {1, 0, 1, 8, 0, 255}, 2}, _on_get_voltage, NULL, NULL}};

int handle_dlms_request(net_buf_t *in, net_buf_t *resp_out) {
    struct net_buf_simple *b = &in->b;
    if (b->len < 7) return -1;

    // 1. 处理 SNRM (物理层握手仍保留在胶水层)
    uint8_t control = b->data[5];
    if (control == 0x83) {
        struct net_buf_simple *rb = &resp_out->b;
        net_buf_simple_add_u8(rb, 0x7E);
        net_buf_simple_add_be16(rb, 0xA007);
        net_buf_simple_add_u8(rb, 0x03);
        net_buf_simple_add_u8(rb, 0x01);
        net_buf_simple_add_u8(rb, 0x63);  // UA
        uint16_t fcs = edge_crc16_ccitt(rb->data + 1, 5);
        net_buf_simple_add_le16(rb, fcs);
        net_buf_simple_add_u8(rb, 0x7E);
        return 0;
    }

    // 2. 使用内核解析 HDLC
    struct iovec rx_iov = {.iov_base = b->data, .iov_len = b->len};
    edge_cursor_t c;
    edge_cursor_init(&c, &rx_iov, 1);

    uint8_t apdu_raw[1024];
    size_t apdu_len = sizeof(apdu_raw);
    if (edge_hdlc_parse(&g_dlms_ctx.hdlc, &c, apdu_raw, &apdu_len) != EP_OK) return -1;

    // 3. 使用内核分发服务
    struct iovec apdu_iov = {.iov_base = apdu_raw, .iov_len = apdu_len};
    edge_cursor_t ac;
    edge_cursor_init(&ac, &apdu_iov, 1);

    struct iovec resp_iov[8];
    edge_vector_t rv;
    edge_vector_init(&rv, resp_iov, 8);

    if (edge_dlms_server_dispatch(&g_dlms_ctx, &ac, &rv) == EP_OK) {
        // 4. 封装回 HDLC
        struct iovec pdu_iov = {.iov_base = resp_iov[0].iov_base, .iov_len = rv.total_len};
        edge_vector_t hdlc_v;
        struct iovec h_iov[4];
        edge_vector_init(&hdlc_v, h_iov, 4);
        edge_hdlc_build_iframe(&g_dlms_ctx.hdlc, &hdlc_v, pdu_iov.iov_base, pdu_iov.iov_len, true);

        // 线性化到 net_buf
        for (int i = 0; i < hdlc_v.used_count; i++) {
            net_buf_simple_add_mem(&resp_out->b, hdlc_v.iovs[i].iov_base, hdlc_v.iovs[i].iov_len);
        }
    }

    return 0;
}

static void on_alloc(uv_handle_t *h, size_t suggested, uv_buf_t *buf) {
    (void)h;
    buf->base = malloc(suggested);
    buf->len = (unsigned int)suggested;
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
        memcpy(req->b.data, buf->base, (size_t)nread);
        req->b.len = (uint16_t)nread;
        handle_dlms_request(req, resp);
        if (resp->b.len > 0) {
            uv_write_t *wr = malloc(sizeof(uv_write_t));
            char *copy = malloc(resp->b.len);
            memcpy(copy, resp->b.data, resp->b.len);
            uv_buf_t w_buf = uv_buf_init(copy, resp->b.len);
            wr->data = copy;
            uv_write(wr, stream, &w_buf, 1, _on_write_free_cb);
        }
        net_buf_unref(req);
        net_buf_unref(resp);
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

    // 初始化规约内核
    edge_hdlc_init(&g_dlms_ctx.hdlc, 0x01, 0x03);
    g_dlms_ctx.resources = g_resources;
    g_dlms_ctx.resource_count = 1;

    const char *path = "/tmp/edge_sim.sock";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) path = argv[++i];
    }
    uv_loop_t *loop = uv_default_loop();
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 16, 4096, 0, NULL, NULL);
    unlink(path);
    uv_pipe_t *server = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, server, 0);
    server->data = &pool;
    uv_pipe_bind(server, path);
    uv_listen((uv_stream_t *)server, 128, on_new_connection);
    LOG_INFO(LOG_CAT_CORE, "Universal Simulator started on %s", path);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
