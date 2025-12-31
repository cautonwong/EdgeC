#include "web_server.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "edge_log.h"

struct web_server_s {
    uv_tcp_t server;
    char doc_root[256];
    web_api_handler_pt api_handler;
    void *api_user_data;
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

static const char *get_mime_type(const char *path) {
    if (strstr(path, ".js")) return "application/javascript";
    if (strstr(path, ".css")) return "text/css";
    if (strstr(path, ".ico")) return "image/x-icon";
    if (strstr(path, ".png")) return "image/png";
    if (strstr(path, ".svg")) return "image/svg+xml";
    return "text/html; charset=utf-8";
}

static void on_read(uv_stream_t *c, ssize_t n, const uv_buf_t *b) {
    if (n > 0) {
        web_server_t *srv = (web_server_t *)c->data;

        // 1. 简单的 HTTP 解析：提取路径 (e.g. "GET /assets/index.js HTTP/1.1")
        char path[128] = {0};
        if (n > 5 && strncmp(b->base, "GET ", 4) == 0) {
            char *start = b->base + 4;
            char *end = strchr(start, ' ');
            if (end) {
                size_t len = (size_t)(end - start);
                if (len > sizeof(path) - 1) len = sizeof(path) - 1;
                memcpy(path, start, len);
            }
        }

        // 1.1 API 路由拦截 (如果配置了 API Handler 且路径以 /api 开头)
        if (srv->api_handler && strncmp(path, "/api/", 5) == 0) {
            char *body = strstr(b->base, "\r\n\r\n");
            if (body) body += 4;

            char *json = srv->api_handler(srv->api_user_data, path, body);
            size_t json_len = strlen(json);

            char head[256];
            snprintf(head, sizeof(head),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %zu\r\n"
                     "Access-Control-Allow-Origin: *\r\n"
                     "Connection: close\r\n\r\n",
                     json_len);

            size_t head_len = strlen(head);
            char *resp = malloc(head_len + json_len + 1);
            memcpy(resp, head, head_len);
            memcpy(resp + head_len, json, json_len);

            uv_write_t *wr_req = (uv_write_t *)malloc(sizeof(uv_write_t));
            wr_req->data = resp;
            uv_buf_t out_buf = uv_buf_init(resp, (unsigned int)(head_len + json_len));
            uv_write(wr_req, c, &out_buf, 1, on_write_done);
            free(json);
            if (b->base) free(b->base);
            return;
        }

        // 2. 映射路径到物理文件
        char full_path[512];
        // 去掉 doc_root 末尾的 index.html (如果是这种情况)
        char base_dir[256];
        strncpy(base_dir, srv->doc_root, sizeof(base_dir) - 1);
        char *last_slash = strrchr(base_dir, '/');
        if (last_slash && strcmp(last_slash, "/index.html") == 0) {
            *last_slash = '\0';
        }

        if (strcmp(path, "/") == 0 || strlen(path) == 0) {
            snprintf(full_path, sizeof(full_path), "%s/index.html", base_dir);
        } else {
            snprintf(full_path, sizeof(full_path), "%s%s", base_dir, path);
        }

        uv_fs_t open_req;
        int fd = uv_fs_open(c->loop, &open_req, full_path, O_RDONLY, 0, NULL);
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
                     "Content-Type: %s\r\n"
                     "Content-Length: %zu\r\n"
                     "Access-Control-Allow-Origin: *\r\n"
                     "Connection: close\r\n\r\n",
                     get_mime_type(full_path), size);

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
            LOG_ERROR(LOG_CAT_CORE, "Failed to open %s (Requested: %s)", full_path, path);
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
    c->data = s->data;  // Pass server context
    if (uv_accept(s, (uv_stream_t *)c) == 0) {
        uv_read_start((uv_stream_t *)c, on_alloc, on_read);
    } else {
        uv_close((uv_handle_t *)c, on_handle_free);
    }
}

web_server_t *web_server_create(uv_loop_t *l, int p, const char *doc_root) {
    web_server_t *srv = (web_server_t *)calloc(1, sizeof(web_server_t));
    if (doc_root) {
        strncpy(srv->doc_root, doc_root, sizeof(srv->doc_root) - 1);
    } else {
        strcpy(srv->doc_root, "config/www/index.html");
    }

    uv_tcp_init(l, &srv->server);
    srv->server.data = srv;
    struct sockaddr_in a;
    uv_ip4_addr("0.0.0.0", p, &a);
    uv_tcp_bind(&srv->server, (const struct sockaddr *)&a, 0);
    int r = uv_listen((uv_stream_t *)&srv->server, 128, on_conn);
    if (r) {
        LOG_ERROR(LOG_CAT_CORE, "WEB Listen error port %d: %s", p, uv_strerror(r));
    } else {
        LOG_INFO(LOG_CAT_CORE, "Native HMI Gateway running at http://localhost:%d (root: %s)", p,
                 srv->doc_root);
    }
    return srv;
}

void web_server_set_api_handler(web_server_t *srv, web_api_handler_pt handler, void *user_data) {
    if (srv) {
        srv->api_handler = handler;
        srv->api_user_data = user_data;
    }
}

void web_server_destroy(web_server_t *srv) {
    if (srv) {
        if (!uv_is_closing((uv_handle_t *)&srv->server)) {
            uv_close((uv_handle_t *)&srv->server, on_handle_free);
        }
    }
}