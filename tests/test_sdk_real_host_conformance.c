/**
 * @file test_sdk_real_host_conformance.c
 * @brief Runs shared SDK vectors against the real app host server.
 *
 * tests/test_sdk_conformance.c answers the vectors with a scripted fake host, so
 * subscribe_with_keepalive passes there by construction. This test runs the same
 * SDK calls through app_host_server and pins the point_subscribe gap recorded in
 * sdk/abi_status.json.
 */

#define _POSIX_C_SOURCE 200809L

#include <cjson/cJSON.h>
#include <cmocka.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "edge_app_host.h"
#include "edge_app_sdk.h"

#define LOG_LINES 8
#define LOG_CHARS 256

/* The vectors this host can serve; their steps are all success expectations. */
static const char *const executed_vectors[] = {
    "resolve_read_product_point",
    "resolve_own_point_publish_text",
    "now_and_debug_log",
};

typedef struct {
    int publish_calls;
    int log_count;
    char logs[LOG_LINES][LOG_CHARS];
    app_host_sample_t last_sample;
} fake_ports_t;

static int fake_resolve(void *ctx, const char *name, uint32_t *handle, uint8_t *access,
                        char *reason, size_t reason_size) {
    (void)ctx;
    if (strcmp(name, "U_A") == 0 || strcmp(name, "U_B") == 0) {
        *handle = strcmp(name, "U_A") == 0 ? 7 : 8;
        *access = APP_HOST_ACCESS_READ;
        return 0;
    }
    if (strcmp(name, "app.demo.value") == 0) {
        *handle = 9;
        *access = APP_HOST_ACCESS_PUBLISH;
        return 0;
    }
    snprintf(reason, reason_size, "not_found");
    return -1;
}

static int fake_read(void *ctx, uint32_t handle, app_host_sample_t *out, char *reason,
                     size_t reason_size) {
    (void)ctx;
    if (handle != 7) {
        snprintf(reason, reason_size, "invalid");
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->kind = APP_HOST_VALUE_FLOAT;
    out->number = 220.5;
    out->quality = APP_HOST_QUALITY_GOOD;
    out->source_ts = 1720000000000ULL;
    return 0;
}

static int fake_publish(void *ctx, uint32_t handle, const app_host_sample_t *sample, char *reason,
                        size_t reason_size) {
    fake_ports_t *fake = ctx;
    fake->publish_calls++;
    if (handle == 7) {
        snprintf(reason, reason_size, "denied_scope");
        return -1;
    }
    if (handle != 9) {
        snprintf(reason, reason_size, "invalid");
        return -1;
    }
    fake->last_sample = *sample;
    return 0;
}

static void fake_log(void *ctx, int level, const char *message) {
    fake_ports_t *fake = ctx;
    (void)level;
    if (fake->log_count < LOG_LINES) {
        snprintf(fake->logs[fake->log_count], LOG_CHARS, "%s", message);
        fake->log_count++;
    }
}

static uint64_t fake_now(void *ctx) {
    (void)ctx;
    return 1720000000123ULL;
}

static cJSON *read_json(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "cannot open %s\n", path);
        return NULL;
    }
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
    fclose(fp);
    text[got] = '\0';
    cJSON *root = cJSON_Parse(text);
    free(text);
    if (root == NULL) fprintf(stderr, "%s is not valid JSON\n", path);
    return root;
}

static cJSON *g_vectors;
static cJSON *g_abi;

static const cJSON *vector_list(void) {
    return cJSON_GetObjectItem(g_vectors, "vectors");
}

static const cJSON *find_vector(const cJSON *vectors, const char *name) {
    const cJSON *vector = NULL;
    cJSON_ArrayForEach(vector, vectors) {
        const cJSON *item = cJSON_GetObjectItem(vector, "name");
        if (cJSON_IsString(item) && strcmp(item->valuestring, name) == 0) return vector;
    }
    return NULL;
}

static int step_fail(const char *vector, int index, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "FAIL %s step %d: ", vector, index);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    return 0;
}

static int sample_from_json(const cJSON *json, edge_app_sample_t *sample) {
    memset(sample, 0, sizeof(*sample));
    const cJSON *value = cJSON_GetObjectItem(json, "value");
    const cJSON *field = NULL;
    if ((field = cJSON_GetObjectItem(value, "float")) != NULL) {
        sample->value.kind = EDGE_APP_VALUE_FLOAT;
        sample->value.number = field->valuedouble;
    } else if ((field = cJSON_GetObjectItem(value, "text")) != NULL) {
        sample->value.kind = EDGE_APP_VALUE_TEXT;
        snprintf(sample->value.text, sizeof(sample->value.text), "%s", field->valuestring);
    } else {
        return -1;
    }
    const cJSON *quality = cJSON_GetObjectItem(json, "quality");
    sample->quality = cJSON_IsString(quality) ? quality->valuestring : "good";
    const cJSON *source_ts = cJSON_GetObjectItem(json, "source_ts");
    sample->source_ts = cJSON_IsNumber(source_ts) ? (uint64_t)source_ts->valuedouble : 0;
    return 0;
}

