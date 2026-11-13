/**
 * @file app_host_server.c
 * @brief Host side of the app ABI v1 over an inherited socketpair.
 */

#include "edge_app_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cjson/cJSON.h>
#include <uv.h>

typedef struct {
    uv_write_t req;
    char *data;
} write_req_t;

struct app_host_server_s {
    uv_pipe_t pipe;
    const app_host_ports_t *ports;
    uint8_t *buffer;
    size_t len;
    size_t cap;
    int closing;
    app_host_closed_cb_t on_closed;
    void *closed_ctx;
};

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void write_be32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)((value >> 24) & 0xffu);
    p[1] = (uint8_t)((value >> 16) & 0xffu);
    p[2] = (uint8_t)((value >> 8) & 0xffu);
    p[3] = (uint8_t)(value & 0xffu);
}

static const char *access_name(uint8_t access) {
    return access == APP_HOST_ACCESS_PUBLISH ? "publish" : "read";
}

static const char *quality_name(uint8_t quality) {
    switch (quality) {
    case APP_HOST_QUALITY_BAD: return "bad";
    case APP_HOST_QUALITY_UNCERTAIN: return "uncertain";
    default: return "good";
    }
}

static int parse_quality(const char *text, uint8_t *out) {
    if (strcmp(text, "good") == 0) {
        *out = APP_HOST_QUALITY_GOOD;
        return 0;
    }
    if (strcmp(text, "bad") == 0) {
        *out = APP_HOST_QUALITY_BAD;
        return 0;
    }
    if (strcmp(text, "uncertain") == 0) {
        *out = APP_HOST_QUALITY_UNCERTAIN;
        return 0;
    }
    return -1;
}

static int valid_error_code(const char *code) {
    static const char *known[] = {
        "denied_not_granted", "denied_scope",     "denied_expired", "denied_revoked",
        "denied_policy",      "denied_quota",     "denied_package_mismatch",
        "denied_abi",         "invalid_manifest", "not_found",      "stale",
        "busy",               "invalid",          "internal",       "protocol",
        "transport",          NULL};
    for (int i = 0; known[i]; i++) {
        if (strcmp(code, known[i]) == 0) return 1;
    }
    return 0;
}

static int json_escape(const char *text, char *out, size_t cap) {
    size_t used = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        const char *rep = NULL;
        char uni[8];
        switch (*p) {
            case '"': rep = "\\\""; break;
            case '\\': rep = "\\\\"; break;
            case '\n': rep = "\\n"; break;
            case '\r': rep = "\\r"; break;
            case '\t': rep = "\\t"; break;
            default:
                if (*p < 0x20) {
                    snprintf(uni, sizeof(uni), "\\u%04x", *p);
                    rep = uni;
                }
                break;
        }
        if (rep) {
            size_t n = strlen(rep);
            if (used + n + 1 > cap) return -1;
            memcpy(out + used, rep, n);
            used += n;
        } else {
            if (used + 2 > cap) return -1;
            out[used++] = (char)*p;
        }
    }
    out[used] = '\0';
    return 0;
}

static void on_write(uv_write_t *req, int status) {
    (void)status;
    write_req_t *write = (write_req_t *)req;
    free(write->data);
    free(write);
}

static void send_json(app_host_server_t *srv, const char *json) {
    size_t len = strlen(json);
    write_req_t *write = calloc(1, sizeof(*write));
    if (!write) return;
    write->data = malloc(len + 4);
    if (!write->data) {
        free(write);
        return;
    }
    write_be32((uint8_t *)write->data, (uint32_t)len);
    memcpy(write->data + 4, json, len);
    uv_buf_t buf = uv_buf_init(write->data, (unsigned)(len + 4));
    if (uv_write(&write->req, (uv_stream_t *)&srv->pipe, &buf, 1, on_write) != 0) {
        free(write->data);
        free(write);
    }
}

static void send_error(app_host_server_t *srv, uint64_t id, const char *code) {
    if (!code || !valid_error_code(code)) code = "internal";
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"id\":%llu,\"ok\":false,\"code\":\"%s\"}",
             (unsigned long long)id, code);
    send_json(srv, buffer);
}

