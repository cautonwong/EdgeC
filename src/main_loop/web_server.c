#include "web_server.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "edge_log.h"

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
    if (req->data) free(req->data);
    free(req);
}

static void on_handle_free(uv_handle_t *h) {
    free(h);
}

static void on_read(uv_stream_t *c, ssize_t n, const uv_buf_t *b) {
    if (n > 0) {
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

            uv_fs_t close_req;
            uv_fs_close(c->loop, &close_req, fd, NULL);
            uv_fs_req_cleanup(&close_req);

            char head[512];
            snprintf(head, sizeof(head),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html; charset=utf-8\r\n"
                     "Content-Length: %zu\r\n"
                     "Access-Control-Allow-Origin: *\r\n"
                     "Connection: close\r\n\r\n",
                     size);

            uv_write_t *wr_req = (uv_write_t *)malloc(sizeof(uv_write_t));
            size_t head_len = strlen(head);
            char *full_payload = malloc(head_len + size);
            memcpy(full_payload, head, head_len);
            memcpy(full_payload + head_len, content, size);

            uv_buf_t out_buf = uv_buf_init(full_payload, (unsigned int)(head_len + size));
            wr_req->data = full_payload;
            uv_write(wr_req, c, &out_buf, 1, on_write_done);
            free(content);
        } else {
            LOG_ERROR(LOG_CAT_CORE, "Failed to open %s: %d", WEB_DOC_ROOT, fd);
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            uv_write_t *wr_req = (uv_write_t *)malloc(sizeof(uv_write_t));
            uv_buf_t buf = uv_buf_init(strdup(not_found), (unsigned int)strlen(not_found));
            wr_req->data = buf.base;
            uv_write(wr_req, c, &buf, 1, on_write_done);
        }
    } else if (n < 0) {
        uv_close((uv_handle_t *)c, on_handle_free);
    }
    if (b->base) free(b->base);
}

static void on_conn(uv_stream_t *s, int st) {
    if (st < 0) return;
    uv_tcp_t *c = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(s->loop, c);
    if (uv_accept(s, (uv_stream_t *)c) == 0) {
        uv_read_start((uv_stream_t *)c, on_alloc, on_read);
    } else {
        uv_close((uv_handle_t *)c, on_handle_free);
    }
}

web_server_t *web_server_create(uv_loop_t *l, int p) {
    web_server_t *srv = (web_server_t *)calloc(1, sizeof(web_server_t));
    uv_tcp_init(l, &srv->server);
    struct sockaddr_in a;
    uv_ip4_addr("0.0.0.0", p, &a);
    uv_tcp_bind(&srv->server, (const struct sockaddr *)&a, 0);
    int r = uv_listen((uv_stream_t *)&srv->server, 128, on_conn);
    if (r) {
        LOG_ERROR(LOG_CAT_CORE, "WEB Listen error port %d: %s", p, uv_strerror(r));
    } else {
        LOG_INFO(LOG_CAT_CORE, "Native HMI Gateway running at http://localhost:%d", p);
    }
    return srv;
}

void web_server_destroy(web_server_t *srv) {
    if (srv) {
        if (!uv_is_closing((uv_handle_t *)&srv->server)) {
            uv_close((uv_handle_t *)&srv->server, on_handle_free);
        }
    }
}