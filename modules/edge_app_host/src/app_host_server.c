/**
 * @file app_host_server.c
 * @brief Host side of the app ABI v1 over an inherited socketpair.
 */

#include "edge_app_host.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cjson/cJSON.h>
#include <uv.h>

#define APP_HOST_MAX_SUBSCRIPTIONS 64
#define APP_HOST_EVENT_QUEUE_CAP 256
#define APP_HOST_EVENT_KIND_SIZE 32
#define APP_HOST_EVENT_PAYLOAD_SIZE 128
#define APP_HOST_ACK_TIMEOUT_MS 5000
/* End-to-end cap on one serialized event frame. A builder returns -1 for a
 * frame that would exceed it, so the frame never reaches the write queue and
 * callers never mark it pending. */
#define APP_HOST_EVENT_FRAME_CAP 4096
/* shortcut: 2KB setting values; raise with the per-field size cap if apps need more. */
#define APP_HOST_SETTING_SIZE 2048

typedef struct {
    uv_write_t req;
    char *data;
} write_req_t;

/* One active wire subscription. Owner: app_host_server_t, loop thread only. */
typedef struct {
    int32_t hub_sub_id; /**< 0 = free slot; > 0 = opaque product token */
    uint32_t handle;    /**< app-facing point handle; never 0 while active */
    uint8_t has_last;
    app_host_sample_t last;
    double deadband;
    uint32_t min_interval_ms;
    uint32_t max_interval_ms;
    uint64_t last_send_ms;
} app_host_subscription_t;

/* One queued event. Mutex-protected. Point events merge by handle; platform
 * events never merge and belong to no subscription. */
typedef struct {
    uint8_t platform;
    uint8_t keepalive;
    uint32_t handle;
    union {
        app_host_sample_t sample;
        struct {
            char kind[APP_HOST_EVENT_KIND_SIZE];
            char payload[APP_HOST_EVENT_PAYLOAD_SIZE];
        } platform_event;
    } data;
} app_host_queued_event_t;

enum {
    APP_HOST_HANDLE_PIPE = 1u << 0,
    APP_HOST_HANDLE_WAKE = 1u << 1,
    APP_HOST_HANDLE_TIMER = 1u << 2,
    APP_HOST_HANDLE_TICK = 1u << 3,
};

struct app_host_server_s {
    uv_pipe_t pipe;
    uv_async_t wake;
    uv_timer_t keepalive;
    uv_timer_t tick;
    const app_host_ports_t *ports;
    uint8_t *buffer;
    size_t len;
    size_t cap;
    int closing; /* written under queue_lock by the loop thread */
    uint8_t settings_watched; /* written under queue_lock by the loop thread */
    app_host_closed_cb_t on_closed;
    void *closed_ctx;
    uint64_t rate_window_ms;
    uint32_t calls_in_window;
    uint32_t publishes_in_window;

    /* Loop-owned event state. */
    uv_loop_t *loop;
    app_host_subscription_t subs[APP_HOST_MAX_SUBSCRIPTIONS];
    uint64_t next_event_id;
    uint64_t pending_event_id; /* 0 = nothing in flight */
    uint64_t pending_since_ms; /* uv_now when pending_event_id was sent */
    uint64_t ack_timeouts;     /* events dropped for a missing ack */
    uint64_t oversize_frames;  /* event frames dropped for exceeding the cap */
    uint8_t pending_tick;      /* the in-flight event is a tick */
    char pending_kind[APP_HOST_EVENT_KIND_SIZE]; /* platform kind in flight, "" for point */
    app_host_event_ack_cb_t event_ack_cb;
    void *event_ack_ctx;
    uint32_t tick_ms;
    uint64_t ticks_enqueued;
    uint64_t ticks_skipped;
    uint8_t handles_open;
    int closes_outstanding;
    /* Reused buffers for setting_get: no per-call allocation on the loop. */
    char setting_value[APP_HOST_SETTING_SIZE];
    char setting_frame[APP_HOST_SETTING_SIZE + 128];

    /* Written by the publisher's thread under queue_lock. */
    uv_mutex_t queue_lock;
    app_host_queued_event_t queue[APP_HOST_EVENT_QUEUE_CAP];
    size_t queue_head;
    size_t queue_count;
    uint64_t dropped_events;
};

_Static_assert(APP_HOST_EVENT_QUEUE_CAP >= APP_HOST_MAX_SUBSCRIPTIONS,
               "merge-by-handle bounds the queue to the active subscription count");

