#include "web_server.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Static Assets (Tailwind + Alpine) ---
// Configured to talk to API on port 8081
static const char *INDEX_HTML = 
"<!DOCTYPE html>"
"<html lang='en'>"
"<head>"
    "<meta charset='UTF-8'>"
    "<title>EdgeOS HMI</title>"
    "<script src='https://cdn.tailwindcss.com'></script>"
    "<script src='https://unpkg.com/alpinejs@3.x.x/dist/cdn.min.js' defer></script>"
"</head>"
"<body class='bg-slate-900 text-white' x-data='app()'>"
    "<nav class='bg-slate-800 p-4 flex justify-between'>"
        "<h1 class='text-xl font-bold text-sky-500'>EdgeOS <span class='text-slate-500 text-sm'>HMI :8080</span></h1>" 
        "<div class='flex items-center gap-2'>"
            "<div class='w-2 h-2 rounded-full' :class=\"connected?'bg-green-500':'bg-red-500'\"></div>" 
            "<span class='text-xs text-slate-400' x-text=\"connected?'API Connected':'API Disconnected'\"></span>" 
        "</div>"
    "</nav>"
    
    "<main class='p-6 space-y-6'>"
        "<!-- Dashboard Grid -->"
        "<div class='grid grid-cols-3 gap-4'>"
            "<template x-for='item in metrics'>"
                "<div class='bg-slate-800 p-4 rounded border border-slate-700'>"
                    "<div class='text-xs text-slate-400 uppercase' x-text='item.name'></div>" 
                    "<div class='text-2xl font-mono mt-1' x-text='item.val.toFixed(2) + " " + item.unit'></div>" 
                "</div>"
            "</template>"
        "</div>"
        
        "<!-- Control -->"
        "<div class='bg-slate-800 p-4 rounded border border-slate-700'>"
            "<h3 class='font-bold mb-4 text-slate-300'>Manual Control</h3>" 
            "<div class='flex gap-4'>"
                "<button @click='sendCmd(1001, 1)' class='bg-sky-600 hover:bg-sky-500 px-4 py-2 rounded transition'>Start Motor</button>" 
                "<button @click='sendCmd(1001, 0)' class='bg-slate-700 hover:bg-slate-600 px-4 py-2 rounded transition'>Stop Motor</button>" 
            "</div>"
        "</div>"
        
        "<!-- Logs -->"
        "<div class='bg-black p-4 rounded font-mono text-xs text-green-400 h-48 overflow-auto'>"
            "<template x-for='l in logs'><div ><span x-text='l'></span></div></template>" 
        "</div>"
    "</main>"

    "<script>"
        "const API_URL = 'http://' + window.location.hostname + ':8081';"
        "function app() {"
            "return {"
                "connected: false, metrics: [], logs: [],"
                "init() {"
                    "this.log('UI Initialized on port 8080');" 
                    "this.connect();"
                "},"
                "connect() {"
                    "const es = new EventSource(API_URL + '/events');" 
                    "es.onopen = () => { this.connected = true; this.log('SSE Connected to :8081'); };" 
                    "es.onerror = () => { this.connected = false; };" 
                    "es.onmessage = (e) => {"
                        "const msg = JSON.parse(e.data);" 
                        "if(msg.type === 'telemetry') this.metrics = msg.payload;" 
                        "if(msg.type === 'log') this.log(msg.msg);" 
                    "};" 
                "}," 
                "async sendCmd(id, val) {"
                    "this.log(`Sending CMD ${id}=${val}...`);" 
                    "await fetch(API_URL + '/api/v1/control', {"
                        "method: 'POST'," 
                        "body: JSON.stringify({ tag_id: id, value: val })" 
                    "});" 
                "}," 
                "log(m) { this.logs.unshift(new Date().toISOString().split('T')[1] + ' ' + m); }" 
            "}" 
        "}" 
    "</script>"
"</body>"
"</html>";

struct web_server_s {
    uv_tcp_t server;
};

static void on_alloc(uv_handle_t *h, size_t size, uv_buf_t *buf) {
    (void)h;
    buf->base = malloc(size);
    buf->len = size;
}

static void on_free_write(uv_write_t *req, int status) {
    (void)status;
    free(req);
}

static void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        // Always serve INDEX_HTML regardless of path for this MVP
        // In real app: check GET / vs GET /app.js
        char head[128];
        snprintf(head, sizeof(head), 
            "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n", 
            strlen(INDEX_HTML));
        
        uv_write_t *req = malloc(sizeof(uv_write_t));
        uv_buf_t bufs[2] = {
            { .base = head, .len = (unsigned int)strlen(head) },
            { .base = (char*)INDEX_HTML, .len = (unsigned int)strlen(INDEX_HTML) }
        };
        uv_write(req, client, bufs, 2, on_free_write);
    }
    if (buf->base) free(buf->base);
}

static void on_handle_free(uv_handle_t *h) {
    free(h);
}

static void on_conn(uv_stream_t *server, int status) {
    if (status < 0) return; 
    uv_tcp_t *client = malloc(sizeof(uv_tcp_t)); 
    uv_tcp_init(server->loop, client);
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        uv_read_start((uv_stream_t*)client, on_alloc, on_read);
    } else {
        uv_close((uv_handle_t*)client, on_handle_free);
    }
}

web_server_t* web_server_create(uv_loop_t *loop, int port) {
    web_server_t *srv = calloc(1, sizeof(web_server_t));
    uv_tcp_init(loop, &srv->server);
    
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    
    uv_tcp_bind(&srv->server, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&srv->server, 128, on_conn);
    
    if (r) {
        fprintf(stderr, "[WEB] Listen error port %d: %s\n", port, uv_strerror(r));
    } else {
        printf("[WEB] HMI Server running at http://localhost:%d\n", port);
    }
    return srv;
}

void web_server_destroy(web_server_t *srv) {

    if (!srv) return;

    if (!uv_is_closing((uv_handle_t*)&srv->server)) {

        uv_close((uv_handle_t*)&srv->server, NULL);

    }

}
