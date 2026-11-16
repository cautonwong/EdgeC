#include "web_server.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"

struct web_server_s {
    uv_tcp_t server;
    char doc_root[256];
    web_api_handler_pt api_handler;
    void *api_user_data;
};

int web_server_resolve_path(const char *doc_root, const char *request_path, char *out,
                            size_t out_size) {
    if (!doc_root || !request_path || !out || out_size == 0) return -1;

    const char *p = request_path;
    while (*p) {
        while (*p == '/') p++;
        const char *segment = p;
        while (*p && *p != '/') p++;
        if ((size_t)(p - segment) == 2 && segment[0] == '.' && segment[1] == '.') return -1;
    }

    char root_real[PATH_MAX];
    if (!realpath(doc_root, root_real)) return -1;

    const char *relative = request_path;
    while (*relative == '/') relative++;
    if (*relative == '\0') relative = "index.html";

    char candidate[PATH_MAX];
    int n = snprintf(candidate, sizeof(candidate), "%s/%s", root_real, relative);
    if (n < 0 || (size_t)n >= sizeof(candidate)) return -1;

    char resolved[PATH_MAX];
    if (!realpath(candidate, resolved)) return -1;
    size_t root_len = strlen(root_real);
    if (strncmp(resolved, root_real, root_len) != 0) return -1;
    if (resolved[root_len] != '/' && resolved[root_len] != '\0') return -1;

    n = snprintf(out, out_size, "%s", resolved);
    if (n < 0 || (size_t)n >= out_size) return -1;
    return 0;
}

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

static void send_status(uv_stream_t *c, const char *status) {
    char line[128];
    int n = snprintf(line, sizeof(line), "HTTP/1.1 %s\r\nContent-Length: 0\r\n\r\n", status);
    if (n < 0 || (size_t)n >= sizeof(line)) return;
    char *body = malloc((size_t)n);
    uv_write_t *req = malloc(sizeof(*req));
    if (!body || !req) {
        free(body);
        free(req);
        return;
    }
    memcpy(body, line, (size_t)n);
    req->data = body;
    uv_buf_t buf = uv_buf_init(body, (unsigned)n);
    if (uv_write(req, c, &buf, 1, on_write_done) != 0) {
        free(body);
        free(req);
    }
}


