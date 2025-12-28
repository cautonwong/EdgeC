#include "rest_server.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "edge_service.h"
#include "ws_proto.h"

#define MAX_WS_CLIENTS 10

typedef struct {
    uv_tcp_t handle;
    bool is_ws;
    bool active;
} rest_client_t;

struct rest_server_s {
    uv_tcp_t server;
    uv_timer_t broadcast_timer;
    service_t *svc;
    rest_client_t clients[MAX_WS_CLIENTS];
    uint64_t last_shm_seq;  // 追踪 DataHub 的读取进度
};

static void on_handle_close(uv_handle_t *h) {
    rest_client_t *c = (rest_client_t *)h;
    // 标记连接已关闭，可被新连接复用
    c->active = false;
}

static void on_write_done(uv_write_t *req, int status) {
    (void)status;
    if (req->data) free(req->data);
    free(req);
}

static void ws_broadcast(rest_server_t *srv, const char *json) {
    uint8_t frame[4096];
    size_t frame_len;
    if (ws_build_frame(WS_OP_TEXT, (const uint8_t *)json, strlen(json), frame, &frame_len) != 0)
        return;

    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (srv->clients[i].active && srv->clients[i].is_ws) {
            uv_write_t *req = malloc(sizeof(uv_write_t));
            // 使用堆内存并在回调释放，确保异步安全
            req->data = malloc(frame_len);
            memcpy(req->data, frame, frame_len);
            uv_buf_t b = uv_buf_init(req->data, (unsigned int)frame_len);
            uv_write(req, (uv_stream_t *)&srv->clients[i].handle, &b, 1, on_write_done);
        }
    }
}

// REST API 实现 (对齐 docs 11.2)
static void handle_http_api(rest_server_t *srv, uv_stream_t *stream, const char *path) {
    char resp_body[2048] = {0};
    char full_resp[4096] = {0};
    const char *status_line = "HTTP/1.1 200 OK\r\n";

    if (strncmp(path, "/api/v1/system/bus", 18) == 0) {
        arbiter_bus_status_t stats[8];
        int count = arbiter_get_statuses(service_get_arbiter(srv->svc), stats, 8);
        cJSON *root = cJSON_CreateArray();
        for (int i = 0; i < count; i++) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "id", stats[i].bus_id);
            cJSON_AddBoolToObject(item, "busy", stats[i].is_busy);
            cJSON_AddBoolToObject(item, "exclusive", stats[i].is_exclusive);
            cJSON_AddNumberToObject(item, "queue", stats[i].queue_len);
            cJSON_AddStringToObject(item, "job", stats[i].current_job_id);
            cJSON_AddItemToArray(root, item);
        }
        char *str = cJSON_PrintUnformatted(root);
        strncpy(resp_body, str, sizeof(resp_body) - 1);
        free(str);
        cJSON_Delete(root);
    } else if (strncmp(path, "/api/v1/points", 14) == 0) {
        cJSON *root = cJSON_CreateArray();
        const char *names[] = {"Grid_Volt", "Grid_Freq", "Batt_SOC", "Load_Power"};
        for (int i = 0; i < 4; i++) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "id", 1001 + i);
            cJSON_AddStringToObject(item, "name", names[i]);
            cJSON_AddStringToObject(item, "proto", "MODBUS");
            cJSON_AddItemToArray(root, item);
        }
        char *str = cJSON_PrintUnformatted(root);
        strncpy(resp_body, str, sizeof(resp_body) - 1);
        free(str);
        cJSON_Delete(root);
    } else if (strncmp(path, "/api/v1/audit", 13) == 0) {
        cJSON *root = cJSON_CreateArray();
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ts", "2025-12-28 12:00:01");
        cJSON_AddStringToObject(item, "act", "WRITE");
        cJSON_AddStringToObject(item, "target", "Relay_01");
        cJSON_AddStringToObject(item, "res", "SUCCESS");
        cJSON_AddItemToArray(root, item);
        char *str = cJSON_PrintUnformatted(root);
        strncpy(resp_body, str, sizeof(resp_body) - 1);
        free(str);
        cJSON_Delete(root);
    } else {
        status_line = "HTTP/1.1 404 Not Found\r\n";
        strcpy(resp_body, "{\"error\":\"not_found\"}");
    }

    snprintf(full_resp, sizeof(full_resp),
             "%s"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             status_line, strlen(resp_body), resp_body);

    uv_write_t *req = malloc(sizeof(uv_write_t));
    size_t total_len = strlen(full_resp);
    uv_buf_t b = uv_buf_init(strdup(full_resp), (unsigned int)total_len);
    req->data = b.base;
    uv_write(req, stream, &b, 1, on_write_done);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    rest_client_t *client = (rest_client_t *)stream;
    rest_server_t *srv = (rest_server_t *)stream->data;

    if (nread < 0) {
        uv_close((uv_handle_t *)stream, on_handle_close);
    } else if (nread > 0) {
        // 确保字符串以 NULL 结尾
        if (nread < (ssize_t)buf->len) {
            buf->base[nread] = '\0';
        } else {
            // 如果满了，手动截断以保安全
            buf->base[nread - 1] = '\0';
        }

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
            }
        } else if (!client->is_ws) {
            char method[8], path[128];
            sscanf(buf->base, "%s %s", method, path);
            handle_http_api(srv, stream, path);
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
    for (int i = 0; i < MAX_WS_CLIENTS; i++)
        if (!srv->clients[i].active) {
            idx = i;
            break;
        }
    if (idx < 0) return;
    rest_client_t *c = &srv->clients[idx];
    uv_tcp_init(server->loop, &c->handle);
    c->handle.data = srv;
    c->active = true;
    c->is_ws = false;
    if (uv_accept(server, (uv_stream_t *)&c->handle) == 0)
        uv_read_start((uv_stream_t *)&c->handle, on_alloc, on_read);
    else
        uv_close((uv_handle_t *)&c->handle, on_handle_close);
}

// 核心：读取 DataHub 真实数据并广播 (S1-S4 场景打通)
static void on_broadcast_tick(uv_timer_t *t) {
    rest_server_t *srv = (rest_server_t *)t->data;
    shm_ring_t *ring = service_get_shmring(srv->svc);
    if (!ring) return;

    PipeMsg msg;
    // 循环读取直到环形缓冲区最新
    while (shm_ring_read(ring, &srv->last_shm_seq, &msg) == 0) {
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "telemetry");
        cJSON_AddNumberToObject(root, "tag", msg.tag_id);
        cJSON_AddNumberToObject(root, "val", msg.payload.val.f);
        cJSON_AddNumberToObject(root, "ts", (double)msg.ts);
        cJSON_AddNumberToObject(root, "trace", msg.state);  // 对应 TraceState

        char *str = cJSON_PrintUnformatted(root);
        ws_broadcast(srv, str);
        free(str);
        cJSON_Delete(root);
    }
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
    uv_timer_init(loop, &srv->broadcast_timer);
    srv->broadcast_timer.data = srv;
    uv_timer_start(&srv->broadcast_timer, on_broadcast_tick, 100, 100);  // 10Hz 扫描
    return srv;
}

void rest_server_destroy(rest_server_t *srv) {
    if (!srv) return;
    uv_timer_stop(&srv->broadcast_timer);
    uv_close((uv_handle_t *)&srv->server, (uv_close_cb)free);
}
