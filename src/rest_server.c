#include "rest_server.h"
#include "cjson/cJSON.h"
#include "sqlite3.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct rest_server_s {
    uv_tcp_t server;
    uv_timer_t sse_timer;
    shm_ring_t *ring;
    uint64_t last_seq;
    
    // Clients
    struct {
        uv_tcp_t handle;
        int active;
        int is_sse;
    } clients[20];
};

static void send_cors_headers(uv_stream_t *stream) {
    (void)stream;
    // Required for :8080 JS to talk to :8081 API
    // const char *cors = 
    //    "Access-Control-Allow-Origin: *\r\n"
    //    "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
    //    "Access-Control-Allow-Headers: Content-Type\r\n";
}

static void on_simple_write(uv_write_t *req, int status) {
    (void)status;
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
    uv_buf_t bufs[2];
    bufs[0].base = strdup(head);
    bufs[0].len = (unsigned int)strlen(head);
    bufs[1].base = (char*)json;
    bufs[1].len = (unsigned int)strlen(json);
    
    // Using a more robust way to free the strdup'd head
    // For MVP, we might leak 'head' or assume it's freed elsewhere.
    // Let's use a simpler single buffer for now or a proper req structure.
    
    char *full_resp = malloc(strlen(head) + strlen(json) + 1);
    strcpy(full_resp, head);
    strcat(full_resp, json);
    
    uv_buf_t buf = uv_buf_init(full_resp, (unsigned int)strlen(full_resp));
    req->data = full_resp;
    
    uv_write(req, client, &buf, 1, (uv_write_cb)on_simple_write);
}

static void on_rest_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        char *req = buf->base;
        
        // Handle OPTIONS (Preflight)
        if (strncmp(req, "OPTIONS", 7) == 0) {
            const char *resp = 
                "HTTP/1.1 204 No Content\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Access-Control-Allow-Methods: GET, POST\r\n"
                "Access-Control-Allow-Headers: Content-Type\r\n\r\n";
            uv_write_t *w = malloc(sizeof(uv_write_t));
            uv_buf_t b = uv_buf_init(strdup(resp), (unsigned int)strlen(resp));
            w->data = b.base;
            uv_write(w, client, &b, 1, (uv_write_cb)on_simple_write);
        }
        else if (strncmp(req, "GET /api/v1/system", 18) == 0) {
            send_json(client, "{\"app\":\"EdgeOS API\",\"port\":8081}");
        }
        else if (strncmp(req, "GET /events", 11) == 0) {
            // SSE
            const char *sse = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/event-stream\r\n"
                "Cache-Control: no-cache\r\n"
                "Access-Control-Allow-Origin: *\r\n\r\n";
            
            uv_write_t *w = malloc(sizeof(uv_write_t));
            uv_buf_t b = uv_buf_init(strdup(sse), (unsigned int)strlen(sse));
            w->data = b.base;
            uv_write(w, client, &b, 1, (uv_write_cb)on_simple_write);
            
            // Register client
            rest_server_t *srv = (rest_server_t*)client->data;
            for(int i=0; i<20; i++) {
                if ((uv_tcp_t*)client == &srv->clients[i].handle) {
                    srv->clients[i].is_sse = 1; break;
                }
            }
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
    
    // Find slot
    int idx = -1;
    for(int i=0; i<20; i++) if (!srv->clients[i].active) { idx = i; break; }
    if (idx < 0) return;
    
    uv_tcp_init(server->loop, &srv->clients[idx].handle);
    srv->clients[idx].active = 1;
    srv->clients[idx].is_sse = 0;
    srv->clients[idx].handle.data = srv; // Link back to server instance
    
    if (uv_accept(server, (uv_stream_t*)&srv->clients[idx].handle) == 0) {
        uv_read_start((uv_stream_t*)&srv->clients[idx].handle, on_alloc, on_rest_read);
    } else {
        srv->clients[idx].active = 0;
        uv_close((uv_handle_t*)&srv->clients[idx].handle, NULL);
    }
}

// Push Logic
static void on_tick(uv_timer_t *t) {
    rest_server_t *srv = (rest_server_t*)t->data;
    
    // Mock Data push
    char buf[256];
    snprintf(buf, sizeof(buf), 
        "data: {\"type\":\"telemetry\",\"payload\":[{\"name\":\"VOLT\",\"val\":%.1f,\"unit\":\"V\"}]}\n\n",
        220.0 + (rand()%10));
    
    for(int i=0; i<20; i++) {
        if (srv->clients[i].active && srv->clients[i].is_sse) {
            uv_write_t *w = malloc(sizeof(uv_write_t));
            uv_buf_t b = uv_buf_init(strdup(buf), (unsigned int)strlen(buf));
            w->data = b.base;
            uv_write(w, (uv_stream_t*)&srv->clients[i].handle, &b, 1, (uv_write_cb)on_simple_write);
        }
    }
}

rest_server_t* rest_server_create(uv_loop_t *loop, shm_ring_t *ring, int port) {
    rest_server_t *srv = calloc(1, sizeof(rest_server_t));
    srv->ring = ring;
    
    uv_tcp_init(loop, &srv->server);
    srv->server.data = srv;
    
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    
    uv_tcp_bind(&srv->server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&srv->server, 128, on_conn);
    
    if (r) {
        fprintf(stderr, "[REST] Listen error port %d: %s\n", port, uv_strerror(r));
    } else {
        printf("[REST] API Server running at http://localhost:%d\n", port);
    }
    
    uv_timer_init(loop, &srv->sse_timer);
    srv->sse_timer.data = srv;
    uv_timer_start(&srv->sse_timer, on_tick, 500, 500);
    
    return srv;
}

void rest_server_destroy(rest_server_t *srv) {
    if (!srv) return;
    uv_close((uv_handle_t*)&srv->server, NULL);
    free(srv);
}
