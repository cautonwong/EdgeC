#include "rest_server.h"
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct rest_server_s {
    uv_tcp_t server;
    uv_timer_t sse_timer;
    service_t *svc;
    uint64_t last_seq;
    
    struct {
        uv_tcp_t handle;
        int active;
        int is_sse;
    } clients[20];
};

static void on_write_done(uv_write_t *req, int status) {
    (void)status;
    if (req->data) free(req->data); // Free the packet if attached
    free(req);
}

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
    req->data = NULL; // No dynamic body to free here
    uv_write(req, client, bufs, 2, on_write_done);
}

// --- New API Handlers ---

static void handle_api_points(rest_server_t *srv, uv_stream_t *client) {
    (void)srv;
    cJSON *root = cJSON_CreateArray();
    // 模拟从 srv->svc->config 读取测点
    for(int i=0; i<5; i++) {
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p, "id", 1000+i);
        cJSON_AddStringToObject(p, "name", i==0?"Grid_Voltage":"Load_Power");
        cJSON_AddStringToObject(p, "proto", "ModbusRTU");
        cJSON_AddItemToArray(root, p);
    }
    char *s = cJSON_PrintUnformatted(root);
    send_json(client, s);
    free(s);
    cJSON_Delete(root);
}

static void handle_control(rest_server_t *srv, uv_stream_t *client, const char *path, const char *body) {
    cJSON *root = NULL;
    if (body) root = cJSON_Parse(body);
    service_cmd_t cmd = {0};

    if (strstr(path, "/write") && root) {
        cmd.type = SVC_CMD_WRITE;
        cmd.tag_id = (uint32_t)cJSON_GetObjectItem(root, "tag_id")->valueint;
        cmd.value = cJSON_GetObjectItem(root, "value")->valuedouble;
        service_post_cmd(srv->svc, &cmd);
        send_json(client, "{\"status\":\"S6_ACCEPTED\"}");
    } else if (strstr(path, "/ota/start")) {
        cmd.type = SVC_CMD_OTA_START;
        service_post_cmd(srv->svc, &cmd);
        send_json(client, "{\"status\":\"S7_OTA_LOCK\"}");
    } else {
        send_json(client, "{\"status\":\"UNKNOWN_CMD\"}");
    }
    if (root) cJSON_Delete(root);
}

static void on_rest_close(uv_handle_t *handle) {
    rest_server_t *srv = (rest_server_t*)handle->data;
    for (int i = 0; i < 20; i++) {
        if ((uv_tcp_t*)handle == &srv->clients[i].handle) {
            srv->clients[i].active = 0;
            srv->clients[i].is_sse = 0;
            break;
        }
    }
}

static void on_rest_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    rest_server_t *srv = (rest_server_t*)client->data;
    if (nread > 0) {
        char *req = buf->base;
        if (strncmp(req, "GET /api/v1/points", 18) == 0) handle_api_points(srv, client);
        else if (strncmp(req, "GET /events", 11) == 0) {
            const char *sse_head = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/event-stream\r\n"
                "Cache-Control: no-cache\r\n"
                "Access-Control-Allow-Origin: *\r\n\r\n";
            uv_write_t *w = malloc(sizeof(uv_write_t));
            uv_buf_t b = uv_buf_init((char*)sse_head, (unsigned int)strlen(sse_head));
            w->data = NULL;
            uv_write(w, client, &b, 1, on_write_done);
            
            for(int i=0; i<20; i++) {
                if ((uv_tcp_t*)client == &srv->clients[i].handle) {
                    srv->clients[i].is_sse = 1; 
                    break;
                }
            }
        }
        else if (strncmp(req, "POST /api/v1/control", 20) == 0) {
            char path[64]={0}; sscanf(req, "POST %s ", path);
            char *body = strstr(req, "\r\n\r\n");
            handle_control(srv, client, path, body?body+4:NULL);
        } else {
            send_json(client, "{\"info\":\"EdgeOS REST v2.2\"}");
        }
    } else if (nread < 0) {
        if (!uv_is_closing((uv_handle_t*)client)) {
            uv_close((uv_handle_t*)client, on_rest_close);
        }
    }
    if (buf->base) free(buf->base);
}

