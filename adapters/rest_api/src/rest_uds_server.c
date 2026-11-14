/**
 * @file rest_uds_server.c
 * @brief Local management channel over a unix socket, routed through rest_dispatch_ex.
 */

#define _GNU_SOURCE

#include "rest_uds_server.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <uv.h>

#include "edge_log.h"
#include "rest_server.h"

typedef struct uds_client_s {
    uv_pipe_t handle;
    uv_write_t write;
    rest_uds_server_t *server;
    uint8_t *frame;
    size_t frame_len;
    size_t frame_cap;
    uint8_t *out;
    int responded;
    int closing;
} uds_client_t;

struct rest_uds_server_s {
    uv_pipe_t handle;
    service_t *svc;
    char path[128];
};

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void write_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
    p[2] = (uint8_t)((value >> 16) & 0xffu);
    p[3] = (uint8_t)((value >> 24) & 0xffu);
}

static int uid_allowed(uid_t uid) {
    if (uid == 0 || uid == geteuid()) return 1;
    const char *extra = getenv("EDGEC_APPCTL_ALLOW_UID");
    if (!extra || !extra[0]) return 0;
    char *end = NULL;
    long value = strtol(extra, &end, 10);
    return end && *end == '\0' && value >= 0 && (uid_t)value == uid;
}

static void on_client_close(uv_handle_t *handle) {
    uds_client_t *client = (uds_client_t *)handle->data;
    free(client->frame);
    free(client->out);
    free(client);
}

static void client_close(uds_client_t *client) {
    if (!client || client->closing) return;
    client->closing = 1;
    uv_read_stop((uv_stream_t *)&client->handle);
    uv_close((uv_handle_t *)&client->handle, on_client_close);
}

static int frame_reserve(uds_client_t *client, size_t extra) {
    size_t need = client->frame_len + extra;
    if (need <= client->frame_cap) return 0;
    size_t cap = client->frame_cap ? client->frame_cap : 4096;
    while (cap < need) cap *= 2;
    uint8_t *grown = realloc(client->frame, cap);
    if (!grown) return -1;
    client->frame = grown;
    client->frame_cap = cap;
    return 0;
}

static void on_write(uv_write_t *req, int status) {
    (void)status;
    client_close((uds_client_t *)req->handle->data);
}

static void respond_error(uds_client_t *client, int status, const char *reason) {
    char body[160];
    int n = snprintf(body, sizeof(body), "{\"status\":\"error\",\"reason\":\"%s\"}", reason);
    size_t body_len = n > 0 ? (size_t)n : 0;
    client->out = malloc(12 + body_len);
    if (!client->out) {
        client_close(client);
        return;
    }
    memcpy(client->out, REST_UDS_MAGIC, 4);
    write_u32(client->out + 4, (uint32_t)status);
    write_u32(client->out + 8, (uint32_t)body_len);
    memcpy(client->out + 12, body, body_len);
    client->responded = 1;
    uv_read_stop((uv_stream_t *)&client->handle);
    uv_buf_t buf = uv_buf_init((char *)client->out, (unsigned)(12 + body_len));
    uv_write(&client->write, (uv_stream_t *)&client->handle, &buf, 1, on_write);
}