static void on_read(uv_stream_t *c, ssize_t n, const uv_buf_t *b) {
    if (n > 0) {
        web_server_t *srv = (web_server_t *)c->data;

        // b->base 来自 libuv 的裸缓冲，不保证 NUL 结尾，先复制再解析
        char request[512];
        size_t nread = (size_t)n;
        size_t copy = nread < sizeof(request) - 1 ? nread : sizeof(request) - 1;
        memcpy(request, b->base, copy);
        request[copy] = '\0';

        // 1. 简单的 HTTP 解析：提取路径 (e.g. "GET /assets/index.js HTTP/1.1")
        char path[128] = {0};
        if (copy > 5 && strncmp(request, "GET ", 4) == 0) {
            char *start = request + 4;
            char *end = strchr(start, ' ');
            if (end) {
                size_t len = (size_t)(end - start);
                if (len > sizeof(path) - 1) len = sizeof(path) - 1;
                memcpy(path, start, len);
            }
        }

        // 1.1 API 路由拦截 (如果配置了 API Handler 且路径以 /api 开头)
        if (srv->api_handler && strncmp(path, "/api/", 5) == 0) {
            char *body = strstr(request, "\r\n\r\n");
            if (body) body += 4;

            char *json = srv->api_handler(srv->api_user_data, path, body);
            if (!json) {
                send_status(c, "500 Internal Server Error");
                free(b->base);
                return;
            }
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
            uv_write_t *wr_req = (uv_write_t *)malloc(sizeof(*wr_req));
            if (!resp || !wr_req) {
                free(json);
                free(resp);
                free(wr_req);
                send_status(c, "500 Internal Server Error");
                free(b->base);
                return;
            }
            memcpy(resp, head, head_len);
            memcpy(resp + head_len, json, json_len);

            wr_req->data = resp;
            uv_buf_t out_buf = uv_buf_init(resp, (unsigned int)(head_len + json_len));
            uv_write(wr_req, c, &out_buf, 1, on_write_done);
            free(json);
            if (b->base) free(b->base);
            return;
        }

        // 2. 映射路径到物理文件
        char full_path[512];
        char base_dir[256];

        // 确保 doc_root 即使为空也能指向当前目录
        if (strlen(srv->doc_root) == 0) {
            strcpy(base_dir, ".");
        } else {
            strncpy(base_dir, srv->doc_root, sizeof(base_dir) - 1);
            base_dir[sizeof(base_dir) - 1] = '\0';
        }

        // 去掉末尾可能存在的 /index.html
        char *idx_ptr = strstr(base_dir, "/index.html");
        if (idx_ptr) *idx_ptr = '\0';

        // 去掉末尾可能存在的单斜杠，统一拼接逻辑
        size_t blen = strlen(base_dir);
        if (blen > 0 && base_dir[blen - 1] == '/') base_dir[blen - 1] = '\0';

        int fd = -1;
        if (web_server_resolve_path(base_dir, path, full_path, sizeof(full_path)) == 0) {
            uv_fs_t open_req;
            fd = uv_fs_open(c->loop, &open_req, full_path, O_RDONLY, 0, NULL);
            uv_fs_req_cleanup(&open_req);
        }

        if (fd >= 0) {
            uv_fs_t stat_req;
            if (uv_fs_fstat(c->loop, &stat_req, fd, NULL) != 0) {
                uv_fs_req_cleanup(&stat_req);
                uv_fs_t close_req;
                uv_fs_close(c->loop, &close_req, fd, NULL);
                uv_fs_req_cleanup(&close_req);
                send_status(c, "500 Internal Server Error");
                free(b->base);
                return;
            }
            size_t size = (size_t)stat_req.statbuf.st_size;
            uv_fs_req_cleanup(&stat_req);
            if (size > 0xFFFFFFFFu) {
                uv_fs_t close_req;
                uv_fs_close(c->loop, &close_req, fd, NULL);
                uv_fs_req_cleanup(&close_req);
                send_status(c, "500 Internal Server Error");
                free(b->base);
                return;
            }

            char *content = malloc(size ? size : 1);
            if (!content) {
                uv_fs_t close_req;
                uv_fs_close(c->loop, &close_req, fd, NULL);
                uv_fs_req_cleanup(&close_req);
                send_status(c, "500 Internal Server Error");
                free(b->base);
                return;
            }
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

            uv_write_t *wr_req = (uv_write_t *)malloc(sizeof(*wr_req));
            size_t head_len = strlen(head);
            char *full_payload = malloc(head_len + size);
            if (!wr_req || !full_payload) {
                free(wr_req);
                free(full_payload);
                free(content);
                send_status(c, "500 Internal Server Error");
                free(b->base);
                return;
            }
            memcpy(full_payload, head, head_len);
            memcpy(full_payload + head_len, content, size);

            uv_buf_t out_buf = uv_buf_init(full_payload, (unsigned int)(head_len + size));
            wr_req->data = full_payload;
            if (uv_write(wr_req, c, &out_buf, 1, on_write_done) != 0) {
                free(full_payload);
                free(wr_req);
            }
            free(content);
        } else {
            char cwd[256];
            if (getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "unknown");
            LOG_ERROR(LOG_CAT_CORE, "Failed to open %s (Requested: %s). CWD: %s", full_path, path,
                      cwd);
            send_status(c, "404 Not Found");
        }
    } else if (n < 0) {
        if (!uv_is_closing((uv_handle_t *)c)) {
            uv_close((uv_handle_t *)c, on_handle_free);
        }
    }
    if (b->base) free(b->base);
}

static void on_conn(uv_stream_t *s, int st) {
    if (st < 0) return;
    uv_tcp_t *c = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
    if (!c) return;
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
    if (!srv) return NULL;
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