#include "rest_server.h"
#include <cjson/cJSON.h>
#include <stdlib.h>
#include "sqlite3.h"

#include <string.h>
#include <stdio.h>

struct rest_server_s {
    uv_tcp_t server;
    uv_timer_t sse_timer;
    service_t *svc;
    
    struct {
        uv_tcp_t handle;
        int active;
        int is_sse;
    } clients[20];
};

static void send_json(uv_stream_t *client, const char *json) {
    char head[512];
    snprintf(head, sizeof(head), 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: keep-alive\r\n\r\n", 
        strlen(json));
        
    uv_write_t *req = malloc(sizeof(uv_write_t));
    uv_buf_t bufs[2] = {
        { .base = head, .len = strlen(head) },
        { .base = (char*)json, .len = strlen(json) }
    };
    uv_write(req, client, bufs, 2, (uv_write_cb)free);
}

static void handle_control(rest_server_t *srv, uv_stream_t *client, const char *path, const char *body) {
    cJSON *root = NULL;
    if (body) root = cJSON_Parse(body);
    
    service_cmd_t cmd = {0};

    if (strcmp(path, "/api/v1/control/write") == 0 && root) {
        cmd.type = SVC_CMD_WRITE;
        cmd.tag_id = (uint32_t)cJSON_GetObjectItem(root, "tag_id")->valueint;
        cmd.value = cJSON_GetObjectItem(root, "value")->valuedouble;
        service_post_cmd(srv->svc, &cmd);
        send_json(client, "{\"status\":\"accepted\",\"scenario\":\"S6\"}");
    } 
    else if (strcmp(path, "/api/v1/control/read") == 0 && root) {
        cmd.type = SVC_CMD_READ_ON_DEMAND;
        cmd.tag_id = (uint32_t)cJSON_GetObjectItem(root, "tag_id")->valueint;
        service_post_cmd(srv->svc, &cmd);
        send_json(client, "{\"status\":\"accepted\",\"scenario\":\"S5\"}");
    }
    else if (strcmp(path, "/api/v1/control/ota/start") == 0) {
        cmd.type = SVC_CMD_OTA_START;
        service_post_cmd(srv->svc, &cmd);
        send_json(client, "{\"status\":\"accepted\",\"scenario\":\"S7\"}");
    }
    else if (strcmp(path, "/api/v1/control/ota/end") == 0) {
        cmd.type = SVC_CMD_OTA_END;
        service_post_cmd(srv->svc, &cmd);
        send_json(client, "{\"status\":\"accepted\",\"scenario\":\"S7\"}");
    }
    else {
        send_json(client, "{\"error\":\"Invalid endpoint or payload\"}");
    }
    
    if (root) cJSON_Delete(root);
}

static void on_rest_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        rest_server_t *srv = (rest_server_t*)client->data;
        char *req = buf->base;
        
        if (strncmp(req, "POST /api/v1/control", 20) == 0) {
            char path[64] = {0};
            sscanf(req, "POST %s HTTP", path);
            char *body = strstr(req, "\r\n\r\n");
            handle_control(srv, client, path, body ? body + 4 : NULL);
        } else {
            send_json(client, "{\"info\":\"EdgeOS REST API v2.1\"}");
        }
    }
    if (buf->base) free(buf->base);
}

static void on_alloc(uv_handle_t *h, size_t size, uv_buf_t *buf) {
    (void)h;
    buf->base = malloc(size);
    buf->len = size;
}

static void on_conn(uv_stream_t *server, int status) {
    if (status < 0) return;
    rest_server_t *srv = (rest_server_t*)server->data;
    
    int idx = -1;
    for(int i=0; i<20; i++) if (!srv->clients[i].active) { idx = i; break; }
    if (idx < 0) return;
    
    uv_tcp_init(server->loop, &srv->clients[idx].handle);
    srv->clients[idx].active = 1;
    srv->clients[idx].handle.data = srv;
    
    if (uv_accept(server, (uv_stream_t*)&srv->clients[idx].handle) == 0) {
        uv_read_start((uv_stream_t*)&srv->clients[idx].handle, on_alloc, on_rest_read);
    } else {
        srv->clients[idx].active = 0;
        uv_close((uv_handle_t*)&srv->clients[idx].handle, NULL);
    }
}

rest_server_t* rest_server_create(uv_loop_t *loop, service_t *svc, int port) {
    rest_server_t *srv = calloc(1, sizeof(rest_server_t));
    srv->svc = svc;
    
    uv_tcp_init(loop, &srv->server);
    srv->server.data = srv;
    
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    uv_tcp_bind(&srv->server, (const struct sockaddr*)&addr, 0);
    uv_listen((uv_stream_t*)&srv->server, 128, on_conn);
    
    return srv;
}

void rest_server_destroy(rest_server_t *srv) {
    if (!srv) return;
    uv_close((uv_handle_t*)&srv->server, NULL);
    free(srv);
}
