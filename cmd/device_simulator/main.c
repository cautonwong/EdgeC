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

static uint8_t dlt645_calc_cs(const uint8_t *buf, size_t len) {
    uint8_t cs = 0;
    for (size_t i = 0; i < len; i++) cs += buf[i];
    return cs;
}

int handle_modbus_request(net_buf_t *in, net_buf_t *resp_out) {
    struct net_buf_simple *b = &in->b;
    if (b->len < 6) return -1;
    uint8_t slave_id = b->data[0];
    uint8_t func = b->data[1];
    uint16_t reg = (uint16_t)((b->data[2] << 8) | b->data[3]);

    struct net_buf_simple *rb = &resp_out->b;
    const modbus_map_t *map = edge_get_map_modbus(reg);
    
    if (func == 0x03 && map && map->id != TAG_INVALID) {
        net_buf_simple_add_u8(rb, slave_id);
        net_buf_simple_add_u8(rb, 0x03);
        net_buf_simple_add_u8(rb, 2); 
        net_buf_simple_add_be16(rb, 2205); // 220.5V
        uint16_t crc = edge_crc16_modbus(rb->data, rb->len);
        net_buf_simple_add_le16(rb, crc);
        return 0;
    }
    return -1;
}

int handle_dlt645_request(net_buf_t *in, net_buf_t *resp_out) {
    struct net_buf_simple *b = &in->b;
    while (b->len > 0 && b->data[0] == 0xFE) net_buf_simple_pull(b, 1);
    if (b->len < 12 || b->data[0] != 0x68) return -1;
    
    uint8_t target_addr[6]; memcpy(target_addr, b->data + 1, 6);
    uint32_t req_di = 0;
    req_di |= (uint32_t)(b->data[10] - 0x33);
    req_di |= (uint32_t)(b->data[11] - 0x33) << 8;
    req_di |= (uint32_t)(b->data[12] - 0x33) << 16;
    req_di |= (uint32_t)(b->data[13] - 0x33) << 24;

    const dlt645_map_t *map = edge_get_map_dlt645(req_di);
    if (map && map->id != TAG_INVALID) {
        struct net_buf_simple *rb = &resp_out->b;
        net_buf_simple_add_u8(rb, 0x68); net_buf_simple_add_mem(rb, target_addr, 6); net_buf_simple_add_u8(rb, 0x68);
        net_buf_simple_add_u8(rb, 0x91); // 成功
        net_buf_simple_add_u8(rb, (uint8_t)(4 + map->length));
        for (int i = 0; i < 4; i++) net_buf_simple_add_u8(rb, (uint8_t)(((req_di >> (i*8))&0xFF) + 0x33));
        
        if (map->length == 2) {
            net_buf_simple_add_u8(rb, 0x38); // 05 + 33
            net_buf_simple_add_u8(rb, 0x55); // 22 + 33
        } else {
            for(int i=0; i<map->length; i++) net_buf_simple_add_u8(rb, 0x33);
        }
        net_buf_simple_add_u8(rb, dlt645_calc_cs(rb->data, rb->len));
        net_buf_simple_add_u8(rb, 0x16);
        return 0;
    }
    return -1;
}

static void on_alloc(uv_handle_t *h, size_t suggested, uv_buf_t *buf) {
    (void)h; buf->base = malloc(suggested); buf->len = (unsigned int)suggested;
}
static void on_write_done(uv_write_t *req, int status) {
    (void)status; free(req->data); free(req);
}
static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    net_buf_pool_t *pool = (net_buf_pool_t *)stream->data;
    if (nread > 0) {
        printf("[SIM RX]: ");
        for(int i=0; i<nread; i++) printf("%02X ", (unsigned char)buf->base[i]);
        printf("\n");

        net_buf_t *req = net_buf_alloc(pool, 0);
        net_buf_t *resp = net_buf_alloc(pool, 0);
        memcpy(req->b.data, buf->base, (size_t)nread);
        req->b.len = (uint16_t)nread;

        int ret = -1;
        if (req->b.data[0] == 0x68 || (unsigned char)req->b.data[0] == 0xFE) 
            ret = handle_dlt645_request(req, resp);
        else 
            ret = handle_modbus_request(req, resp);

        if (ret == 0) {
            uv_write_t *wr = malloc(sizeof(uv_write_t));
            char *copy = malloc(resp->b.len);
            memcpy(copy, resp->b.data, resp->b.len);
            uv_buf_t w_buf = uv_buf_init(copy, resp->b.len);
            wr->data = copy;
            uv_write(wr, stream, &w_buf, 1, on_write_done);
        }
        net_buf_unref(req); net_buf_unref(resp);
    } else if (nread < 0) {
        uv_close((uv_handle_t *)stream, (uv_close_cb)free);
    }
    if (buf->base) free(buf->base);
}
static void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        client->data = server->data;
        uv_read_start((uv_stream_t *)client, on_alloc, on_read);
    } else {
        uv_close((uv_handle_t *)client, (uv_close_cb)free);
    }
}

int main(int argc, char *argv[]) {
    edge_log_init(NULL, false);
    const char *path = "/tmp/edge_sim.sock";
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) path = argv[++i];
    }
    uv_loop_t *loop = uv_default_loop();
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 10, 512, 0, NULL, NULL);
    unlink(path);
    uv_pipe_t *server = malloc(sizeof(uv_pipe_t));
    uv_pipe_init(loop, server, 0);
    server->data = &pool;
    uv_pipe_bind(server, path);
    uv_listen((uv_stream_t *)server, 128, on_new_connection);
    LOG_INFO(LOG_CAT_CORE, "Universal Simulator V4.5 started on %s", path);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}