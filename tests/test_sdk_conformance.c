#define _POSIX_C_SOURCE 200809L

#include <cjson/cJSON.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "edge_app_sdk.h"

#define LOG_LINES 8
#define LOG_CHARS 256

static void set_failure(char *buf, size_t cap, const char *fmt, ...) {
    if (buf[0] != '\0') return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
}

static char *read_source_file(const char *rel_path) {
    static const char *prefixes[] = {"", "../", "../../"};
    char path[512];
    for (size_t i = 0; i < sizeof prefixes / sizeof prefixes[0]; i++) {
        snprintf(path, sizeof path, "%s%s", prefixes[i], rel_path);
        FILE *fp = fopen(path, "r");
        if (fp == NULL) continue;
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (size < 0) {
            fclose(fp);
            return NULL;
        }
        char *text = malloc((size_t)size + 1);
        if (text == NULL) {
            fclose(fp);
            return NULL;
        }
        size_t got = fread(text, 1, (size_t)size, fp);
        text[got] = '\0';
        fclose(fp);
        return text;
    }
    return NULL;
}

static int write_all(int fd, const void *buf, size_t len) {
    const unsigned char *p = buf;
    while (len > 0) {
        ssize_t n = write(fd, p, len);
        if (n < 0) return -1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t len) {
    unsigned char *p = buf;
    while (len > 0) {
        ssize_t n = read(fd, p, len);
        if (n <= 0) return -1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

static int send_frame(int fd, const char *json) {
    size_t len = strlen(json);
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

typedef struct {
    int fd;
    const cJSON *steps;
    char logs[LOG_LINES][LOG_CHARS];
    int log_count;
    char failure[512];
} host_t;

static void *host_thread(void *arg) {
    host_t *host = arg;
    int index = 0;
    char frame[4096];
    const cJSON *step = NULL;

    cJSON_ArrayForEach(step, host->steps) {
        if (recv_frame(host->fd, frame, sizeof frame) < 0) {
            set_failure(host->failure, sizeof host->failure,
                        "host timed out waiting for step %d", index);
            return NULL;
        }
        cJSON *request = cJSON_Parse(frame);
        if (request == NULL) {
            set_failure(host->failure, sizeof host->failure,
                        "step %d is not JSON", index);
            return NULL;
        }

        const cJSON *fn = cJSON_GetObjectItem(request, "fn");
        const cJSON *args = cJSON_GetObjectItem(request, "args");
        const cJSON *want_fn = cJSON_GetObjectItem(step, "fn");
        const cJSON *want_args = cJSON_GetObjectItem(step, "args");
        if (!cJSON_IsString(fn) || !cJSON_IsString(want_fn) ||
            strcmp(fn->valuestring, want_fn->valuestring) != 0) {
            set_failure(host->failure, sizeof host->failure,
                        "step %d expected fn %s", index,
                        cJSON_IsString(want_fn) ? want_fn->valuestring : "?");
            cJSON_Delete(request);
            return NULL;
        }
        if (!cJSON_Compare(args, want_args, 1)) {
            char *got = cJSON_PrintUnformatted(args);
            char *want = cJSON_PrintUnformatted(want_args);
            set_failure(host->failure, sizeof host->failure,
                        "step %d %s args got %s want %s", index,
                        fn->valuestring, got != NULL ? got : "?",
                        want != NULL ? want : "?");
            cJSON_free(got);
            cJSON_free(want);
            cJSON_Delete(request);
            return NULL;
        }

        if (strcmp(fn->valuestring, "log") == 0 && host->log_count < LOG_LINES) {
            const cJSON *message = cJSON_GetObjectItem(args, "message");
            if (cJSON_IsString(message)) {
                snprintf(host->logs[host->log_count], LOG_CHARS, "%s",
                         message->valuestring);
                host->log_count++;
            }
        }

        const cJSON *expect = cJSON_GetObjectItem(step, "expect");
        cJSON *response = cJSON_CreateObject();
        const cJSON *id = cJSON_GetObjectItem(request, "id");
        cJSON_AddNumberToObject(response, "id",
                                id != NULL ? id->valuedouble : 0);
        if (cJSON_IsTrue(cJSON_GetObjectItem(expect, "ok"))) {
            cJSON_AddBoolToObject(response, "ok", 1);
            cJSON_AddItemToObject(response, "result",
                                  cJSON_Duplicate(
                                      cJSON_GetObjectItem(expect, "result"), 1));
        } else {
            cJSON_AddBoolToObject(response, "ok", 0);
            cJSON_AddStringToObject(
                response, "code",
                cJSON_GetObjectItem(expect, "code")->valuestring);
        }
        char *json = cJSON_PrintUnformatted(response);
        if (json == NULL || send_frame(host->fd, json) != 0) {
            set_failure(host->failure, sizeof host->failure,
                        "step %d response write failed", index);
            cJSON_free(json);
            cJSON_Delete(response);
            cJSON_Delete(request);
            return NULL;
        }
        cJSON_free(json);
        cJSON_Delete(response);
        cJSON_Delete(request);
        index++;
    }
    return NULL;
}

typedef struct {
    edge_app_sdk_t *sdk;
    const cJSON *steps;
    int initialized;
    int stopped;
    int events;
    char failure[512];
} app_t;

static void app_init(void *ctx) {
    ((app_t *)ctx)->initialized = 1;
}

static void app_stop(void *ctx) {
    ((app_t *)ctx)->stopped = 1;
}

static int expect_ok(const cJSON *expect) {
    return cJSON_IsTrue(cJSON_GetObjectItem(expect, "ok"));
}

static int expect_error(app_t *app, const char *code, const cJSON *expect) {
    if (expect_ok(expect)) {
        set_failure(app->failure, sizeof app->failure,
                    "unexpected error %s", code);
        return 0;
    }
    const cJSON *want = cJSON_GetObjectItem(expect, "code");
    if (!cJSON_IsString(want) || strcmp(code, want->valuestring) != 0) {
        set_failure(app->failure, sizeof app->failure,
                    "got error %s want %s", code,
                    cJSON_IsString(want) ? want->valuestring : "?");
        return 0;
    }
    return 1;
}

static int check_u64(app_t *app, uint64_t got, const cJSON *result,
                     const char *key) {
    const cJSON *want = cJSON_GetObjectItem(result, key);
    if (want == NULL || (uint64_t)want->valuedouble != got) {
        set_failure(app->failure, sizeof app->failure, "%s got %llu", key,
                    (unsigned long long)got);
        return 0;
    }
    return 1;
}

static int value_matches(const edge_app_value_t *got, const cJSON *want) {
    const cJSON *field = NULL;
    if ((field = cJSON_GetObjectItem(want, "float")) != NULL) {
        return got->kind == EDGE_APP_VALUE_FLOAT &&
               got->number == field->valuedouble;
    }
    if ((field = cJSON_GetObjectItem(want, "integer")) != NULL) {
        return got->kind == EDGE_APP_VALUE_INTEGER &&
               got->integer == (int64_t)field->valuedouble;
    }
    if ((field = cJSON_GetObjectItem(want, "boolean")) != NULL) {
        return got->kind == EDGE_APP_VALUE_BOOLEAN &&
               got->boolean == cJSON_IsTrue(field);
    }
    if ((field = cJSON_GetObjectItem(want, "text")) != NULL) {
        return got->kind == EDGE_APP_VALUE_TEXT && cJSON_IsString(field) &&
               strcmp(got->text, field->valuestring) == 0;
    }
    return 0;
}

static void sample_from_json(const cJSON *json, edge_app_sample_t *sample) {
    memset(sample, 0, sizeof *sample);
    const cJSON *value = cJSON_GetObjectItem(json, "value");
    const cJSON *field = NULL;
    if ((field = cJSON_GetObjectItem(value, "float")) != NULL) {
        sample->value.kind = EDGE_APP_VALUE_FLOAT;
        sample->value.number = field->valuedouble;
    } else if ((field = cJSON_GetObjectItem(value, "integer")) != NULL) {
        sample->value.kind = EDGE_APP_VALUE_INTEGER;
        sample->value.integer = (int64_t)field->valuedouble;
        sample->value.number = (double)sample->value.integer;
    } else if ((field = cJSON_GetObjectItem(value, "boolean")) != NULL) {
        sample->value.kind = EDGE_APP_VALUE_BOOLEAN;
        sample->value.boolean = cJSON_IsTrue(field);
    } else if ((field = cJSON_GetObjectItem(value, "text")) != NULL) {
        sample->value.kind = EDGE_APP_VALUE_TEXT;
        snprintf(sample->value.text, sizeof sample->value.text, "%s",
                 field->valuestring);
    }
    const cJSON *quality = cJSON_GetObjectItem(json, "quality");
    sample->quality = cJSON_IsString(quality) ? quality->valuestring : "good";
    const cJSON *source_ts = cJSON_GetObjectItem(json, "source_ts");
    sample->source_ts = source_ts != NULL ? (uint64_t)source_ts->valuedouble : 0;
}

static int run_step(app_t *app, const cJSON *step) {
    const char *fn = cJSON_GetObjectItem(step, "fn")->valuestring;
    const cJSON *args = cJSON_GetObjectItem(step, "args");
    const cJSON *expect = cJSON_GetObjectItem(step, "expect");
    const cJSON *result = cJSON_GetObjectItem(expect, "result");
    const char *err = NULL;

    if (strcmp(fn, "point_resolve") == 0) {
        uint32_t handle = 0;
        const char *access = NULL;
        err = edge_app_point_resolve(
            app->sdk, cJSON_GetObjectItem(args, "name")->valuestring, &handle,
            &access);
        if (err != NULL) return expect_error(app, err, expect);
        if (!expect_ok(expect)) {
            set_failure(app->failure, sizeof app->failure,
                        "point_resolve unexpectedly succeeded");
            return 0;
        }
        if (!check_u64(app, handle, result, "handle")) return 0;
        const cJSON *want_access = cJSON_GetObjectItem(result, "access");
        if (!cJSON_IsString(want_access) ||
            strcmp(access, want_access->valuestring) != 0) {
            set_failure(app->failure, sizeof app->failure,
                        "access got %s", access);
            return 0;
        }
        return 1;
    }

    if (strcmp(fn, "point_read") == 0) {
        edge_app_sample_t sample;
        err = edge_app_point_read(
            app->sdk, (uint32_t)cJSON_GetObjectItem(args, "handle")->valuedouble,
            &sample);
        if (err != NULL) return expect_error(app, err, expect);
        if (!expect_ok(expect)) {
            set_failure(app->failure, sizeof app->failure,
                        "point_read unexpectedly succeeded");
            return 0;
        }
        if (!value_matches(&sample.value,
                           cJSON_GetObjectItem(result, "value"))) {
            set_failure(app->failure, sizeof app->failure,
                        "point_read value mismatch");
            return 0;
        }
        const cJSON *quality = cJSON_GetObjectItem(result, "quality");
        if (!cJSON_IsString(quality) ||
            strcmp(sample.quality, quality->valuestring) != 0) {
            set_failure(app->failure, sizeof app->failure,
                        "quality got %s", sample.quality);
            return 0;
        }
        return check_u64(app, sample.source_ts, result, "source_ts");
    }

    if (strcmp(fn, "point_subscribe") == 0) {
        uint32_t subscription = 0;
        err = edge_app_point_subscribe(
            app->sdk, (uint32_t)cJSON_GetObjectItem(args, "handle")->valuedouble,
            cJSON_GetObjectItem(args, "deadband")->valuedouble,
            (uint32_t)cJSON_GetObjectItem(args, "min_interval_ms")->valuedouble,
            (uint32_t)cJSON_GetObjectItem(args, "max_interval_ms")->valuedouble,
            &subscription);
        if (err != NULL) return expect_error(app, err, expect);
        if (!expect_ok(expect)) {
            set_failure(app->failure, sizeof app->failure,
                        "point_subscribe unexpectedly succeeded");
            return 0;
        }
        return check_u64(app, subscription, result, "subscription");
    }

    if (strcmp(fn, "point_publish") == 0) {
        edge_app_sample_t sample;
        sample_from_json(cJSON_GetObjectItem(args, "sample"), &sample);
        err = edge_app_point_publish(
            app->sdk, (uint32_t)cJSON_GetObjectItem(args, "handle")->valuedouble,
            &sample);
        if (err != NULL) return expect_error(app, err, expect);
        if (!expect_ok(expect)) {
            set_failure(app->failure, sizeof app->failure,
                        "point_publish unexpectedly succeeded");
            return 0;
        }
        return 1;
    }

    if (strcmp(fn, "log") == 0) {
        err = edge_app_log(app->sdk,
                           cJSON_GetObjectItem(args, "level")->valuestring,
                           cJSON_GetObjectItem(args, "message")->valuestring);
        if (err != NULL) return expect_error(app, err, expect);
        if (!expect_ok(expect)) {
            set_failure(app->failure, sizeof app->failure,
                        "log unexpectedly succeeded");
            return 0;
        }
        return 1;
    }

    if (strcmp(fn, "now") == 0) {
        uint64_t now_ms = 0;
        err = edge_app_now(app->sdk, &now_ms);
        if (err != NULL) return expect_error(app, err, expect);
        if (!expect_ok(expect)) {
            set_failure(app->failure, sizeof app->failure,
                        "now unexpectedly succeeded");
            return 0;
        }
        return check_u64(app, now_ms, result, "now_ms");
    }

    set_failure(app->failure, sizeof app->failure, "unknown fn %s", fn);
    return 0;
}

static void app_on_event(void *ctx, const char *event_json) {
    (void)event_json;
    app_t *app = ctx;
    app->events++;
    const cJSON *step = NULL;
    cJSON_ArrayForEach(step, app->steps) {
        if (!run_step(app, step)) return;
    }
}

static int dispatch_ok(edge_app_sdk_t *sdk, const char *request, int want_id,
                       char *failure, size_t failure_cap) {
    char response[1024];
    if (edge_app_sdk_dispatch(sdk, request, response, sizeof response) != 0) {
        set_failure(failure, failure_cap, "dispatch failed for %s", request);
        return 0;
    }
    cJSON *parsed = cJSON_Parse(response);
    const cJSON *id = cJSON_GetObjectItem(parsed, "id");
    int ok = parsed != NULL && cJSON_IsTrue(cJSON_GetObjectItem(parsed, "ok")) &&
             id != NULL && (int)id->valuedouble == want_id;
    if (!ok) set_failure(failure, failure_cap, "bad response %s", response);
    cJSON_Delete(parsed);
    return ok;
}

static int run_vector(const cJSON *vector) {
    const char *name = cJSON_GetObjectItem(vector, "name")->valuestring;
    const cJSON *steps = cJSON_GetObjectItem(vector, "steps");
    const cJSON *event = cJSON_GetObjectItem(vector, "event");
    const cJSON *expect_log = cJSON_GetObjectItem(vector, "expect_log");
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
        printf("FAIL %s: socketpair\n", name);
        return 0;
    }
    struct timeval timeout = {2, 0};
    setsockopt(sockets[0], SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    setsockopt(sockets[1], SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    host_t host = {0};
    host.fd = sockets[1];
    host.steps = steps;
    pthread_t thread;
    if (pthread_create(&thread, NULL, host_thread, &host) != 0) {
        close(sockets[0]);
        close(sockets[1]);
        printf("FAIL %s: pthread_create\n", name);
        return 0;
    }

    edge_app_sdk_t *sdk = edge_app_sdk_create(sockets[0]);
    app_t app = {0};
    app.sdk = sdk;
    app.steps = steps;
    edge_app_sdk_set_exports(sdk, app_init, app_on_event, app_stop, &app);

    int ok = dispatch_ok(sdk, "{\"id\":1,\"fn\":\"app_init\",\"args\":{}}", 1,
                         app.failure, sizeof app.failure);

    if (ok) {
        cJSON *request = cJSON_CreateObject();
        cJSON_AddNumberToObject(request, "id", 2);
        cJSON_AddStringToObject(request, "fn", "app_on_event");
        cJSON_AddItemToObject(request, "args", cJSON_Duplicate(event, 1));
        char *json = cJSON_PrintUnformatted(request);
        ok = json != NULL &&
             dispatch_ok(sdk, json, 2, app.failure, sizeof app.failure);
        cJSON_free(json);
        cJSON_Delete(request);
    }
    if (ok) {
        ok = dispatch_ok(sdk, "{\"id\":3,\"fn\":\"app_stop\",\"args\":{}}", 3,
                         app.failure, sizeof app.failure);
    }

    pthread_join(thread, NULL);
    if (ok && (app.initialized != 1 || app.stopped != 1 || app.events != 1)) {
        set_failure(app.failure, sizeof app.failure,
                    "export counters init=%d event=%d stop=%d", app.initialized,
                    app.events, app.stopped);
        ok = 0;
    }
    if (ok && host.failure[0] != '\0') {
        set_failure(app.failure, sizeof app.failure, "%s", host.failure);
        ok = 0;
    }
    if (ok) {
        int want_logs = cJSON_GetArraySize(expect_log);
        if (want_logs != host.log_count) {
            set_failure(app.failure, sizeof app.failure, "log count got %d want %d",
                        host.log_count, want_logs);
            ok = 0;
        }
        for (int i = 0; ok && i < want_logs; i++) {
            const cJSON *line = cJSON_GetArrayItem(expect_log, i);
            if (!cJSON_IsString(line) ||
                strcmp(host.logs[i], line->valuestring) != 0) {
                set_failure(app.failure, sizeof app.failure,
                            "log[%d] got %s", i, host.logs[i]);
                ok = 0;
            }
        }
    }

    edge_app_sdk_destroy(sdk);
    close(sockets[1]);

    if (!ok) {
        printf("FAIL %s: %s\n", name, app.failure);
    }
    return ok;
}

int main(void) {
    char *text = read_source_file("sdk/conformance/vectors.json");
    if (text == NULL) {
        fprintf(stderr, "cannot find sdk/conformance/vectors.json\n");
        return 1;
    }
    cJSON *root = cJSON_Parse(text);
    free(text);
    if (root == NULL) {
        fprintf(stderr, "vectors.json is not valid JSON\n");
        return 1;
    }

    int passed = 0;
    int total = 0;
    const cJSON *vector = NULL;
    cJSON_ArrayForEach(vector, cJSON_GetObjectItem(root, "vectors")) {
        total++;
        const char *name = cJSON_GetObjectItem(vector, "name")->valuestring;
        if (run_vector(vector)) {
            passed++;
            printf("PASS %s\n", name);
        }
    }
    cJSON_Delete(root);

    printf("sdk conformance (c): %d/%d vectors passed\n", passed, total);
    return passed == total ? 0 : 1;
}