static void server_drain(app_host_server_t *srv);
static void keepalive_rearm(app_host_server_t *srv);

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
        "transport",          "rate_limited",     "not_supported",  "not_set",
        "too_large",          NULL};
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

static int send_json(app_host_server_t *srv, const char *json) {
    size_t len = strlen(json);
    write_req_t *write = calloc(1, sizeof(*write));
    if (!write) return -1;
    write->data = malloc(len + 4);
    if (!write->data) {
        free(write);
        return -1;
    }
    write_be32((uint8_t *)write->data, (uint32_t)len);
    memcpy(write->data + 4, json, len);
    uv_buf_t buf = uv_buf_init(write->data, (unsigned)(len + 4));
    if (uv_write(&write->req, (uv_stream_t *)&srv->pipe, &buf, 1, on_write) != 0) {
        free(write->data);
        free(write);
        return -1;
    }
    return 0;
}

static void send_error(app_host_server_t *srv, uint64_t id, const char *code) {
    if (!code || !valid_error_code(code)) code = "internal";
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"id\":%llu,\"ok\":false,\"code\":\"%s\"}",
             (unsigned long long)id, code);
    (void)send_json(srv, buffer);
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

static int json_u32(cJSON *item, uint32_t *out) {
    if (item == NULL || !cJSON_IsNumber(item)) return -1;
    double value = item->valuedouble;
    if (!(value >= 0.0) || value > 4294967295.0) return -1;
    uint32_t parsed = (uint32_t)value;
    if ((double)parsed != value) return -1;
    *out = parsed;
    return 0;
}

static app_host_subscription_t *find_subscription(app_host_server_t *srv, uint32_t handle) {
    for (size_t i = 0; i < APP_HOST_MAX_SUBSCRIPTIONS; i++) {
        if (srv->subs[i].hub_sub_id != 0 && srv->subs[i].handle == handle) return &srv->subs[i];
    }
    return NULL;
}

/* Copies the borrowed sample into the bounded queue, merging by handle. */
static void queue_event(app_host_server_t *srv, uint32_t handle,
                        const app_host_sample_t *sample, int keepalive) {
    uv_mutex_lock(&srv->queue_lock);
    if (srv->closing) {
        uv_mutex_unlock(&srv->queue_lock);
        return;
    }
    app_host_queued_event_t *entry = NULL;
    for (size_t i = 0; i < srv->queue_count; i++) {
        app_host_queued_event_t *candidate =
            &srv->queue[(srv->queue_head + i) % APP_HOST_EVENT_QUEUE_CAP];
        if (!candidate->platform && candidate->handle == handle) {
            entry = candidate;
            break;
        }
    }
    if (!entry) {
        if (srv->queue_count == APP_HOST_EVENT_QUEUE_CAP) {
            srv->queue_head = (srv->queue_head + 1) % APP_HOST_EVENT_QUEUE_CAP;
            srv->queue_count--;
            srv->dropped_events++;
        }
        entry = &srv->queue[(srv->queue_head + srv->queue_count) % APP_HOST_EVENT_QUEUE_CAP];
        srv->queue_count++;
    }
    entry->platform = 0;
    entry->handle = handle;
    entry->keepalive = keepalive ? 1 : 0;
    entry->data.sample = *sample;
    uv_async_send(&srv->wake);
    uv_mutex_unlock(&srv->queue_lock);
}