static int run_step(edge_app_sdk_t *sdk, const char *vector, int index, const cJSON *step) {
    const char *fn = cJSON_GetObjectItem(step, "fn")->valuestring;
    const cJSON *args = cJSON_GetObjectItem(step, "args");
    const cJSON *expect = cJSON_GetObjectItem(step, "expect");
    const cJSON *result = cJSON_GetObjectItem(expect, "result");
    const char *err;

    if (!cJSON_IsTrue(cJSON_GetObjectItem(expect, "ok"))) {
        return step_fail(vector, index, "%s: only success steps are executed here", fn);
    }

    if (strcmp(fn, "point_resolve") == 0) {
        const char *name = cJSON_GetObjectItem(args, "name")->valuestring;
        uint32_t handle = 0;
        const char *access = NULL;
        err = edge_app_point_resolve(sdk, name, &handle, &access);
        if (err != NULL) return step_fail(vector, index, "point_resolve %s: %s", name, err);
        const cJSON *want_handle = cJSON_GetObjectItem(result, "handle");
        const cJSON *want_access = cJSON_GetObjectItem(result, "access");
        if (!cJSON_IsNumber(want_handle) || handle != (uint32_t)want_handle->valuedouble) {
            return step_fail(vector, index, "handle got %u want %g", handle,
                             cJSON_IsNumber(want_handle) ? want_handle->valuedouble : -1.0);
        }
        if (!cJSON_IsString(want_access) || strcmp(access, want_access->valuestring) != 0) {
            return step_fail(vector, index, "access got %s want %s", access,
                             cJSON_IsString(want_access) ? want_access->valuestring : "?");
        }
        return 1;
    }

    if (strcmp(fn, "point_read") == 0) {
        edge_app_sample_t sample;
        memset(&sample, 0, sizeof(sample));
        err = edge_app_point_read(
            sdk, (uint32_t)cJSON_GetObjectItem(args, "handle")->valuedouble, &sample);
        if (err != NULL) return step_fail(vector, index, "point_read: %s", err);
        const cJSON *value = cJSON_GetObjectItem(result, "value");
        const cJSON *number = cJSON_GetObjectItem(value, "float");
        if (sample.value.kind != EDGE_APP_VALUE_FLOAT || !cJSON_IsNumber(number) ||
            sample.value.number != number->valuedouble) {
            return step_fail(vector, index, "value kind %d number %.17g", sample.value.kind,
                             sample.value.number);
        }
        const cJSON *quality = cJSON_GetObjectItem(result, "quality");
        if (!cJSON_IsString(quality) || strcmp(sample.quality, quality->valuestring) != 0) {
            return step_fail(vector, index, "quality got %s", sample.quality);
        }
        const cJSON *source_ts = cJSON_GetObjectItem(result, "source_ts");
        if (!cJSON_IsNumber(source_ts) ||
            sample.source_ts != (uint64_t)source_ts->valuedouble) {
            return step_fail(vector, index, "source_ts got %llu",
                             (unsigned long long)sample.source_ts);
        }
        return 1;
    }

    if (strcmp(fn, "point_publish") == 0) {
        edge_app_sample_t sample;
        if (sample_from_json(cJSON_GetObjectItem(args, "sample"), &sample) != 0) {
            return step_fail(vector, index, "unsupported sample value");
        }
        err = edge_app_point_publish(
            sdk, (uint32_t)cJSON_GetObjectItem(args, "handle")->valuedouble, &sample);
        if (err != NULL) return step_fail(vector, index, "point_publish: %s", err);
        return 1;
    }

    if (strcmp(fn, "log") == 0) {
        err = edge_app_log(sdk, cJSON_GetObjectItem(args, "level")->valuestring,
                           cJSON_GetObjectItem(args, "message")->valuestring);
        if (err != NULL) return step_fail(vector, index, "log: %s", err);
        return 1;
    }

    if (strcmp(fn, "now") == 0) {
        uint64_t now_ms = 0;
        err = edge_app_now(sdk, &now_ms);
        if (err != NULL) return step_fail(vector, index, "now: %s", err);
        const cJSON *want = cJSON_GetObjectItem(result, "now_ms");
        if (!cJSON_IsNumber(want) || now_ms != (uint64_t)want->valuedouble) {
            return step_fail(vector, index, "now_ms got %llu", (unsigned long long)now_ms);
        }
        return 1;
    }

    return step_fail(vector, index, "unknown fn %s", fn);
}