static int sample_value_json(const app_host_sample_t *sample, char *out, size_t cap) {
    switch (sample->kind) {
        case APP_HOST_VALUE_FLOAT:
            return snprintf(out, cap, "{\"float\":%.17g}", sample->number);
        case APP_HOST_VALUE_INTEGER:
            return snprintf(out, cap, "{\"integer\":%lld}", (long long)sample->integer);
        case APP_HOST_VALUE_BOOLEAN:
            return snprintf(out, cap, "{\"boolean\":%s}", sample->boolean ? "true" : "false");
        case APP_HOST_VALUE_TEXT: {
            char escaped[APP_HOST_TEXT_SIZE * 2];
            if (json_escape(sample->text, escaped, sizeof(escaped)) != 0) return -1;
            return snprintf(out, cap, "{\"text\":\"%s\"}", escaped);
        }
        default:
            return -1;
    }
}

static int parse_sample(cJSON *sample_json, app_host_sample_t *out) {
    if (!cJSON_IsObject(sample_json)) return -1;
    memset(out, 0, sizeof(*out));
    out->quality = APP_HOST_QUALITY_GOOD;

    cJSON *value = cJSON_GetObjectItemCaseSensitive(sample_json, "value");
    if (!cJSON_IsObject(value)) return -1;
    cJSON *item = cJSON_GetObjectItemCaseSensitive(value, "float");
    if (cJSON_IsNumber(item)) {
        out->kind = APP_HOST_VALUE_FLOAT;
        out->number = item->valuedouble;
    } else if ((item = cJSON_GetObjectItemCaseSensitive(value, "integer")) && cJSON_IsNumber(item)) {
        out->kind = APP_HOST_VALUE_INTEGER;
        out->integer = (int64_t)item->valuedouble;
        out->number = (double)out->integer;
    } else if ((item = cJSON_GetObjectItemCaseSensitive(value, "boolean")) && cJSON_IsBool(item)) {
        out->kind = APP_HOST_VALUE_BOOLEAN;
        out->boolean = cJSON_IsTrue(item);
    } else if ((item = cJSON_GetObjectItemCaseSensitive(value, "text")) && cJSON_IsString(item)) {
        out->kind = APP_HOST_VALUE_TEXT;
        snprintf(out->text, sizeof(out->text), "%s", item->valuestring);
    } else {
        return -1;
    }

    cJSON *quality = cJSON_GetObjectItemCaseSensitive(sample_json, "quality");
    if (cJSON_IsString(quality) && parse_quality(quality->valuestring, &out->quality) != 0) {
        return -1;
    }
    cJSON *source_ts = cJSON_GetObjectItemCaseSensitive(sample_json, "source_ts");
    if (cJSON_IsNumber(source_ts)) out->source_ts = (uint64_t)source_ts->valuedouble;
    return 0;
}

static void handle_resolve(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *name = args ? cJSON_GetObjectItemCaseSensitive(args, "name") : NULL;
    if (!cJSON_IsString(name) || !name->valuestring[0] || !srv->ports->resolve) {
        send_error(srv, id, "invalid");
        return;
    }
    uint32_t handle = 0;
    uint8_t access = APP_HOST_ACCESS_READ;
    char reason[APP_HOST_REASON_SIZE] = "";
    if (srv->ports->resolve(srv->ports->ctx, name->valuestring, &handle, &access, reason,
                            sizeof(reason)) != 0) {
        send_error(srv, id, reason);
        return;
    }
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "{\"id\":%llu,\"ok\":true,\"result\":{\"handle\":%u,\"access\":\"%s\"}}",
             (unsigned long long)id, handle, access_name(access));
    send_json(srv, buffer);
}