static void handle_frame(uds_client_t *client, rest_uds_server_t *srv) {
    if (client->frame_len < 16) return;
    if (memcmp(client->frame, REST_UDS_MAGIC, 4) != 0) {
        client_close(client);
        return;
    }
    uint32_t method_len = read_u32(client->frame + 4);
    uint32_t path_len = read_u32(client->frame + 8);
    uint32_t body_len = read_u32(client->frame + 12);
    if (method_len == 0 || method_len >= REST_UDS_MAX_METHOD || path_len == 0 ||
        path_len >= REST_UDS_MAX_PATH || body_len > REST_UDS_MAX_BODY) {
        respond_error(client, 400, "bad_frame");
        return;
    }
    size_t total = 16 + (size_t)method_len + path_len + body_len;
    if (client->frame_len < total) return;
    if (client->frame_len > total) {
        respond_error(client, 400, "trailing_data");
        return;
    }

    char *method = malloc((size_t)method_len + 1);
    char *path = malloc((size_t)path_len + 1);
    char *body = body_len ? malloc((size_t)body_len + 1) : NULL;
    if (!method || !path || (body_len && !body)) {
        free(method);
        free(path);
        free(body);
        respond_error(client, 500, "out_of_memory");
        return;
    }
    const uint8_t *cursor = client->frame + 16;
    memcpy(method, cursor, method_len);
    method[method_len] = '\0';
    cursor += method_len;
    memcpy(path, cursor, path_len);
    path[path_len] = '\0';
    cursor += path_len;
    if (body_len) {
        memcpy(body, cursor, body_len);
        body[body_len] = '\0';
    }

    int status = 200;
    char *response = rest_dispatch_ex(srv->svc, method, path, body, &status);
    free(method);
    free(path);
    free(body);
    if (!response) {
        respond_error(client, 500, "no_response");
        return;
    }

    size_t response_len = strlen(response);
    client->out = malloc(12 + response_len);
    if (!client->out) {
        free(response);
        client_close(client);
        return;
    }
    memcpy(client->out, REST_UDS_MAGIC, 4);
    write_u32(client->out + 4, (uint32_t)status);
    write_u32(client->out + 8, (uint32_t)response_len);
    memcpy(client->out + 12, response, response_len);
    free(response);
    client->responded = 1;
    uv_read_stop((uv_stream_t *)&client->handle);
    uv_buf_t buf = uv_buf_init((char *)client->out, (unsigned)(12 + response_len));
    uv_write(&client->write, (uv_stream_t *)&client->handle, &buf, 1, on_write);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    uds_client_t *client = (uds_client_t *)stream->data;
    if (nread < 0) {
        free(buf->base);
        client_close(client);
        return;
    }
    if (nread == 0 || client->responded) {
        free(buf->base);
        return;
    }
    if (frame_reserve(client, (size_t)nread) != 0) {
        free(buf->base);
        respond_error(client, 413, "frame_too_large");
        return;
    }
    memcpy(client->frame + client->frame_len, buf->base, (size_t)nread);
    client->frame_len += (size_t)nread;
    free(buf->base);

    handle_frame(client, client->server);
}

static void on_alloc(uv_handle_t *handle, size_t suggested, uv_buf_t *buf) {
    (void)handle;
    (void)suggested;
    buf->base = malloc(65536);
    buf->len = buf->base ? 65536 : 0;
}

static void on_connect(uv_stream_t *server, int status) {
    if (status < 0) return;
    rest_uds_server_t *srv = (rest_uds_server_t *)server->data;
    uds_client_t *client = calloc(1, sizeof(*client));
    if (!client) return;
    client->server = srv;
    client->handle.data = client;
    uv_pipe_init(server->loop, &client->handle, 0);
    if (uv_accept(server, (uv_stream_t *)&client->handle) != 0) {
        client_close(client);
        return;
    }

    int fd = -1;
    struct ucred cred;
    socklen_t cred_len = sizeof(cred);
    if (uv_fileno((const uv_handle_t *)&client->handle, &fd) != 0 || fd < 0 ||
        getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) != 0) {
        LOG_WARN(LOG_CAT_CORE, "appctl: peer credential unavailable, connection rejected");
        client_close(client);
        return;
    }
    if (!uid_allowed(cred.uid)) {
        LOG_WARN(LOG_CAT_CORE, "appctl: uid %u is not allowed on %s", (unsigned)cred.uid, srv->path);
        client_close(client);
        return;
    }
    uv_read_start((uv_stream_t *)&client->handle, on_alloc, on_read);
}

rest_uds_server_t *rest_uds_server_start(uv_loop_t *loop, service_t *svc, const char *path) {
    if (!loop || !svc || !path || !path[0]) return NULL;
    rest_uds_server_t *srv = calloc(1, sizeof(*srv));
    if (!srv) return NULL;
    snprintf(srv->path, sizeof(srv->path), "%s", path);
    srv->svc = svc;
    uv_pipe_init(loop, &srv->handle, 0);
    srv->handle.data = srv;

    unlink(path);
    int rc = uv_pipe_bind(&srv->handle, path);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "appctl: bind %s failed: %s", path, uv_strerror(rc));
        uv_close((uv_handle_t *)&srv->handle, (uv_close_cb)free);
        return NULL;
    }
    rc = uv_listen((uv_stream_t *)&srv->handle, 16, on_connect);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "appctl: listen %s failed: %s", path, uv_strerror(rc));
        unlink(path);
        uv_close((uv_handle_t *)&srv->handle, (uv_close_cb)free);
        return NULL;
    }
    if (chmod(path, 0600) != 0) {
        LOG_WARN(LOG_CAT_CORE, "appctl: chmod %s failed", path);
    }
    return srv;
}

void rest_uds_server_stop(rest_uds_server_t *srv) {
    if (!srv) return;
    unlink(srv->path);
    uv_close((uv_handle_t *)&srv->handle, (uv_close_cb)free);
}
