#define _POSIX_C_SOURCE 200809L

#include "edge_app_sdk.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct edge_app_sdk {
    int fd;
    uint64_t next_id;
    edge_app_init_fn init;
    edge_app_event_fn on_event;
    edge_app_stop_fn stop;
    void *ctx;
    char tx[8192];
    char rx[4096];
    char result[4096];
    char args[4096];
    char value[512];
    char access[16];
    char quality[16];
    char error[64];
};

static const char *set_error(edge_app_sdk_t *sdk, const char *code) {
    snprintf(sdk->error, sizeof sdk->error, "%s", code);
    return sdk->error;
}

static int write_all(int fd, const void *buf, size_t len) {
    const unsigned char *p = buf;
    while (len > 0) {
        ssize_t n = write(fd, p, len);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t len) {
    unsigned char *p = buf;
    while (len > 0) {
        ssize_t n = read(fd, p, len);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

static int send_frame(int fd, const char *json) {
    size_t len = strlen(json);
    if (len > 0xffffffffu) return -1;
    uint32_t prefix = htonl((uint32_t)len);
    if (write_all(fd, &prefix, sizeof prefix) != 0) return -1;
    return write_all(fd, json, len);
}

static int recv_frame(int fd, char *out, size_t cap) {
    uint32_t prefix;
    if (read_all(fd, &prefix, sizeof prefix) != 0) return -1;
    uint32_t len = ntohl(prefix);
    if ((size_t)len + 1 > cap) return -1;
    if (read_all(fd, out, len) != 0) return -1;
    out[len] = '\0';
    return (int)len;
}

/* Field scan over a trusted host response. Shortcut: keys are matched anywhere,
 * which is enough while every response is produced by our own host. */
static const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

static const char *find_key(const char *json, const char *key) {
    char pat[64];
    int n = snprintf(pat, sizeof pat, "\"%s\"", key);
    if (n < 0 || (size_t)n >= sizeof pat) return NULL;
    const char *p = json;
    while ((p = strstr(p, pat)) != NULL) {
        const char *q = skip_ws(p + n);
        if (*q == ':') return skip_ws(q + 1);
        p += n;
    }
    return NULL;
}

/* Shortcut: ASCII only; \u escapes degrade to '?'. Upgrade when point names or
 * log text become non-ASCII. */
static int parse_string(const char **pp, char *out, size_t cap) {
    const char *p = *pp;
    if (*p != '"') return -1;
    p++;
    size_t used = 0;
    while (*p != '\0' && *p != '"') {
        char c = *p++;
        if (c == '\\') {
            char esc = *p++;
            switch (esc) {
            case '"': c = '"'; break;
            case '\\': c = '\\'; break;
            case '/': c = '/'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'u':
                for (int i = 0; i < 4 && *p != '\0'; i++) p++;
                c = '?';
                break;
            default: return -1;
            }
        }
        if (used + 1 >= cap) return -1;
        out[used++] = c;
    }
    if (*p != '"') return -1;
    out[used] = '\0';
    *pp = p + 1;
    return 0;
}

static int json_u64_field(const char *json, const char *key, uint64_t *out) {
    const char *p = find_key(json, key);
    if (p == NULL) return -1;
    char *end = NULL;
    errno = 0;
    unsigned long long v = strtoull(p, &end, 10);
    if (end == p || errno != 0) return -1;
    *out = (uint64_t)v;
    return 0;
}

static int json_string_field(const char *json, const char *key, char *out,
                             size_t cap) {
    const char *p = find_key(json, key);
    if (p == NULL) return -1;
    return parse_string(&p, out, cap);
}

static int copy_json_object(const char *json, const char *key, char *out,
                            size_t cap) {
    const char *p = find_key(json, key);
    if (p == NULL || *p != '{') return -1;
    int depth = 0;
    int in_string = 0;
    int escaped = 0;
    for (const char *q = p; *q != '\0'; q++) {
        char c = *q;
        if (in_string) {
            if (escaped) escaped = 0;
            else if (c == '\\') escaped = 1;
            else if (c == '"') in_string = 0;
            continue;
        }
        if (c == '"') in_string = 1;
        else if (c == '{') depth++;
        else if (c == '}' && --depth == 0) {
            size_t len = (size_t)(q - p + 1);
            if (len + 1 > cap) return -1;
            memcpy(out, p, len);
            out[len] = '\0';
            return 0;
        }
    }
    return -1;
}

static int append_escaped(char *out, size_t cap, const char *s) {
    size_t used = 0;
    for (; *s != '\0'; s++) {
        unsigned char c = (unsigned char)*s;
        const char *rep = NULL;
        char uni[8];
        switch (c) {
        case '"': rep = "\\\""; break;
        case '\\': rep = "\\\\"; break;
        case '\n': rep = "\\n"; break;
        case '\r': rep = "\\r"; break;
        case '\t': rep = "\\t"; break;
        default: break;
        }
        if (rep != NULL) {
            size_t n = strlen(rep);
            if (used + n >= cap) return -1;
            memcpy(out + used, rep, n);
            used += n;
        } else if (c < 0x20) {
            int n = snprintf(uni, sizeof uni, "\\u%04x", c);
            if (n < 0 || used + (size_t)n >= cap) return -1;
            memcpy(out + used, uni, (size_t)n);
            used += (size_t)n;
        } else {
            if (used + 1 >= cap) return -1;
            out[used++] = (char)c;
        }
    }
    out[used] = '\0';
    return 0;
}

static int valid_fn(const char *fn) {
    if (*fn == '\0') return 0;
    for (; *fn != '\0'; fn++) {
        char c = *fn;
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
            return 0;
        }
    }
    return 1;
}

static int format_sample(const edge_app_sample_t *sample, char *out,
                         size_t cap) {
    char value[128];
    int n;
    switch (sample->value.kind) {
    case EDGE_APP_VALUE_FLOAT:
        n = snprintf(value, sizeof value, "{\"float\":%.17g}", sample->value.number);
        break;
    case EDGE_APP_VALUE_INTEGER:
        n = snprintf(value, sizeof value, "{\"integer\":%lld}",
                     (long long)sample->value.integer);
        break;
    case EDGE_APP_VALUE_BOOLEAN:
        n = snprintf(value, sizeof value, "{\"boolean\":%s}",
                     sample->value.boolean ? "true" : "false");
        break;
    case EDGE_APP_VALUE_TEXT: {
        char escaped[128];
        if (append_escaped(escaped, sizeof escaped, sample->value.text) != 0) {
            return -1;
        }
        n = snprintf(value, sizeof value, "{\"text\":\"%s\"}", escaped);
        break;
    }
    default:
        return -1;
    }
    if (n < 0 || (size_t)n >= sizeof value) return -1;
    const char *quality = sample->quality != NULL ? sample->quality : "good";
    n = snprintf(out, cap,
                 "{\"value\":%s,\"quality\":\"%s\",\"source_ts\":%llu}", value,
                 quality, (unsigned long long)sample->source_ts);
    return (n < 0 || (size_t)n >= cap) ? -1 : 0;
}

static int parse_value(const char *json, edge_app_value_t *value) {
    char *end = NULL;
    const char *p = find_key(json, "float");
    if (p != NULL) {
        errno = 0;
        value->kind = EDGE_APP_VALUE_FLOAT;
        value->number = strtod(p, &end);
        return (end == p || errno != 0) ? -1 : 0;
    }
    p = find_key(json, "integer");
    if (p != NULL) {
        errno = 0;
        value->kind = EDGE_APP_VALUE_INTEGER;
        value->integer = strtoll(p, &end, 10);
        value->number = (double)value->integer;
        return (end == p || errno != 0) ? -1 : 0;
    }
    p = find_key(json, "boolean");
    if (p != NULL) {
        value->kind = EDGE_APP_VALUE_BOOLEAN;
        if (strncmp(p, "true", 4) == 0) value->boolean = true;
        else if (strncmp(p, "false", 5) == 0) value->boolean = false;
        else return -1;
        return 0;
    }
    p = find_key(json, "text");
    if (p != NULL) {
        value->kind = EDGE_APP_VALUE_TEXT;
        return parse_string(&p, value->text, sizeof value->text);
    }
    return -1;
}

edge_app_sdk_t *edge_app_sdk_create(int fd) {
    edge_app_sdk_t *sdk = calloc(1, sizeof *sdk);
    if (sdk == NULL) return NULL;
    sdk->fd = fd;
    sdk->next_id = 1;
    return sdk;
}

void edge_app_sdk_destroy(edge_app_sdk_t *sdk) {
    if (sdk == NULL) return;
    close(sdk->fd);
    free(sdk);
}

void edge_app_sdk_set_exports(edge_app_sdk_t *sdk, edge_app_init_fn init,
                              edge_app_event_fn on_event, edge_app_stop_fn stop,
                              void *ctx) {
    sdk->init = init;
    sdk->on_event = on_event;
    sdk->stop = stop;
    sdk->ctx = ctx;
}

int edge_app_sdk_dispatch(edge_app_sdk_t *sdk, const char *request_json,
                          char *response_json, size_t response_cap) {
    uint64_t id = 0;
    (void)json_u64_field(request_json, "id", &id);

    char fn[32];
    const char *fn_pos = find_key(request_json, "fn");
    if (fn_pos == NULL || parse_string(&fn_pos, fn, sizeof fn) != 0) return -1;

    int handled = 1;
    if (strcmp(fn, "app_init") == 0) {
        if (sdk->init != NULL) sdk->init(sdk->ctx);
    } else if (strcmp(fn, "app_on_event") == 0) {
        if (copy_json_object(request_json, "args", sdk->args, sizeof sdk->args) != 0) {
            return -1;
        }
        if (sdk->on_event != NULL) sdk->on_event(sdk->ctx, sdk->args);
    } else if (strcmp(fn, "app_stop") == 0) {
        if (sdk->stop != NULL) sdk->stop(sdk->ctx);
    } else {
        handled = 0;
    }

    int n;
    if (handled) {
        n = snprintf(response_json, response_cap,
                     "{\"id\":%llu,\"ok\":true,\"result\":{}}",
                     (unsigned long long)id);
    } else {
        n = snprintf(response_json, response_cap,
                     "{\"id\":%llu,\"ok\":false,\"code\":\"not_found\"}",
                     (unsigned long long)id);
    }
    return (n < 0 || (size_t)n >= response_cap) ? -1 : 0;
}

const char *edge_app_sdk_call(edge_app_sdk_t *sdk, const char *fn,
                              const char *args_json, char *result_json,
                              size_t result_cap) {
    if (!valid_fn(fn)) return set_error(sdk, "protocol");
    uint64_t id = sdk->next_id++;
    int n = snprintf(sdk->tx, sizeof sdk->tx,
                     "{\"id\":%llu,\"fn\":\"%s\",\"args\":%s}",
                     (unsigned long long)id, fn, args_json);
    if (n < 0 || (size_t)n >= sizeof sdk->tx) return set_error(sdk, "protocol");
    if (send_frame(sdk->fd, sdk->tx) != 0) return set_error(sdk, "transport");
    if (recv_frame(sdk->fd, sdk->rx, sizeof sdk->rx) < 0) {
        return set_error(sdk, "transport");
    }

    uint64_t response_id = 0;
    if (json_u64_field(sdk->rx, "id", &response_id) != 0 || response_id != id) {
        return set_error(sdk, "protocol");
    }
    const char *ok = find_key(sdk->rx, "ok");
    if (ok == NULL) return set_error(sdk, "protocol");
    if (strncmp(ok, "true", 4) == 0) {
        if (copy_json_object(sdk->rx, "result", result_json, result_cap) != 0) {
            return set_error(sdk, "protocol");
        }
        return NULL;
    }
    if (strncmp(ok, "false", 5) != 0) return set_error(sdk, "protocol");

    char code[64];
    if (json_string_field(sdk->rx, "code", code, sizeof code) != 0) {
        return set_error(sdk, "protocol");
    }
    return set_error(sdk, code);
}

const char *edge_app_point_resolve(edge_app_sdk_t *sdk, const char *name,
                                   uint32_t *handle, const char **access) {
    char escaped[512];
    if (append_escaped(escaped, sizeof escaped, name) != 0) {
        return set_error(sdk, "protocol");
    }
    int n = snprintf(sdk->args, sizeof sdk->args, "{\"name\":\"%s\"}", escaped);
    if (n < 0 || (size_t)n >= sizeof sdk->args) return set_error(sdk, "protocol");

    const char *err = edge_app_sdk_call(sdk, "point_resolve", sdk->args,
                                        sdk->result, sizeof sdk->result);
    if (err != NULL) return err;

    uint64_t value = 0;
    if (json_u64_field(sdk->result, "handle", &value) != 0 ||
        json_string_field(sdk->result, "access", sdk->access,
                          sizeof sdk->access) != 0) {
        return set_error(sdk, "protocol");
    }
    *handle = (uint32_t)value;
    *access = sdk->access;
    return NULL;
}

const char *edge_app_point_read(edge_app_sdk_t *sdk, uint32_t handle,
                                edge_app_sample_t *sample) {
    int n = snprintf(sdk->args, sizeof sdk->args, "{\"handle\":%u}", handle);
    if (n < 0 || (size_t)n >= sizeof sdk->args) return set_error(sdk, "protocol");

    const char *err = edge_app_sdk_call(sdk, "point_read", sdk->args,
                                        sdk->result, sizeof sdk->result);
    if (err != NULL) return err;

    if (copy_json_object(sdk->result, "value", sdk->value, sizeof sdk->value) != 0 ||
        parse_value(sdk->value, &sample->value) != 0 ||
        json_string_field(sdk->result, "quality", sdk->quality,
                          sizeof sdk->quality) != 0) {
        return set_error(sdk, "protocol");
    }
    uint64_t source_ts = 0;
    (void)json_u64_field(sdk->result, "source_ts", &source_ts);
    sample->quality = sdk->quality;
    sample->source_ts = source_ts;
    return NULL;
}

const char *edge_app_point_subscribe(edge_app_sdk_t *sdk, uint32_t handle,
                                     double deadband, uint32_t min_interval_ms,
                                     uint32_t max_interval_ms,
                                     uint32_t *subscription) {
    int n = snprintf(sdk->args, sizeof sdk->args,
                     "{\"handle\":%u,\"deadband\":%.17g,\"min_interval_ms\":%u,"
                     "\"max_interval_ms\":%u}",
                     handle, deadband, min_interval_ms, max_interval_ms);
    if (n < 0 || (size_t)n >= sizeof sdk->args) return set_error(sdk, "protocol");

    const char *err = edge_app_sdk_call(sdk, "point_subscribe", sdk->args,
                                        sdk->result, sizeof sdk->result);
    if (err != NULL) return err;

    uint64_t value = 0;
    if (json_u64_field(sdk->result, "subscription", &value) != 0) {
        return set_error(sdk, "protocol");
    }
    *subscription = (uint32_t)value;
    return NULL;
}

const char *edge_app_point_publish(edge_app_sdk_t *sdk, uint32_t handle,
                                   const edge_app_sample_t *sample) {
    char sample_json[512];
    if (format_sample(sample, sample_json, sizeof sample_json) != 0) {
        return set_error(sdk, "protocol");
    }
    int n = snprintf(sdk->args, sizeof sdk->args,
                     "{\"handle\":%u,\"sample\":%s}", handle, sample_json);
    if (n < 0 || (size_t)n >= sizeof sdk->args) return set_error(sdk, "protocol");

    return edge_app_sdk_call(sdk, "point_publish", sdk->args, sdk->result,
                             sizeof sdk->result);
}

const char *edge_app_log(edge_app_sdk_t *sdk, const char *level,
                         const char *message) {
    char level_json[32];
    char message_json[512];
    if (append_escaped(level_json, sizeof level_json, level) != 0 ||
        append_escaped(message_json, sizeof message_json, message) != 0) {
        return set_error(sdk, "protocol");
    }
    int n = snprintf(sdk->args, sizeof sdk->args,
                     "{\"level\":\"%s\",\"message\":\"%s\"}", level_json,
                     message_json);
    if (n < 0 || (size_t)n >= sizeof sdk->args) return set_error(sdk, "protocol");

    return edge_app_sdk_call(sdk, "log", sdk->args, sdk->result,
                             sizeof sdk->result);
}

const char *edge_app_now(edge_app_sdk_t *sdk, uint64_t *now_ms) {
    const char *err = edge_app_sdk_call(sdk, "now", "{}", sdk->result,
                                        sizeof sdk->result);
    if (err != NULL) return err;

    uint64_t value = 0;
    if (json_u64_field(sdk->result, "now_ms", &value) != 0) {
        return set_error(sdk, "protocol");
    }
    *now_ms = value;
    return NULL;
}
