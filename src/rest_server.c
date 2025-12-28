#include "rest_server.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ws_proto.h"

#define MAX_WS_CLIENTS 10

typedef struct {
    uv_tcp_t handle;
    bool is_ws;
    bool active;
} rest_client_t;

struct rest_server_s {
    uv_tcp_t server;
    uv_timer_t sse_timer;  // 用于周期性广播实时数据
    service_t *svc;
    rest_client_t clients[MAX_WS_CLIENTS];
};

static void on_handle_close(uv_handle_t *h) {
    rest_client_t *c = (rest_client_t *)h;
    c->active = false;
}

static void on_write_done(uv_write_t *req, int status) {
    (void)status;
    if (req->data) free(req->data);
    free(req);
}

// --- WebSocket 广播实现 ---
static void ws_broadcast(rest_server_t *srv, const char *json) {
    uint8_t frame[2048];
    size_t frame_len;
    if (ws_build_frame(WS_OP_TEXT, (const uint8_t *)json, strlen(json), frame, &frame_len) != 0)
        return;

    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (srv->clients[i].active && srv->clients[i].is_ws) {
            uv_write_t *req = malloc(sizeof(uv_write_t));
            uv_buf_t b = uv_buf_init(strdup((char *)frame), (unsigned int)frame_len);
            req->data = b.base;
            uv_write(req, (uv_stream_t *)&srv->clients[i].handle, &b, 1, on_write_done);
        }
    }
}

// --- 核心业务逻辑路由 (对齐 docs 11.2) ---
static void handle_http_api(rest_server_t *srv, uv_stream_t *stream, const char *method,
                            const char *path, const char *body) {
    (void)srv;
    (void)method;
    (void)body;
    char resp[1024];

    if (strcmp(path, "/api/v1/live") == 0) {
        snprintf(resp, sizeof(resp),
                 "HTTP/1.1 200 OK\r\nContent-Type: "
                 "application/json\r\nAccess-Control-Allow-Origin: *\r\n\r\n"
                 "{\"status\":\"running\",\"trace_mode\":\"v3.1\"}");
    } else {
        snprintf(resp, sizeof(resp), "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
    }

    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t b = uv_buf_init(strdup(resp), (unsigned int)strlen(resp));
    req->data = b.base;
    uv_write(req, stream, &b, 1, on_write_done);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    rest_client_t *client = (rest_client_t *)stream;
    rest_server_t *srv = (rest_server_t *)stream->data;

    if (nread < 0) {
        uv_close((uv_handle_t *)stream, on_handle_close);
    } else if (nread > 0) {
        // 1. 简单的 WebSocket 握手检查
        if (strstr(buf->base, "Upgrade: websocket")) {
            char *key_start = strstr(buf->base, "Sec-WebSocket-Key: ");
            if (key_start) {
                char key[128], accept[128];
                sscanf(key_start + 19, "%s", key);
                ws_gen_accept_key(key, accept);

                char handshake[512];
                snprintf(handshake, sizeof(handshake),
                         "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: "
                         "Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n",
                         accept);

                uv_write_t *req = malloc(sizeof(uv_write_t));
                uv_buf_t b = uv_buf_init(strdup(handshake), (unsigned int)strlen(handshake));
                req->data = b.base;
                uv_write(req, stream, &b, 1, on_write_done);
                client->is_ws = true;
                printf("[REST] Client upgraded to WebSocket\n");
            }
        } else if (!client->is_ws) {
            // 2. 普通 REST API
            char method[8], path[128];
            sscanf(buf->base, "%s %s", method, path);
            handle_http_api(srv, stream, method, path, NULL);
        }
    }
    if (buf->base) free(buf->base);
}

static void on_alloc(uv_handle_t *h, size_t s, uv_buf_t *b) {
    (void)h;
    b->base = malloc(s);
    b->len = (unsigned int)s;
}

static void on_connection(uv_stream_t *server, int status) {
    if (status < 0) return;
    rest_server_t *srv = (rest_server_t *)server->data;

    int idx = -1;
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (!srv->clients[i].active) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return;

    rest_client_t *c = &srv->clients[idx];
    uv_tcp_init(server->loop, &c->handle);
    c->handle.data = srv;
    c->active = true;
    c->is_ws = false;

    if (uv_accept(server, (uv_stream_t *)&c->handle) == 0) {
        uv_read_start((uv_stream_t *)&c->handle, on_alloc, on_read);
    } else {
        c->active = false;
        uv_close((uv_handle_t *)&c->handle, on_handle_close);
    }
}

static void on_broadcast_tick(uv_timer_t *t) {
    rest_server_t *srv = (rest_server_t *)t->data;
    // 模拟从 DataHub 获取数据并广播
    ws_broadcast(srv, "{\"type\":\"telemetry\",\"tag\":1001,\"val\":220.5}");
}

rest_server_t *rest_server_create(uv_loop_t *loop, service_t *svc, int port) {
    rest_server_t *srv = calloc(1, sizeof(rest_server_t));
    srv->svc = svc;
    uv_tcp_init(loop, &srv->server);
    srv->server.data = srv;

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    uv_tcp_bind(&srv->server, (const struct sockaddr *)&addr, 0);
    uv_listen((uv_stream_t *)&srv->server, 128, on_connection);

    uv_timer_init(loop, &srv->sse_timer);
    srv->sse_timer.data = srv;
    uv_timer_start(&srv->sse_timer, on_broadcast_tick, 1000, 1000);

    printf("[REST] Gateway running at http://localhost:%d\n", port);
    return srv;
}

void rest_server_destroy(rest_server_t *srv) {
    if (!srv) return;
    uv_timer_stop(&srv->sse_timer);
    uv_close((uv_handle_t *)&srv->server, (uv_close_cb)free);
}