static void on_alloc(uv_handle_t *h, size_t size, uv_buf_t *buf) {
    (void)h;
    buf->base = malloc(size); buf->len = (unsigned int)size;
}

static void on_conn(uv_stream_t *server, int status) {
    if (status < 0) return;
    rest_server_t *srv = (rest_server_t*)server->data;
    int idx = -1;
    for(int i=0; i<20; i++) if(!srv->clients[i].active) { idx=i; break; }
    if(idx < 0) return;
    uv_tcp_init(server->loop, &srv->clients[idx].handle);
    srv->clients[idx].active = 1;
    srv->clients[idx].handle.data = srv;
    uv_accept(server, (uv_stream_t*)&srv->clients[idx].handle);
    uv_read_start((uv_stream_t*)&srv->clients[idx].handle, on_alloc, on_rest_read);
}

// --- SSE Broadcast (Data Push) ---
static void on_tick(uv_timer_t *t) {
    rest_server_t *srv = (rest_server_t*)t->data;
    
    shm_ring_t *ring = service_get_ring(srv->svc);
    PipeMsg msg;
    double latest_val = 0;
    bool has_data = false;
    
    if (ring) {
        while (shm_ring_read(ring, &srv->last_seq, &msg) == 0) {
            if (msg.type == PIPEMSG_SCALAR_F64) {
                latest_val = msg.payload.val_f;
                has_data = true;
            }
        }
    }

    if (!has_data) {
        latest_val = 220.0 + (rand() % 100) / 10.0;
    }

    // 构建一个包含多维度数据的模拟 JSON
    char pkt[1024];
    snprintf(pkt, sizeof(pkt), 
        "data: {\"type\":\"telemetry\",\"val\":%.2f,\"current\":%.2f,\"freq\":50.01,\"temp\":42.5}\n\n", 
        latest_val, latest_val / 20.0);
    
    for(int i=0; i<20; i++) {
        if(srv->clients[i].active && srv->clients[i].is_sse) {
            uv_write_t *w = malloc(sizeof(uv_write_t));
            uv_buf_t b = uv_buf_init(strdup(pkt), (unsigned int)strlen(pkt));
            w->data = b.base; 
            uv_write(w, (uv_stream_t*)&srv->clients[i].handle, &b, 1, on_write_done);
        }
    }
}

rest_server_t* rest_server_create(uv_loop_t *loop, service_t *svc, int port) {
    rest_server_t *srv = calloc(1, sizeof(rest_server_t));
    srv->svc = svc;
    
    uv_tcp_init(loop, &srv->server);
    srv->server.data = srv;
    
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    
    int r = uv_tcp_bind(&srv->server, (const struct sockaddr*)&addr, 0);
    if (r) {
        fprintf(stderr, "[REST] Bind error port %d: %s\n", port, uv_strerror(r));
        free(srv);
        return NULL;
    }

    r = uv_listen((uv_stream_t*)&srv->server, 128, on_conn);
    if (r) {
        fprintf(stderr, "[REST] Listen error port %d: %s\n", port, uv_strerror(r));
        free(srv);
        return NULL;
    }
    
    uv_timer_init(loop, &srv->sse_timer);
    srv->sse_timer.data = srv;
    uv_timer_start(&srv->sse_timer, on_tick, 500, 500);
    
    printf("[REST] API Server running at http://localhost:%d\n", port);
    return srv;
}

void rest_server_destroy(rest_server_t *srv) {

    if (!srv) return;

    

    if (!uv_is_closing((uv_handle_t*)&srv->sse_timer)) {

        uv_close((uv_handle_t*)&srv->sse_timer, NULL);

    }

    

    // Close the server listener - the memory will be freed by uv_walk in northbound_mgr

    if (!uv_is_closing((uv_handle_t*)&srv->server)) {

        uv_close((uv_handle_t*)&srv->server, NULL);

    }

}