static void handle_read(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *handle = args ? cJSON_GetObjectItemCaseSensitive(args, "handle") : NULL;
    if (!cJSON_IsNumber(handle) || !srv->ports->read) {
        send_error(srv, id, "invalid");
        return;
    }
    app_host_sample_t sample;
    char reason[APP_HOST_REASON_SIZE] = "";
    if (srv->ports->read(srv->ports->ctx, (uint32_t)handle->valuedouble, &sample, reason,
                         sizeof(reason)) != 0) {
        send_error(srv, id, reason);
        return;
    }
    char value[256];
    if (sample_value_json(&sample, value, sizeof(value)) < 0) {
        send_error(srv, id, "internal");
        return;
    }
    char buffer[512];
    snprintf(buffer, sizeof(buffer),
             "{\"id\":%llu,\"ok\":true,\"result\":{\"value\":%s,\"quality\":\"%s\","
             "\"source_ts\":%llu}}",
             (unsigned long long)id, value, quality_name(sample.quality),
             (unsigned long long)sample.source_ts);
    send_json(srv, buffer);
}

static void handle_publish(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *handle = args ? cJSON_GetObjectItemCaseSensitive(args, "handle") : NULL;
    cJSON *sample_json = args ? cJSON_GetObjectItemCaseSensitive(args, "sample") : NULL;
    if (!cJSON_IsNumber(handle) || !srv->ports->publish) {
        send_error(srv, id, "invalid");
        return;
    }
    app_host_sample_t sample;
    if (parse_sample(sample_json, &sample) != 0) {
        send_error(srv, id, "invalid");
        return;
    }
    char reason[APP_HOST_REASON_SIZE] = "";
    if (srv->ports->publish(srv->ports->ctx, (uint32_t)handle->valuedouble, &sample, reason,
                            sizeof(reason)) != 0) {
        send_error(srv, id, reason);
        return;
    }
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "{\"id\":%llu,\"ok\":true,\"result\":{}}",
             (unsigned long long)id);
    send_json(srv, buffer);
}

static void handle_log(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *level = args ? cJSON_GetObjectItemCaseSensitive(args, "level") : NULL;
    cJSON *message = args ? cJSON_GetObjectItemCaseSensitive(args, "message") : NULL;
    if (!cJSON_IsString(level) || !cJSON_IsString(message) || !srv->ports->log) {
        send_error(srv, id, "invalid");
        return;
    }
    static const char *levels[] = {"debug", "info", "warn", "error"};
    int value = -1;
    for (int i = 0; i < 4; i++) {
        if (strcmp(level->valuestring, levels[i]) == 0) value = i;
    }
    if (value < 0) {
        send_error(srv, id, "invalid");
        return;
    }
    srv->ports->log(srv->ports->ctx, value, message->valuestring);
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "{\"id\":%llu,\"ok\":true,\"result\":{}}",
             (unsigned long long)id);
    send_json(srv, buffer);
}

static void handle_now(app_host_server_t *srv, uint64_t id) {
    uint64_t now = srv->ports->now_ms ? srv->ports->now_ms(srv->ports->ctx) : 0;
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "{\"id\":%llu,\"ok\":true,\"result\":{\"now_ms\":%llu}}",
             (unsigned long long)id, (unsigned long long)now);
    send_json(srv, buffer);
}

static void handle_frame(app_host_server_t *srv, const char *json, size_t len) {
    char *copy = malloc(len + 1);
    if (!copy) return;
    memcpy(copy, json, len);
    copy[len] = '\0';
    cJSON *root = cJSON_Parse(copy);
    free(copy);
    if (!root) {
        send_error(srv, 0, "invalid");
        return;
    }

    cJSON *id_item = cJSON_GetObjectItemCaseSensitive(root, "id");
    uint64_t id = cJSON_IsNumber(id_item) ? (uint64_t)id_item->valuedouble : 0;
    cJSON *fn = cJSON_GetObjectItemCaseSensitive(root, "fn");
    cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    if (!cJSON_IsString(fn) || !fn->valuestring) {
        send_error(srv, id, "invalid");
    } else if (strcmp(fn->valuestring, "point_resolve") == 0) {
        handle_resolve(srv, id, args);
    } else if (strcmp(fn->valuestring, "point_read") == 0) {
        handle_read(srv, id, args);
    } else if (strcmp(fn->valuestring, "point_publish") == 0) {
        handle_publish(srv, id, args);
    } else if (strcmp(fn->valuestring, "log") == 0) {
        handle_log(srv, id, args);
    } else if (strcmp(fn->valuestring, "now") == 0) {
        handle_now(srv, id);
    } else if (strcmp(fn->valuestring, "point_subscribe") == 0) {
        send_error(srv, id, "internal");
    } else {
        send_error(srv, id, "invalid");
    }
    cJSON_Delete(root);
}