static int run_vector(edge_app_sdk_t *sdk, const cJSON *vector) {
    const char *name = cJSON_GetObjectItem(vector, "name")->valuestring;
    const cJSON *steps = cJSON_GetObjectItem(vector, "steps");
    const cJSON *step = NULL;
    int index = 0;
    cJSON_ArrayForEach(step, steps) {
        if (!run_step(sdk, name, index, step)) return 0;
        index++;
    }
    printf("PASS %s\n", name);
    return 1;
}

static int run_subscribe_gap(edge_app_sdk_t *sdk, const cJSON *vector) {
    const char *name = cJSON_GetObjectItem(vector, "name")->valuestring;
    const cJSON *steps = cJSON_GetObjectItem(vector, "steps");
    if (cJSON_GetArraySize(steps) != 2) {
        return step_fail(name, 0, "expected resolve and subscribe steps");
    }
    if (!run_step(sdk, name, 0, cJSON_GetArrayItem(steps, 0))) return 0;

    const cJSON *args = cJSON_GetObjectItem(cJSON_GetArrayItem(steps, 1), "args");
    const cJSON *host = cJSON_GetObjectItem(
        cJSON_GetObjectItem(cJSON_GetObjectItem(g_abi, "functions"), "point-subscribe"), "host");
    uint32_t subscription = 0;
    const char *err = edge_app_point_subscribe(
        sdk, (uint32_t)cJSON_GetObjectItem(args, "handle")->valuedouble,
        cJSON_GetObjectItem(args, "deadband")->valuedouble,
        (uint32_t)cJSON_GetObjectItem(args, "min_interval_ms")->valuedouble,
        (uint32_t)cJSON_GetObjectItem(args, "max_interval_ms")->valuedouble, &subscription);
    if (!cJSON_IsString(host)) {
        return step_fail(name, 1, "abi ledger has no point-subscribe host");
    }
    if (err == NULL || strcmp(err, host->valuestring) != 0) {
        return step_fail(name, 1, "point_subscribe got %s want ledger host %s",
                         err != NULL ? err : "success", host->valuestring);
    }
    printf("PASS %s (host gap: %s)\n", name, host->valuestring);
    return 1;
}

static int run_extra_host_calls(edge_app_sdk_t *sdk) {
    const cJSON *extra = cJSON_GetObjectItem(g_abi, "extra_host_json");
    const cJSON *entry = NULL;
    int ok = 1;
    for (entry = extra != NULL ? extra->child : NULL; entry != NULL; entry = entry->next) {
        char result[128];
        const char *err = edge_app_sdk_call(sdk, entry->string, "{}", result, sizeof(result));
        if (!cJSON_IsString(entry) || err == NULL || strcmp(err, entry->valuestring) != 0) {
            fprintf(stderr, "FAIL extra_host_json %s: got %s want %s\n", entry->string,
                    err != NULL ? err : "success",
                    cJSON_IsString(entry) ? entry->valuestring : "?");
            ok = 0;
        }
    }
    return ok;
}

static void child_client(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (sdk == NULL) _exit(10);

    for (size_t i = 0; i < sizeof(executed_vectors) / sizeof(executed_vectors[0]); i++) {
        const cJSON *vector = find_vector(vector_list(), executed_vectors[i]);
        if (vector == NULL) {
            fprintf(stderr, "FAIL missing vector %s\n", executed_vectors[i]);
            _exit(11);
        }
        if (!run_vector(sdk, vector)) _exit(12);
    }

    const cJSON *subscribe = find_vector(vector_list(), "subscribe_with_keepalive");
    if (subscribe == NULL || !run_subscribe_gap(sdk, subscribe)) _exit(13);
    if (!run_extra_host_calls(sdk)) _exit(14);

    edge_app_sdk_destroy(sdk);
    fflush(NULL);
    _exit(0);
}

static int run_loop_until(uv_loop_t *loop, pid_t pid, int timeout_ms) {
    int status = 0;
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (waitpid(pid, &status, WNOHANG) == pid) return status;
        usleep(10000);
    }
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
    return -1;
}