static int valid_platform_kind(const char *kind) {
    size_t len = strlen(kind);
    if (len == 0 || len >= APP_HOST_EVENT_KIND_SIZE) return 0;
    for (size_t i = 0; i < len; i++) {
        char c = kind[i];
        if (!(c == '_' || c == '.' || c == '-' || (c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

/* drop_oldest=0 is the tick path: a full queue skips the event instead of
 * displacing a point event. */
static int queue_platform_event(app_host_server_t *srv, const char *kind,
                                const char *payload_json, int drop_oldest) {
    if (!srv || !kind || !payload_json || !valid_platform_kind(kind)) return -1;
    size_t payload_len = strlen(payload_json);
    if (payload_len >= APP_HOST_EVENT_PAYLOAD_SIZE) return -1;

    uv_mutex_lock(&srv->queue_lock);
    if (srv->closing) {
        uv_mutex_unlock(&srv->queue_lock);
        return -1;
    }
    if (srv->queue_count == APP_HOST_EVENT_QUEUE_CAP) {
        if (!drop_oldest) {
            uv_mutex_unlock(&srv->queue_lock);
            return -1;
        }
        srv->queue_head = (srv->queue_head + 1) % APP_HOST_EVENT_QUEUE_CAP;
        srv->queue_count--;
        srv->dropped_events++;
    }
    app_host_queued_event_t *entry =
        &srv->queue[(srv->queue_head + srv->queue_count) % APP_HOST_EVENT_QUEUE_CAP];
    srv->queue_count++;
    entry->platform = 1;
    entry->keepalive = 0;
    entry->handle = 0;
    memcpy(entry->data.platform_event.kind, kind, strlen(kind) + 1);
    memcpy(entry->data.platform_event.payload, payload_json, payload_len + 1);
    uv_async_send(&srv->wake);
    uv_mutex_unlock(&srv->queue_lock);
    return 0;
}

int app_host_server_post_platform_event(app_host_server_t *srv, const char *kind,
                                        const char *payload_json) {
    return queue_platform_event(srv, kind, payload_json, 1);
}

int app_host_server_post_settings_changed(app_host_server_t *srv, const char *key) {
    if (!srv || !key || !key[0]) return -1;
    uv_mutex_lock(&srv->queue_lock);
    int watched = srv->settings_watched;
    uv_mutex_unlock(&srv->queue_lock);
    if (!watched) return -1;

    char escaped[APP_HOST_EVENT_PAYLOAD_SIZE];
    char payload[APP_HOST_EVENT_PAYLOAD_SIZE];
    if (json_escape(key, escaped, sizeof(escaped)) != 0) return -1;
    int n = snprintf(payload, sizeof(payload), "{\"key\":\"%s\"}", escaped);
    if (n < 0 || (size_t)n >= sizeof(payload)) return -1;
    return queue_platform_event(srv, "settings_changed", payload, 1);
}

void app_host_server_set_event_ack_cb(app_host_server_t *srv, app_host_event_ack_cb_t cb,
                                      void *ctx) {
    if (!srv) return;
    srv->event_ack_cb = cb;
    srv->event_ack_ctx = ctx;
}

static int tick_queued(app_host_server_t *srv) {
    int found = 0;
    uv_mutex_lock(&srv->queue_lock);
    for (size_t i = 0; i < srv->queue_count; i++) {
        const app_host_queued_event_t *entry =
            &srv->queue[(srv->queue_head + i) % APP_HOST_EVENT_QUEUE_CAP];
        if (entry->platform && strcmp(entry->data.platform_event.kind, "tick") == 0) {
            found = 1;
            break;
        }
    }
    uv_mutex_unlock(&srv->queue_lock);
    return found;
}

/* Periodic manifest tick. It is not reentrant: a queued or in-flight tick
 * skips the period, as does a full queue, and the skip is counted. */
static void on_tick(uv_timer_t *handle) {
    app_host_server_t *srv = (app_host_server_t *)handle->data;
    if (!srv || srv->closing) return;
    if (srv->pending_tick || tick_queued(srv)) {
        srv->ticks_skipped++;
        return;
    }
    char payload[32];
    int n = snprintf(payload, sizeof(payload), "{\"tick_ms\":%u}", srv->tick_ms);
    if (n < 0 || (size_t)n >= sizeof(payload) ||
        queue_platform_event(srv, "tick", payload, 0) != 0) {
        srv->ticks_skipped++;
        return;
    }
    srv->ticks_enqueued++;
    server_drain(srv);
}

void app_host_server_tick_counters(const app_host_server_t *srv, uint64_t *enqueued,
                                   uint64_t *skipped) {
    if (enqueued) *enqueued = srv ? srv->ticks_enqueued : 0;
    if (skipped) *skipped = srv ? srv->ticks_skipped : 0;
}

/* Sink handed to the product port; runs on the publisher's thread. */
static void server_post_event(void *event_ctx, uint32_t handle,
                              const app_host_sample_t *sample) {
    app_host_server_t *srv = (app_host_server_t *)event_ctx;
    if (!srv || !sample) return;
    queue_event(srv, handle, sample, 0);
}

static int sample_matches(const app_host_sample_t *a, const app_host_sample_t *b,
                          double deadband) {
    if (a->kind != b->kind) return 0;
    if (a->kind == APP_HOST_VALUE_FLOAT || a->kind == APP_HOST_VALUE_INTEGER) {
        double delta = a->number - b->number;
        if (delta < 0.0) delta = -delta;
        return delta <= deadband;
    }
    if (a->kind == APP_HOST_VALUE_BOOLEAN) return a->boolean == b->boolean;
    return strcmp(a->text, b->text) == 0;
}

static int send_event_frame(app_host_server_t *srv, const app_host_subscription_t *sub,
                            const app_host_queued_event_t *event, uint64_t event_id) {
    char value[256];
    int n = sample_value_json(&event->data.sample, value, sizeof(value));
    if (n < 0 || (size_t)n >= sizeof(value)) return -1;
    char payload[512];
    n = snprintf(payload, sizeof(payload),
                 "{\"value\":%s,\"quality\":\"%s\",\"source_ts\":%llu,\"reason\":\"%s\"}", value,
                 quality_name(event->data.sample.quality),
                 (unsigned long long)event->data.sample.source_ts,
                 event->keepalive ? "keepalive" : "change");
    if (n < 0 || (size_t)n >= sizeof(payload)) return -1;

    char prefix[128];
    n = snprintf(prefix, sizeof(prefix),
                 "{\"id\":%llu,\"fn\":\"app_on_event\",\"args\":{\"kind\":\"point_change\","
                 "\"point\":%u,\"payload\":\"",
                 (unsigned long long)event_id, sub->handle);
    if (n < 0 || (size_t)n >= sizeof(prefix)) return -1;
    size_t prefix_len = (size_t)n;

    /* Escaping can expand each byte sixfold; size the frame dynamically. The
     * end-to-end cap bounds that buffer before it is allocated. */
    size_t escaped_cap = strlen(payload) * 6 + 1;
    size_t frame_len_max = prefix_len + escaped_cap + 3; /* closing quote and two braces */
    if (frame_len_max > APP_HOST_EVENT_FRAME_CAP) {
        srv->oversize_frames++;
        return -1;
    }
    char *frame = malloc(frame_len_max + 1);
    if (!frame) return -1;
    memcpy(frame, prefix, prefix_len);
    if (json_escape(payload, frame + prefix_len, escaped_cap) != 0) {
        free(frame);
        return -1;
    }
    size_t used = prefix_len + strlen(frame + prefix_len);
    memcpy(frame + used, "\"}}", 4);
    int ok = send_json(srv, frame) == 0;
    free(frame);
    return ok ? 0 : -1;
}

/* The event caps bound the escaped payload to 6 * 128 bytes and the rest of the
 * frame to a few hundred, so the fixed frame fits with room to spare. */
static int send_platform_event_frame(app_host_server_t *srv,
                                     const app_host_queued_event_t *event, uint64_t event_id) {
    char frame[2048]; // stack-ok: bounded by APP_HOST_EVENT_*_SIZE, loop thread only
    int n = snprintf(frame, sizeof(frame),
                     "{\"id\":%llu,\"fn\":\"app_on_event\",\"args\":{\"kind\":\"%s\","
                     "\"point\":null,\"payload\":\"",
                     (unsigned long long)event_id, event->data.platform_event.kind);
    if (n < 0 || (size_t)n >= sizeof(frame)) return -1;
    size_t used = (size_t)n;
    if (json_escape(event->data.platform_event.payload, frame + used, sizeof(frame) - used) != 0) {
        return -1;
    }
    used += strlen(frame + used);
    if (used + 4 > sizeof(frame) || used + 3 > APP_HOST_EVENT_FRAME_CAP) {
        srv->oversize_frames++;
        return -1;
    }
    memcpy(frame + used, "\"}}", 4);
    return send_json(srv, frame) == 0 ? 0 : -1;
}

/* Sends at most one event and stops until its ack clears pending_event_id or
 * its deadline expires. */
static void server_drain(app_host_server_t *srv) {
    if (srv->closing || srv->pending_event_id != 0) return;
    for (;;) {
        uv_mutex_lock(&srv->queue_lock);
        if (srv->queue_count == 0) {
            uv_mutex_unlock(&srv->queue_lock);
            return;
        }
        app_host_queued_event_t event = srv->queue[srv->queue_head];
        srv->queue_head = (srv->queue_head + 1) % APP_HOST_EVENT_QUEUE_CAP;
        srv->queue_count--;
        uv_mutex_unlock(&srv->queue_lock);

        if (event.platform) {
            uint64_t event_id = srv->next_event_id;
            if (++srv->next_event_id == 0) srv->next_event_id = 1;
            if (send_platform_event_frame(srv, &event, event_id) != 0) continue;
            srv->pending_event_id = event_id;
            srv->pending_tick =
                strcmp(event.data.platform_event.kind, "tick") == 0 ? 1 : 0;
            snprintf(srv->pending_kind, sizeof(srv->pending_kind), "%s",
                     event.data.platform_event.kind);
            srv->pending_since_ms = uv_now(srv->loop);
            keepalive_rearm(srv);
            return;
        }

        app_host_subscription_t *sub = find_subscription(srv, event.handle);
        if (!sub) continue;
        if (srv->ports->allow) {
            char reason[APP_HOST_REASON_SIZE] = "";
            if (srv->ports->allow(srv->ports->ctx, event.handle, reason, sizeof(reason)) != 0) {
                /* The grant was revoked after subscribe. Tell the app why its
                 * stream stopped before releasing the data-source fan-out. */
                char payload[APP_HOST_EVENT_PAYLOAD_SIZE];
                char escaped[APP_HOST_REASON_SIZE * 6];
                if (json_escape(reason, escaped, sizeof(escaped)) == 0) {
                    int n = snprintf(payload, sizeof(payload),
                                     "{\"handle\":%u,\"reason\":\"%s\"}", event.handle, escaped);
                    if (n > 0 && (size_t)n < sizeof(payload)) {
                        (void)app_host_server_post_platform_event(srv, "grant_changed", payload);
                    }
                }
                int32_t token = sub->hub_sub_id;
                sub->hub_sub_id = 0;
                memset(sub, 0, sizeof(*sub));
                if (srv->ports->unsubscribe) {
                    (void)srv->ports->unsubscribe(srv->ports->ctx, token);
                }
                keepalive_rearm(srv);
                continue;
            }
        }
        uint64_t now = uv_now(srv->loop);
        if (!event.keepalive) {
            if (sub->min_interval_ms > 0 && now - sub->last_send_ms < sub->min_interval_ms) {
                continue;
            }
            if (sub->has_last && sub->last.quality == event.data.sample.quality &&
                sample_matches(&sub->last, &event.data.sample, sub->deadband)) {
                continue;
            }
        }
        uint64_t event_id = srv->next_event_id;
        if (++srv->next_event_id == 0) srv->next_event_id = 1;
        if (send_event_frame(srv, sub, &event, event_id) != 0) continue;
        srv->pending_event_id = event_id;
        srv->pending_kind[0] = '\0';
        srv->pending_since_ms = now;
        sub->last = event.data.sample;
        sub->has_last = 1;
        sub->last_send_ms = now;
        keepalive_rearm(srv);
        return;
    }
}

static void on_wake(uv_async_t *handle) {
    app_host_server_t *srv = (app_host_server_t *)handle->data;
    if (!srv || srv->closing) return;
    server_drain(srv);
}

static void on_keepalive(uv_timer_t *handle) {
    app_host_server_t *srv = (app_host_server_t *)handle->data;
    if (!srv || srv->closing) return;
    uint64_t now = uv_now(srv->loop);
    /* A withheld ack must not stall the stream: drop the in-flight event once
     * its deadline passes and let the drain below send the next one. */
    if (srv->pending_event_id != 0 && now - srv->pending_since_ms >= APP_HOST_ACK_TIMEOUT_MS) {
        srv->pending_event_id = 0;
        srv->pending_tick = 0;
        srv->pending_kind[0] = '\0';
        srv->ack_timeouts++;
    }
    for (size_t i = 0; i < APP_HOST_MAX_SUBSCRIPTIONS; i++) {
        app_host_subscription_t *sub = &srv->subs[i];
        if (sub->hub_sub_id == 0 || sub->max_interval_ms == 0) continue;
        if (now - sub->last_send_ms < sub->max_interval_ms) continue;
        app_host_sample_t sample;
        char reason[APP_HOST_REASON_SIZE] = "";
        int ok = srv->ports->read &&
                 srv->ports->read(srv->ports->ctx, sub->handle, &sample, reason,
                                  sizeof(reason)) == 0;
        if (ok) queue_event(srv, sub->handle, &sample, 1);
        /* Count a failed read and a queued event waiting behind an unacked
         * frame too; otherwise the timer re-fires until the ack. */
        sub->last_send_ms = now;
    }
    server_drain(srv);
    keepalive_rearm(srv);
}

static void keepalive_rearm(app_host_server_t *srv) {
    uint64_t now = uv_now(srv->loop);
    uint64_t earliest = 0;
    if (srv->pending_event_id != 0) {
        earliest = srv->pending_since_ms + APP_HOST_ACK_TIMEOUT_MS;
    }
    for (size_t i = 0; i < APP_HOST_MAX_SUBSCRIPTIONS; i++) {
        app_host_subscription_t *sub = &srv->subs[i];
        if (sub->hub_sub_id == 0 || sub->max_interval_ms == 0) continue;
        uint64_t due = sub->last_send_ms + sub->max_interval_ms;
        if (earliest == 0 || due < earliest) earliest = due;
    }
    if (earliest == 0) {
        uv_timer_stop(&srv->keepalive);
        return;
    }
    uv_timer_start(&srv->keepalive, on_keepalive, earliest > now ? earliest - now : 0, 0);
}

static void handle_resolve(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *name = args ? cJSON_GetObjectItemCaseSensitive(args, "name") : NULL;
    if (name == NULL || !cJSON_IsString(name) || !name->valuestring[0] ||
        !srv->ports->resolve) {
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
    (void)send_json(srv, buffer);
}

static void handle_read(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *handle = args ? cJSON_GetObjectItemCaseSensitive(args, "handle") : NULL;
    if (handle == NULL || !cJSON_IsNumber(handle) || !srv->ports->read) {
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
    (void)send_json(srv, buffer);
}

static void handle_publish(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *handle = args ? cJSON_GetObjectItemCaseSensitive(args, "handle") : NULL;
    cJSON *sample_json = args ? cJSON_GetObjectItemCaseSensitive(args, "sample") : NULL;
    if (handle == NULL || !cJSON_IsNumber(handle) || !srv->ports->publish) {
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
    (void)send_json(srv, buffer);
}

static void handle_log(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *level = args ? cJSON_GetObjectItemCaseSensitive(args, "level") : NULL;
    cJSON *message = args ? cJSON_GetObjectItemCaseSensitive(args, "message") : NULL;
    if (level == NULL || message == NULL || !cJSON_IsString(level) ||
        !cJSON_IsString(message) || !srv->ports->log) {
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
    (void)send_json(srv, buffer);
}

static void handle_setting_get(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *key = args ? cJSON_GetObjectItemCaseSensitive(args, "key") : NULL;
    if (!cJSON_IsString(key) || !key->valuestring[0] || !srv->ports->setting_get) {
        send_error(srv, id, "invalid");
        return;
    }
    char reason[APP_HOST_REASON_SIZE] = "";
    if (srv->ports->setting_get(srv->ports->ctx, key->valuestring, srv->setting_value,
                                sizeof(srv->setting_value), reason, sizeof(reason)) != 0) {
        send_error(srv, id, reason);
        return;
    }
    snprintf(srv->setting_frame, sizeof(srv->setting_frame),
             "{\"id\":%llu,\"ok\":true,\"result\":{\"value\":%s}}", (unsigned long long)id,
             srv->setting_value);
    (void)send_json(srv, srv->setting_frame);
}

static void handle_setting_watch(app_host_server_t *srv, uint64_t id) {
    /* No settings port means this product has no per-app settings store. */
    if (!srv->ports->setting_get) {
        send_error(srv, id, "not_supported");
        return;
    }
    uv_mutex_lock(&srv->queue_lock);
    srv->settings_watched = 1;
    uv_mutex_unlock(&srv->queue_lock);
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "{\"id\":%llu,\"ok\":true,\"result\":{}}",
             (unsigned long long)id);
    (void)send_json(srv, buffer);
}

static void handle_now(app_host_server_t *srv, uint64_t id) {
    uint64_t now = srv->ports->now_ms ? srv->ports->now_ms(srv->ports->ctx) : 0;
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "{\"id\":%llu,\"ok\":true,\"result\":{\"now_ms\":%llu}}",
             (unsigned long long)id, (unsigned long long)now);
    (void)send_json(srv, buffer);
}

static void handle_subscribe(app_host_server_t *srv, uint64_t id, cJSON *args) {
    cJSON *handle_item = args ? cJSON_GetObjectItemCaseSensitive(args, "handle") : NULL;
    cJSON *deadband_item = args ? cJSON_GetObjectItemCaseSensitive(args, "deadband") : NULL;
    cJSON *min_item = args ? cJSON_GetObjectItemCaseSensitive(args, "min_interval_ms") : NULL;
    cJSON *max_item = args ? cJSON_GetObjectItemCaseSensitive(args, "max_interval_ms") : NULL;
    uint32_t handle = 0;
    uint32_t min_interval = 0;
    uint32_t max_interval = 0;
    if (!srv->ports->subscribe) {
        send_error(srv, id, "internal");
        return;
    }
    if (json_u32(handle_item, &handle) != 0 || handle == 0 || !cJSON_IsNumber(deadband_item) ||
        !isfinite(deadband_item->valuedouble) || deadband_item->valuedouble < 0.0 ||
        json_u32(min_item, &min_interval) != 0 || json_u32(max_item, &max_interval) != 0 ||
        (max_interval != 0 && max_interval < min_interval)) {
        send_error(srv, id, "invalid");
        return;
    }
    if (find_subscription(srv, handle)) {
        send_error(srv, id, "busy");
        return;
    }
    size_t slot = APP_HOST_MAX_SUBSCRIPTIONS;
    for (size_t i = 0; i < APP_HOST_MAX_SUBSCRIPTIONS; i++) {
        if (srv->subs[i].hub_sub_id == 0) {
            slot = i;
            break;
        }
    }
    if (slot == APP_HOST_MAX_SUBSCRIPTIONS) {
        send_error(srv, id, "denied_quota");
        return;
    }
    int32_t token = 0;
    char reason[APP_HOST_REASON_SIZE] = "";
    if (srv->ports->subscribe(srv->ports->ctx, handle, server_post_event, srv, &token, reason,
                              sizeof(reason)) != 0) {
        send_error(srv, id, reason);
        return;
    }
    if (token <= 0) {
        send_error(srv, id, "internal");
        return;
    }
    app_host_subscription_t *sub = &srv->subs[slot];
    memset(sub, 0, sizeof(*sub));
    sub->hub_sub_id = token;
    sub->handle = handle;
    sub->deadband = deadband_item->valuedouble;
    sub->min_interval_ms = min_interval;
    sub->max_interval_ms = max_interval;
    sub->last_send_ms = uv_now(srv->loop);
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "{\"id\":%llu,\"ok\":true,\"result\":{\"subscription\":%zu}}",
             (unsigned long long)id, slot + 1);
    (void)send_json(srv, buffer);
    keepalive_rearm(srv);
}

static bool rate_allow(app_host_server_t *srv, const char *fn) {
    uint32_t call_limit = srv->ports->calls_per_sec;
    uint32_t publish_limit = srv->ports->publish_per_sec;
    if (call_limit == 0 && publish_limit == 0) return true;
    uint64_t now = srv->ports->now_ms ? srv->ports->now_ms(srv->ports->ctx) : 0;
    if (srv->rate_window_ms == 0 || now < srv->rate_window_ms ||
        now - srv->rate_window_ms >= 1000) {
        srv->rate_window_ms = now;
        srv->calls_in_window = 0;
        srv->publishes_in_window = 0;
    }
    bool publish = strcmp(fn, "point_publish") == 0;
    if (call_limit > 0 && srv->calls_in_window >= call_limit) return false;
    if (publish && publish_limit > 0 && srv->publishes_in_window >= publish_limit) return false;
    srv->calls_in_window++;
    if (publish) srv->publishes_in_window++;
    return true;
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
    if (!cJSON_IsString(fn)) {
        /* A no-fn frame is the event ack when the id matches. Anything else is
         * ignored: answering a stray id would desynchronize the app's calls. */
        if (srv->pending_event_id != 0 && id == srv->pending_event_id) {
            char acked_kind[APP_HOST_EVENT_KIND_SIZE];
            snprintf(acked_kind, sizeof(acked_kind), "%s", srv->pending_kind);
            srv->pending_event_id = 0;
            srv->pending_tick = 0;
            srv->pending_kind[0] = '\0';
            if (acked_kind[0] && srv->event_ack_cb) {
                srv->event_ack_cb(srv->event_ack_ctx, acked_kind);
            }
            server_drain(srv);
        }
        cJSON_Delete(root);
        return;
    }
    if (!fn->valuestring[0]) {
        send_error(srv, id, "invalid");
    } else if (!rate_allow(srv, fn->valuestring)) {
        send_error(srv, id, "rate_limited");
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
        handle_subscribe(srv, id, args);
    } else if (strcmp(fn->valuestring, "setting_get") == 0) {
        handle_setting_get(srv, id, args);
    } else if (strcmp(fn->valuestring, "setting_watch") == 0) {
        handle_setting_watch(srv, id);
    } else if (strcmp(fn->valuestring, "kv_get") == 0 ||
               strcmp(fn->valuestring, "kv_set") == 0) {
        send_error(srv, id, "not_supported");
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
    if (!srv) return;
    if (--srv->closes_outstanding > 0) return;
    uv_mutex_destroy(&srv->queue_lock);
    app_host_closed_cb_t on_closed = srv->on_closed;
    void *closed_ctx = srv->closed_ctx;
    free(srv->buffer);
    free(srv);
    if (on_closed) on_closed(closed_ctx);
}

static void close_initialized_handles(app_host_server_t *srv) {
    if (srv->handles_open & APP_HOST_HANDLE_PIPE) {
        srv->handles_open &= (uint8_t)~APP_HOST_HANDLE_PIPE;
        uv_close((uv_handle_t *)&srv->pipe, on_close);
    }
    if (srv->handles_open & APP_HOST_HANDLE_WAKE) {
        srv->handles_open &= (uint8_t)~APP_HOST_HANDLE_WAKE;
        uv_close((uv_handle_t *)&srv->wake, on_close);
    }
    if (srv->handles_open & APP_HOST_HANDLE_TIMER) {
        srv->handles_open &= (uint8_t)~APP_HOST_HANDLE_TIMER;
        uv_close((uv_handle_t *)&srv->keepalive, on_close);
    }
    if (srv->handles_open & APP_HOST_HANDLE_TICK) {
        srv->handles_open &= (uint8_t)~APP_HOST_HANDLE_TICK;
        uv_close((uv_handle_t *)&srv->tick, on_close);
    }
}

static void server_shutdown(app_host_server_t *srv) {
    if (!srv || srv->closing) return;
    uv_mutex_lock(&srv->queue_lock);
    srv->closing = 1;
    uv_mutex_unlock(&srv->queue_lock);

    /* Unsubscribe outside queue_lock: datahub_unsubscribe blocks on sub_lock,
     * which publisher callbacks hold while they queue. */
    for (size_t i = 0; i < APP_HOST_MAX_SUBSCRIPTIONS; i++) {
        app_host_subscription_t *sub = &srv->subs[i];
        if (sub->hub_sub_id == 0) continue;
        int32_t token = sub->hub_sub_id;
        sub->hub_sub_id = 0;
        if (srv->ports->unsubscribe) (void)srv->ports->unsubscribe(srv->ports->ctx, token);
        memset(sub, 0, sizeof(*sub));
    }
    uv_timer_stop(&srv->keepalive);
    if (srv->handles_open & APP_HOST_HANDLE_TICK) uv_timer_stop(&srv->tick);
    uv_read_stop((uv_stream_t *)&srv->pipe);
    close_initialized_handles(srv);
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
    // 限流需要单调时钟；缺少时钟端口时拒绝启动，避免静默变成不限流
    if ((ports->calls_per_sec > 0 || ports->publish_per_sec > 0) && !ports->now_ms) return NULL;
    app_host_server_t *srv = calloc(1, sizeof(*srv));
    if (!srv) {
        close(fd);
        return NULL;
    }
    srv->ports = ports;
    srv->on_closed = on_closed;
    srv->closed_ctx = closed_ctx;
    srv->loop = loop;
    srv->next_event_id = 1;
    if (uv_mutex_init(&srv->queue_lock) != 0) {
        close(fd);
        free(srv);
        return NULL;
    }
    if (uv_async_init(loop, &srv->wake, on_wake) != 0) {
        uv_mutex_destroy(&srv->queue_lock);
        close(fd);
        free(srv);
        return NULL;
    }
    srv->wake.data = srv;
    srv->handles_open |= APP_HOST_HANDLE_WAKE;
    srv->closes_outstanding++;
    if (uv_timer_init(loop, &srv->keepalive) != 0) {
        srv->on_closed = NULL;
        srv->closed_ctx = NULL;
        close_initialized_handles(srv);
        close(fd);
        return NULL;
    }
    srv->keepalive.data = srv;
    srv->handles_open |= APP_HOST_HANDLE_TIMER;
    srv->closes_outstanding++;
    srv->tick_ms = ports->tick_ms;
    if (srv->tick_ms > 0) {
        if (uv_timer_init(loop, &srv->tick) != 0) {
            srv->on_closed = NULL;
            srv->closed_ctx = NULL;
            close_initialized_handles(srv);
            close(fd);
            return NULL;
        }
        srv->tick.data = srv;
        srv->handles_open |= APP_HOST_HANDLE_TICK;
        srv->closes_outstanding++;
        uv_timer_start(&srv->tick, on_tick, srv->tick_ms, srv->tick_ms);
    }
    if (uv_pipe_init(loop, &srv->pipe, 0) != 0) {
        srv->on_closed = NULL;
        srv->closed_ctx = NULL;
        close_initialized_handles(srv);
        close(fd);
        return NULL;
    }
    srv->pipe.data = srv;
    srv->handles_open |= APP_HOST_HANDLE_PIPE;
    srv->closes_outstanding++;
    if (uv_pipe_open(&srv->pipe, fd) != 0) {
        close(fd);
        srv->on_closed = NULL;
        srv->closed_ctx = NULL;
        close_initialized_handles(srv);
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