static void on_alloc(uv_handle_t *handle, size_t suggested, uv_buf_t *buf) {
    (void)handle;
    (void)suggested;
    buf->base = malloc(65536);
    buf->len = buf->base ? 65536 : 0;
}

static void on_close(uv_handle_t *handle) {
    app_host_server_t *srv = (app_host_server_t *)handle->data;
    app_host_closed_cb_t on_closed = srv->on_closed;
    void *closed_ctx = srv->closed_ctx;
    free(srv->buffer);
    free(srv);
    if (on_closed) on_closed(closed_ctx);
}

static void server_shutdown(app_host_server_t *srv) {
    if (!srv || srv->closing) return;
    srv->closing = 1;
    uv_read_stop((uv_stream_t *)&srv->pipe);
    uv_close((uv_handle_t *)&srv->pipe, on_close);
}

static void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    app_host_server_t *srv = (app_host_server_t *)stream->data;
    if (nread < 0) {
        free(buf->base);
        server_shutdown(srv);
        return;
    }
    if (nread == 0) {
        free(buf->base);
        return;
    }
    if (srv->len + (size_t)nread > APP_HOST_MAX_FRAME + 4) {
        free(buf->base);
        server_shutdown(srv);
        return;
    }
    if (srv->len + (size_t)nread > srv->cap) {
        size_t cap = srv->cap ? srv->cap : 4096;
        while (cap < srv->len + (size_t)nread) cap *= 2;
        uint8_t *grown = realloc(srv->buffer, cap);
        if (!grown) {
            free(buf->base);
            server_shutdown(srv);
            return;
        }
        srv->buffer = grown;
        srv->cap = cap;
    }
    memcpy(srv->buffer + srv->len, buf->base, (size_t)nread);
    srv->len += (size_t)nread;
    free(buf->base);

    size_t offset = 0;
    while (srv->len - offset >= 4) {
        uint32_t frame_len = read_be32(srv->buffer + offset);
        if (frame_len > APP_HOST_MAX_FRAME) {
            server_shutdown(srv);
            return;
        }
        if (srv->len - offset < 4 + (size_t)frame_len) break;
        handle_frame(srv, (const char *)srv->buffer + offset + 4, frame_len);
        offset += 4 + (size_t)frame_len;
    }
    if (offset > 0) {
        memmove(srv->buffer, srv->buffer + offset, srv->len - offset);
        srv->len -= offset;
    }
}

app_host_server_t *app_host_server_create_ex(uv_loop_t *loop, int fd,
                                             const app_host_ports_t *ports,
                                             app_host_closed_cb_t on_closed, void *closed_ctx) {
    if (!loop || fd < 0 || !ports) return NULL;
    app_host_server_t *srv = calloc(1, sizeof(*srv));
    if (!srv) return NULL;
    srv->ports = ports;
    srv->on_closed = on_closed;
    srv->closed_ctx = closed_ctx;
    if (uv_pipe_init(loop, &srv->pipe, 0) != 0) {
        free(srv);
        return NULL;
    }
    srv->pipe.data = srv;
    if (uv_pipe_open(&srv->pipe, fd) != 0) {
        close(fd);
        free(srv);
        return NULL;
    }
    if (uv_read_start((uv_stream_t *)&srv->pipe, on_alloc, on_read) != 0) {
        srv->on_closed = NULL;
        srv->closed_ctx = NULL;
        server_shutdown(srv);
        return NULL;
    }
    return srv;
}

app_host_server_t *app_host_server_create(uv_loop_t *loop, int fd, const app_host_ports_t *ports) {
    return app_host_server_create_ex(loop, fd, ports, NULL, NULL);
}

void app_host_server_destroy(app_host_server_t *srv) {
    server_shutdown(srv);
}
