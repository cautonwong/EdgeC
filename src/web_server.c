#include "web_server.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

#define WEB_DOC_ROOT "config/www/index.html"

struct web_server_s {
    uv_tcp_t server;
};

static void on_alloc(uv_handle_t *h, size_t s, uv_buf_t *b) {
    (void)h;
    b->base = (char *)malloc(s);
    b->len = (unsigned int)s;
}

static void on_write_done(uv_write_t *req, int status) {
    (void)status;
    if (req->data) free(req->data); // 释放读取的文件内容缓存
    free(req);
}

static void on_handle_free(uv_handle_t *h) {
    free(h);
}

static void on_read(uv_stream_t *c, ssize_t n, const uv_buf_t *b) {
    if (n > 0) {
        // ... (File reading and sending logic remains the same)
        uv_fs_t open_req;
        int fd = uv_fs_open(c->loop, &open_req, WEB_DOC_ROOT, O_RDONLY, 0, NULL);
        uv_fs_req_cleanup(&open_req);

        if (fd >= 0) {
            uv_fs_t stat_req;
            uv_fs_fstat(c->loop, &stat_req, fd, NULL);
            size_t size = (size_t)stat_req.statbuf.st_size;
            uv_fs_req_cleanup(&stat_req);

            char *content = (char *)malloc(size);
            uv_buf_t content_buf = uv_buf_init(content, (unsigned int)size);
            
            uv_fs_t read_req;
            uv_fs_read(c->loop, &read_req, fd, &content_buf, 1, 0, NULL);
            uv_fs_req_cleanup(&read_req);
            uv_fs_close(c->loop, &open_req, fd, NULL);
            uv_fs_req_cleanup(&open_req);

            char head[256];
            snprintf(head, sizeof(head), 
                "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n"
                "Access-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n", 
                size);
            
            uv_write_t *wr_req = (uv_write_t *)malloc(sizeof(uv_write_t));
            uv_buf_t bufs[2] = {
                { .base = head, .len = (unsigned int)strlen(head) },
                { .base = content, .len = (unsigned int)size }
            };
            wr_req->data = content;
            uv_write(wr_req, c, bufs, 2, on_write_done);
        } else {
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            uv_write_t *wr_req = (uv_write_t *)malloc(sizeof(uv_write_t));
            uv_buf_t buf = uv_buf_init((char*)not_found, (unsigned int)strlen(not_found));
            wr_req->data = NULL;
            uv_write(wr_req, c, &buf, 1, on_write_done);
        }
    } else if (n < 0) {
        if (!uv_is_closing((uv_handle_t*)c)) {
            uv_close((uv_handle_t*)c, on_handle_free);
        }
    }
    // 移除 if(b->base) free(b->base)，因为 libuv 内存应由 alloc_cb 分配并在这里统一处理
    // 如果 b->base 是在 on_read 之前分配的，确实需要 free。
    if (b->base) free(b->base);
}

static void on_conn(uv_stream_t *s, int st) {
    if (st < 0) return;
    uv_tcp_t *c = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(s->loop, c);
    if (uv_accept(s, (uv_stream_t*)c) == 0) {
        uv_read_start((uv_stream_t*)c, on_alloc, on_read);
    } else {
        uv_close((uv_handle_t*)c, on_handle_free);
    }
}

web_server_t* web_server_create(uv_loop_t *l, int p) {
    web_server_t *srv = (web_server_t *)calloc(1, sizeof(web_server_t));
    uv_tcp_init(l, &srv->server);
    struct sockaddr_in a;
    uv_ip4_addr("0.0.0.0", p, &a);
    uv_tcp_bind(&srv->server, (const struct sockaddr*)&a, 0);
    int r = uv_listen((uv_stream_t*)&srv->server, 128, on_conn);
    if (r) {
        fprintf(stderr, "[WEB] Listen error port %d: %s\n", p, uv_strerror(r));
    } else {
        printf("[WEB] HMI Professional UI running at http://localhost:%d\n", p);
    }
    return srv;
}

void web_server_destroy(web_server_t *srv) {
    if (srv) {
        if (!uv_is_closing((uv_handle_t*)&srv->server)) {
            uv_close((uv_handle_t*)&srv->server, on_handle_free);
        }
    }
}