static void check_server_observations(const fake_ports_t *fake) {
    char want_logs[LOG_LINES][LOG_CHARS];
    int want_count = 0;
    for (size_t i = 0; i < sizeof(executed_vectors) / sizeof(executed_vectors[0]); i++) {
        const cJSON *vector = find_vector(vector_list(), executed_vectors[i]);
        const cJSON *line = NULL;
        assert_non_null(vector);
        cJSON_ArrayForEach(line, cJSON_GetObjectItem(vector, "expect_log")) {
            assert_true(want_count < LOG_LINES);
            snprintf(want_logs[want_count], LOG_CHARS, "%s", line->valuestring);
            want_count++;
        }
    }
    assert_int_equal(fake->log_count, want_count);
    for (int i = 0; i < want_count; i++) {
        assert_string_equal(fake->logs[i], want_logs[i]);
    }

    const cJSON *publish = find_vector(vector_list(), "resolve_own_point_publish_text");
    assert_non_null(publish);
    const cJSON *sample = cJSON_GetObjectItem(
        cJSON_GetObjectItem(cJSON_GetArrayItem(cJSON_GetObjectItem(publish, "steps"), 1), "args"),
        "sample");
    const cJSON *text = cJSON_GetObjectItem(cJSON_GetObjectItem(sample, "value"), "text");
    assert_true(cJSON_IsString(text));
    assert_int_equal(fake->publish_calls, 1);
    assert_int_equal(fake->last_sample.kind, APP_HOST_VALUE_TEXT);
    assert_string_equal(fake->last_sample.text, text->valuestring);
}

static void test_vectors_through_real_host(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    fake_ports_t fake;
    memset(&fake, 0, sizeof(fake));
    app_host_ports_t ports = {
        .ctx = &fake,
        .resolve = fake_resolve,
        .read = fake_read,
        .publish = fake_publish,
        .log = fake_log,
        .now_ms = fake_now,
    };
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);

    fflush(NULL);
    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        child_client(fds[1]);
    }
    close(fds[1]);

    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);

    check_server_observations(&fake);

    app_host_server_destroy(server);
    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static int vector_has_step(const cJSON *vectors, const char *fn_name) {
    const cJSON *vector = NULL;
    cJSON_ArrayForEach(vector, vectors) {
        const cJSON *step = NULL;
        cJSON_ArrayForEach(step, cJSON_GetObjectItem(vector, "steps")) {
            const cJSON *fn = cJSON_GetObjectItem(step, "fn");
            if (cJSON_IsString(fn) && strcmp(fn->valuestring, fn_name) == 0) return 1;
        }
    }
    return 0;
}

static void test_every_implemented_function_has_a_vector(void **state) {
    (void)state;
    const cJSON *vectors = vector_list();
    const cJSON *functions = cJSON_GetObjectItem(g_abi, "functions");
    const cJSON *fn = NULL;
    assert_non_null(vectors);
    assert_non_null(functions);
    for (fn = functions->child; fn != NULL; fn = fn->next) {
        const cJSON *host = cJSON_GetObjectItem(fn, "host");
        if (!cJSON_IsString(host) || strcmp(host->valuestring, "implemented") != 0) continue;
        char step_fn[64];
        snprintf(step_fn, sizeof(step_fn), "%s", fn->string);
        for (char *p = step_fn; *p != '\0'; p++) {
            if (*p == '-') *p = '_';
        }
        int covered = vector_has_step(vectors, step_fn);
        if (!covered) print_message("ledger function %s has no vector step\n", fn->string);
        assert_true(covered);
    }
}

static void test_subscribe_gap_is_recorded(void **state) {
    (void)state;
    const cJSON *vector = find_vector(vector_list(), "subscribe_with_keepalive");
    assert_non_null(vector);
    const cJSON *steps = cJSON_GetObjectItem(vector, "steps");
    assert_int_equal(cJSON_GetArraySize(steps), 2);

    const cJSON *subscribe = cJSON_GetArrayItem(steps, 1);
    const cJSON *fn = cJSON_GetObjectItem(subscribe, "fn");
    assert_true(cJSON_IsString(fn));
    assert_string_equal(fn->valuestring, "point_subscribe");
    assert_true(cJSON_IsTrue(cJSON_GetObjectItem(cJSON_GetObjectItem(subscribe, "expect"), "ok")));

    const cJSON *host = cJSON_GetObjectItem(
        cJSON_GetObjectItem(cJSON_GetObjectItem(g_abi, "functions"), "point-subscribe"), "host");
    assert_true(cJSON_IsString(host));
}

int main(void) {
    g_vectors = read_json(EDGEC_VECTORS);
    g_abi = read_json(EDGEC_ABI_STATUS);
    if (g_vectors == NULL || g_abi == NULL) {
        cJSON_Delete(g_vectors);
        cJSON_Delete(g_abi);
        return 1;
    }

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_every_implemented_function_has_a_vector),
        cmocka_unit_test(test_subscribe_gap_is_recorded),
        cmocka_unit_test(test_vectors_through_real_host),
    };
    int rc = cmocka_run_group_tests(tests, NULL, NULL);
    cJSON_Delete(g_vectors);
    cJSON_Delete(g_abi);
    return rc;
}
