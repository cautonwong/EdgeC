/**
 * @file test_app_host_server.c
 * @brief Host ABI server driven by the real C SDK over a socketpair.
 */

#include <cjson/cJSON.h>
#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "edge_app_host.h"
#include "edge_app_sdk.h"

typedef struct {
    int resolve_calls;
    int read_calls;
    int publish_calls;
    int log_calls;
    int now_calls;
    int subscribe_calls;
    int unsubscribe_calls;
    int allow_calls;
    int allow_denied;
    int next_token;
    uint32_t last_sub_handle;
    double last_deadband;
    uint32_t last_min_interval;
    uint32_t last_max_interval;
    app_host_event_cb_t sink;
    void *sink_ctx;
    int last_level;
    char last_log[128];
    app_host_sample_t last_sample;
} fake_ports_t;

static int fake_resolve(void *ctx, const char *name, uint32_t *handle, uint8_t *access,
                        char *reason, size_t reason_size) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->resolve_calls++;
    if (strcmp(name, "U_A") == 0) {
        *handle = 7;
        *access = APP_HOST_ACCESS_READ;
        return 0;
    }
    if (strcmp(name, "app.demo.value") == 0) {
        *handle = 131072 + 5;
        *access = APP_HOST_ACCESS_PUBLISH;
        return 0;
    }
    snprintf(reason, reason_size, "not_found");
    return -1;
}

static int fake_read(void *ctx, uint32_t handle, app_host_sample_t *out, char *reason,
                     size_t reason_size) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->read_calls++;
    if (handle != 7 && handle != 8) {
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

static int fake_subscribe(void *ctx, uint32_t handle, app_host_event_cb_t on_event,
                          void *event_ctx, int32_t *out_sub_id, char *reason,
                          size_t reason_size) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    (void)reason;
    (void)reason_size;
    fake->subscribe_calls++;
    fake->last_sub_handle = handle;
    fake->sink = on_event;
    fake->sink_ctx = event_ctx;
    *out_sub_id = fake->next_token++;
    return 0;
}

static int fake_unsubscribe(void *ctx, int32_t sub_id) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    (void)sub_id;
    fake->unsubscribe_calls++;
    return 0;
}

static int fake_allow(void *ctx, uint32_t handle, char *reason, size_t reason_size) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->allow_calls++;
    if (handle == 7 && fake->allow_denied) {
        snprintf(reason, reason_size, "denied_revoked");
        return -1;
    }
    return 0;
}

static int fake_publish(void *ctx, uint32_t handle, const app_host_sample_t *sample, char *reason,
                        size_t reason_size) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->publish_calls++;
    if (handle == 9) {
        snprintf(reason, reason_size, "denied_scope");
        return -1;
    }
    fake->last_sample = *sample;
    return 0;
}

static void fake_log(void *ctx, int level, const char *message) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->log_calls++;
    fake->last_level = level;
    snprintf(fake->last_log, sizeof(fake->last_log), "%s", message);
}

static uint64_t fake_now(void *ctx) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->now_calls++;
    return 1720000000123ULL;
}

static app_host_sample_t event_sample(double value, uint64_t source_ts) {
    app_host_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    sample.kind = APP_HOST_VALUE_FLOAT;
    sample.number = value;
    sample.quality = APP_HOST_QUALITY_GOOD;
    sample.source_ts = source_ts;
    return sample;
}

static int frame_event_matches(const char *frame_json, uint32_t point, double value,
                               const char *reason) {
    cJSON *root = cJSON_Parse(frame_json);
    if (!root) return 0;
    const cJSON *fn = cJSON_GetObjectItemCaseSensitive(root, "fn");
    const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    const cJSON *kind = cJSON_GetObjectItemCaseSensitive(args, "kind");
    const cJSON *point_item = cJSON_GetObjectItemCaseSensitive(args, "point");
    const cJSON *payload_text = cJSON_GetObjectItemCaseSensitive(args, "payload");
    int ok = cJSON_IsString(fn) && strcmp(fn->valuestring, "app_on_event") == 0 &&
             cJSON_IsString(kind) && strcmp(kind->valuestring, "point_change") == 0 &&
             cJSON_IsNumber(point_item) && (uint32_t)point_item->valuedouble == point &&
             cJSON_IsString(payload_text);
    if (ok) {
        cJSON *payload = cJSON_Parse(payload_text->valuestring);
        ok = payload != NULL;
        if (ok) {
            const cJSON *val = cJSON_GetObjectItemCaseSensitive(payload, "value");
            const cJSON *number = cJSON_GetObjectItemCaseSensitive(val, "float");
            const cJSON *quality = cJSON_GetObjectItemCaseSensitive(payload, "quality");
            const cJSON *why = cJSON_GetObjectItemCaseSensitive(payload, "reason");
            ok = cJSON_IsNumber(number) && number->valuedouble == value &&
                 cJSON_IsString(quality) && strcmp(quality->valuestring, "good") == 0 &&
                 cJSON_IsString(why) && strcmp(why->valuestring, reason) == 0;
            cJSON_Delete(payload);
        }
    }
    cJSON_Delete(root);
    return ok;
}

static uint64_t frame_event_id(const char *frame_json) {
    cJSON *root = cJSON_Parse(frame_json);
    if (!root) return 0;
    const cJSON *id = cJSON_GetObjectItemCaseSensitive(root, "id");
    uint64_t value = cJSON_IsNumber(id) ? (uint64_t)id->valuedouble : 0;
    cJSON_Delete(root);
    return value;
}

static int frame_platform_event_matches(const char *frame_json, const char *kind, uint32_t handle,
                                        const char *reason) {
    cJSON *root = cJSON_Parse(frame_json);
    if (!root) return 0;
    const cJSON *fn = cJSON_GetObjectItemCaseSensitive(root, "fn");
    const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    const cJSON *kind_item = cJSON_GetObjectItemCaseSensitive(args, "kind");
    const cJSON *point_item = cJSON_GetObjectItemCaseSensitive(args, "point");
    const cJSON *payload_text = cJSON_GetObjectItemCaseSensitive(args, "payload");
    int ok = cJSON_IsString(fn) && strcmp(fn->valuestring, "app_on_event") == 0 &&
             cJSON_IsString(kind_item) && strcmp(kind_item->valuestring, kind) == 0 &&
             cJSON_IsNull(point_item) && cJSON_IsString(payload_text);
    if (ok) {
        cJSON *payload = cJSON_Parse(payload_text->valuestring);
        ok = payload != NULL;
        if (ok) {
            const cJSON *handle_item = cJSON_GetObjectItemCaseSensitive(payload, "handle");
            const cJSON *why = cJSON_GetObjectItemCaseSensitive(payload, "reason");
            ok = cJSON_IsNumber(handle_item) && (uint32_t)handle_item->valuedouble == handle &&
                 cJSON_IsString(why) && strcmp(why->valuestring, reason) == 0;
            cJSON_Delete(payload);
        }
    }
    cJSON_Delete(root);
    return ok;
}

/* Drives the loop until the child writes one sync byte. */
static int run_loop_until_byte(uv_loop_t *loop, int fd, int timeout_ms) {
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        uv_run(loop, UV_RUN_NOWAIT);
        char byte = 0;
        if (read(fd, &byte, 1) == 1) return (unsigned char)byte;
        usleep(10000);
    }
    return -1;
}

static void make_sync_pipe(int fds[2]) {
    assert_int_equal(pipe(fds), 0);
    int flags = fcntl(fds[0], F_GETFL, 0);
    assert_true(flags >= 0);
    assert_int_equal(fcntl(fds[0], F_SETFL, flags | O_NONBLOCK), 0);
}

static void drain_loop(uv_loop_t *loop) {
    for (int i = 0; i < 20; i++) uv_run(loop, UV_RUN_NOWAIT);
}

static void child_client(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);

    uint32_t handle = 0;
    const char *access = NULL;
    const char *err = edge_app_point_resolve(sdk, "U_A", &handle, &access);
    if (err || handle != 7 || strcmp(access, "read") != 0) _exit(11);

    err = edge_app_point_resolve(sdk, "app.demo.value", &handle, &access);
    if (err || handle != 131072 + 5 || strcmp(access, "publish") != 0) _exit(12);

    err = edge_app_point_resolve(sdk, "missing", &handle, &access);
    if (!err || strcmp(err, "not_found") != 0) _exit(13);

    edge_app_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    err = edge_app_point_read(sdk, 7, &sample);
    if (err || sample.value.kind != EDGE_APP_VALUE_FLOAT || sample.value.number != 220.5)
        _exit(14);
    if (strcmp(sample.quality, "good") != 0 || sample.source_ts != 1720000000000ULL) _exit(15);

    edge_app_sample_t publish;
    memset(&publish, 0, sizeof(publish));
    publish.value.kind = EDGE_APP_VALUE_FLOAT;
    publish.value.number = 42.25;
    publish.quality = "good";
    publish.source_ts = 1720000000456ULL;
    err = edge_app_point_publish(sdk, 12, &publish);
    if (err) _exit(16);
    err = edge_app_point_publish(sdk, 9, &publish);
    if (!err || strcmp(err, "denied_scope") != 0) _exit(17);

    err = edge_app_log(sdk, "info", "hello from app");
    if (err) _exit(18);

    uint64_t now = 0;
    err = edge_app_now(sdk, &now);
    if (err || now != 1720000000123ULL) _exit(19);

    edge_app_sdk_destroy(sdk);
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

static void test_host_abi_server(void **state) {
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

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        child_client(fds[1]);
    }
    close(fds[1]);

    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    assert_int_equal(fake.resolve_calls, 3);
    assert_int_equal(fake.read_calls, 1);
    assert_int_equal(fake.publish_calls, 2);
    assert_int_equal(fake.log_calls, 1);
    assert_int_equal(fake.last_level, 1);
    assert_string_equal(fake.last_log, "hello from app");
    assert_int_equal(fake.last_sample.kind, APP_HOST_VALUE_FLOAT);
    assert_true(fake.last_sample.number == 42.25);
    assert_int_equal(fake.last_sample.source_ts, 1720000000456ULL);
    assert_int_equal(fake.now_calls, 1);

    app_host_server_destroy(server);
    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void child_calls_rate_limited(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    uint32_t handle = 0;
    const char *access = NULL;
    if (edge_app_point_resolve(sdk, "U_A", &handle, &access)) _exit(11);
    if (edge_app_point_resolve(sdk, "U_A", &handle, &access)) _exit(12);
    const char *err = edge_app_point_resolve(sdk, "U_A", &handle, &access);
    if (!err || strcmp(err, "rate_limited") != 0) _exit(13);
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_publish_rate_limited(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    edge_app_sample_t publish;
    memset(&publish, 0, sizeof(publish));
    publish.value.kind = EDGE_APP_VALUE_FLOAT;
    publish.value.number = 1.0;
    publish.quality = "good";
    publish.source_ts = 1720000000456ULL;
    if (edge_app_point_publish(sdk, 12, &publish)) _exit(11);
    const char *err = edge_app_point_publish(sdk, 12, &publish);
    if (!err || strcmp(err, "rate_limited") != 0) _exit(12);
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_unsupported_calls(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    const char *fns[] = {"setting_watch", "kv_get", "kv_set"};
    char result[128];
    for (size_t i = 0; i < sizeof(fns) / sizeof(fns[0]); i++) {
        const char *err = edge_app_sdk_call(sdk, fns[i], "{}", result, sizeof(result));
        if (!err || strcmp(err, "not_supported") != 0) _exit(12);
    }
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_subscribe_events(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);

    uint32_t sub = 0;
    const char *err = edge_app_point_subscribe(sdk, 7, 0.0, 0, 1000, &sub);
    if (err || sub != 1) _exit(11);
    err = edge_app_point_subscribe(sdk, 7, 0.0, 0, 1000, &sub);
    if (!err || strcmp(err, "busy") != 0) _exit(12);
    err = edge_app_point_subscribe(sdk, 7, -1.0, 0, 0, &sub);
    if (!err || strcmp(err, "invalid") != 0) _exit(13);
    err = edge_app_point_subscribe(sdk, 0, 0.0, 0, 0, &sub);
    if (!err || strcmp(err, "invalid") != 0) _exit(14);
    err = edge_app_point_subscribe(sdk, 7, 0.0, 100, 50, &sub);
    if (!err || strcmp(err, "invalid") != 0) _exit(15);

    char byte = '1';
    if (write(sync_fd, &byte, 1) != 1) _exit(16);
    char frame[4096];
    char response[256];
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(17);
    if (!frame_event_matches(frame, 7, 42.5, "change")) _exit(18);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(19);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(20);

    byte = '2';
    if (write(sync_fd, &byte, 1) != 1) _exit(21);
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(22);
    if (!frame_event_matches(frame, 7, 43.5, "change")) _exit(23);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(24);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(25);

    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_grant_revoked_event(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);

    uint32_t sub = 0;
    const char *err = edge_app_point_subscribe(sdk, 7, 0.0, 0, 0, &sub);
    if (err || sub != 1) _exit(11);
    char byte = '1';
    if (write(sync_fd, &byte, 1) != 1) _exit(12);

    char frame[4096];
    char response[256];
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(13);
    if (!frame_event_matches(frame, 7, 42.5, "change")) _exit(14);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(15);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(16);

    byte = '2';
    if (write(sync_fd, &byte, 1) != 1) _exit(17);

    /* The parent denies the grant before posting the next sample; the queued
     * event cannot be delivered, so grant_changed arrives instead. */
    struct timeval timeout = {2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(18);
    if (!frame_platform_event_matches(frame, "grant_changed", 7, "denied_revoked")) _exit(19);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(20);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(21);

    byte = '3';
    if (write(sync_fd, &byte, 1) != 1) _exit(22);

    timeout.tv_sec = 0;
    timeout.tv_usec = 200 * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) == 0) _exit(23);

    byte = '4';
    if (write(sync_fd, &byte, 1) != 1) _exit(24);

    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_subscribe_quota(int fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    for (uint32_t i = 0; i < 64; i++) {
        uint32_t sub = 0;
        const char *err = edge_app_point_subscribe(sdk, 100 + i, 0.0, 0, 0, &sub);
        if (err || sub != i + 1) _exit(11);
    }
    uint32_t sub = 0;
    const char *err = edge_app_point_subscribe(sdk, 500, 0.0, 0, 0, &sub);
    if (!err || strcmp(err, "denied_quota") != 0) _exit(12);
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_event_filters(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);

    uint32_t sub = 0;
    const char *err = edge_app_point_subscribe(sdk, 7, 1.0, 0, 0, &sub);
    if (err || sub != 1) _exit(11);

    char byte = '1';
    if (write(sync_fd, &byte, 1) != 1) _exit(12);
    char frame[4096];
    char response[256];
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(13);
    if (!frame_event_matches(frame, 7, 220.5, "change")) _exit(14);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(15);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(16);

    byte = '2';
    if (write(sync_fd, &byte, 1) != 1) _exit(17);
    struct timeval timeout = {0, 200 * 1000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) == 0) _exit(18);

    byte = '3';
    if (write(sync_fd, &byte, 1) != 1) _exit(19);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(20);
    if (!frame_event_matches(frame, 7, 230.0, "change")) _exit(21);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(22);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(23);

    sub = 0;
    err = edge_app_point_subscribe(sdk, 8, 0.0, 0, 100, &sub);
    if (err || sub == 0) _exit(24);
    byte = '4';
    if (write(sync_fd, &byte, 1) != 1) _exit(25);

    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(26);
    if (!frame_event_matches(frame, 8, 220.5, "keepalive")) _exit(27);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(28);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(29);

    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_withholds_ack(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    uint32_t sub = 0;
    const char *err = edge_app_point_subscribe(sdk, 7, 0.0, 0, 0, &sub);
    if (err || sub != 1) _exit(11);
    char byte = '1';
    if (write(sync_fd, &byte, 1) != 1) _exit(12);

    char frame[4096];
    char response[256];
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(13);
    if (!frame_event_matches(frame, 7, 42.5, "change")) _exit(14);
    /* Withhold this ack; the host must still deliver the next event. */
    byte = '2';
    if (write(sync_fd, &byte, 1) != 1) _exit(15);

    struct timeval timeout = {8, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(16);
    if (!frame_event_matches(frame, 7, 43.5, "change")) _exit(17);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(18);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(19);
    byte = '3';
    if (write(sync_fd, &byte, 1) != 1) _exit(20);

    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_late_ack(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    uint32_t sub = 0;
    const char *err = edge_app_point_subscribe(sdk, 7, 0.0, 0, 0, &sub);
    if (err || sub != 1) _exit(11);

    char byte = '1';
    if (write(sync_fd, &byte, 1) != 1) _exit(12);
    char frame[4096];
    char response[256];
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(13);
    if (!frame_event_matches(frame, 7, 42.5, "change")) _exit(14);
    uint64_t first_id = frame_event_id(frame);
    if (first_id == 0) _exit(15);
    /* Withhold this ack past APP_HOST_ACK_TIMEOUT_MS. */
    byte = '2';
    if (write(sync_fd, &byte, 1) != 1) _exit(16);

    struct timeval timeout = {8, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(17);
    if (!frame_event_matches(frame, 7, 43.5, "change")) _exit(18);
    uint64_t second_id = frame_event_id(frame);
    if (second_id == 0 || second_id == first_id) _exit(19);

    /* A late ack for the timed-out event must not clear the second one. */
    snprintf(response, sizeof(response), "{\"id\":%llu}", (unsigned long long)first_id);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(20);
    byte = '3';
    if (write(sync_fd, &byte, 1) != 1) _exit(21);

    timeout.tv_sec = 0;
    timeout.tv_usec = 300 * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) == 0) _exit(22);

    snprintf(response, sizeof(response), "{\"id\":%llu}", (unsigned long long)second_id);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(23);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(24);
    if (!frame_event_matches(frame, 7, 44.5, "change")) _exit(25);
    if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(26);
    if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(27);

    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_wait_for_eof(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    uint32_t sub = 0;
    const char *err = edge_app_point_subscribe(sdk, 7, 0.0, 0, 0, &sub);
    if (err || sub != 1) _exit(11);
    char byte = 'R';
    if (write(sync_fd, &byte, 1) != 1) _exit(12);
    char frame[128];
    (void)edge_app_sdk_next_frame(sdk, frame, sizeof frame);
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static int frame_tick_matches(const char *frame_json, uint32_t tick_ms) {
    cJSON *root = cJSON_Parse(frame_json);
    if (!root) return 0;
    const cJSON *fn = cJSON_GetObjectItemCaseSensitive(root, "fn");
    const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");
    const cJSON *kind = cJSON_GetObjectItemCaseSensitive(args, "kind");
    const cJSON *point = cJSON_GetObjectItemCaseSensitive(args, "point");
    const cJSON *payload_text = cJSON_GetObjectItemCaseSensitive(args, "payload");
    int ok = cJSON_IsString(fn) && strcmp(fn->valuestring, "app_on_event") == 0 &&
             cJSON_IsString(kind) && strcmp(kind->valuestring, "tick") == 0 &&
             cJSON_IsNull(point) && cJSON_IsString(payload_text);
    if (ok) {
        cJSON *payload = cJSON_Parse(payload_text->valuestring);
        ok = payload != NULL;
        if (ok) {
            const cJSON *tick = cJSON_GetObjectItemCaseSensitive(payload, "tick_ms");
            ok = cJSON_IsNumber(tick) && (uint32_t)tick->valuedouble == tick_ms;
            cJSON_Delete(payload);
        }
    }
    cJSON_Delete(root);
    return ok;
}

static void child_receives_ticks(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    char frame[4096];
    char response[256];
    for (int i = 0; i < 2; i++) {
        if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(11);
        if (!frame_tick_matches(frame, 50)) _exit(12);
        if (edge_app_sdk_dispatch(sdk, frame, response, sizeof response) != 0) _exit(13);
        if (edge_app_sdk_send_frame(sdk, response) != 0) _exit(14);
    }
    char byte = 'D';
    if (write(sync_fd, &byte, 1) != 1) _exit(15);
    /* Wait for the host to close. */
    (void)edge_app_sdk_next_frame(sdk, frame, sizeof frame);
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void child_receives_no_tick(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    struct timeval timeout = {0, 300 * 1000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    char byte = 'R';
    if (write(sync_fd, &byte, 1) != 1) _exit(11);
    char frame[256];
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) {
        edge_app_sdk_destroy(sdk);
        _exit(0);
    }
    _exit(12);
}

static void child_holds_tick_ack(int fd, int sync_fd) {
    edge_app_sdk_t *sdk = edge_app_sdk_create(fd);
    if (!sdk) _exit(10);
    char frame[4096];
    if (edge_app_sdk_next_frame(sdk, frame, sizeof frame) != 0) _exit(11);
    if (!frame_tick_matches(frame, 50)) _exit(12);
    char byte = '1';
    if (write(sync_fd, &byte, 1) != 1) _exit(13);
    struct timeval timeout = {2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    (void)edge_app_sdk_next_frame(sdk, frame, sizeof frame);
    edge_app_sdk_destroy(sdk);
    _exit(0);
}

static void run_rate_case(void (*child)(int), uint32_t calls_per_sec, uint32_t publish_per_sec) {
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
        .calls_per_sec = calls_per_sec,
        .publish_per_sec = publish_per_sec,
    };
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);
    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        child(fds[1]);
    }
    close(fds[1]);
    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);
    app_host_server_destroy(server);
    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_call_rate_limit(void **state) {
    (void)state;
    run_rate_case(child_calls_rate_limited, 2, 0);
}

static void test_host_publish_rate_limit(void **state) {
    (void)state;
    run_rate_case(child_publish_rate_limited, 0, 1);
}

static app_host_server_t *create_subscribe_server(uv_loop_t *loop, int fds[2], fake_ports_t *fake,
                                                  app_host_ports_t *ports) {
    memset(fake, 0, sizeof(*fake));
    fake->next_token = 7;
    *ports = (app_host_ports_t){
        .ctx = fake,
        .resolve = fake_resolve,
        .read = fake_read,
        .publish = fake_publish,
        .allow = fake_allow,
        .subscribe = fake_subscribe,
        .unsubscribe = fake_unsubscribe,
        .log = fake_log,
        .now_ms = fake_now,
    };
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    app_host_server_t *server = app_host_server_create(loop, fds[0], ports);
    assert_non_null(server);
    return server;
}

static void test_host_subscribe(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    fake_ports_t fake;
    app_host_ports_t ports;
    app_host_server_t *server = create_subscribe_server(&loop, fds, &fake, &ports);
    (void)server;
    make_sync_pipe(sync);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_subscribe_events(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '1');
    assert_int_equal(fake.subscribe_calls, 1);
    assert_int_equal(fake.last_sub_handle, 7);
    app_host_sample_t first = event_sample(42.5, 6000);
    fake.sink(fake.sink_ctx, 7, &first);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '2');
    app_host_sample_t second = event_sample(43.5, 7000);
    fake.sink(fake.sink_ctx, 7, &second);

    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    drain_loop(&loop);
    assert_int_equal(fake.unsubscribe_calls, 1);
    close(sync[0]);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_grant_revoked_event(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    fake_ports_t fake;
    app_host_ports_t ports;
    app_host_server_t *server = create_subscribe_server(&loop, fds, &fake, &ports);
    (void)server;
    make_sync_pipe(sync);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_grant_revoked_event(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '1');
    app_host_sample_t first = event_sample(42.5, 6000);
    fake.sink(fake.sink_ctx, 7, &first);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '2');
    fake.allow_denied = 1;
    app_host_sample_t revoked = event_sample(43.5, 7000);
    fake.sink(fake.sink_ctx, 7, &revoked);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '3');
    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '4');

    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    drain_loop(&loop);
    assert_int_equal(fake.allow_calls, 2);
    assert_int_equal(fake.unsubscribe_calls, 1);
    close(sync[0]);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_subscribe_quota(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    fake_ports_t fake;
    app_host_ports_t ports;
    app_host_server_t *server = create_subscribe_server(&loop, fds, &fake, &ports);
    (void)server;

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        child_subscribe_quota(fds[1]);
    }
    close(fds[1]);

    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    drain_loop(&loop);
    assert_int_equal(fake.subscribe_calls, 64);
    assert_int_equal(fake.unsubscribe_calls, 64);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_event_filters(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    fake_ports_t fake;
    app_host_ports_t ports;
    app_host_server_t *server = create_subscribe_server(&loop, fds, &fake, &ports);
    (void)server;
    make_sync_pipe(sync);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_event_filters(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '1');
    app_host_sample_t first = event_sample(220.5, 6000);
    fake.sink(fake.sink_ctx, 7, &first);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '2');
    app_host_sample_t within = event_sample(220.6, 7000);
    fake.sink(fake.sink_ctx, 7, &within);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '3');
    app_host_sample_t beyond = event_sample(230.0, 8000);
    fake.sink(fake.sink_ctx, 7, &beyond);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '4');
    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    assert_true(fake.read_calls >= 1);
    drain_loop(&loop);
    assert_int_equal(fake.unsubscribe_calls, 2);
    close(sync[0]);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_ack_timeout(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    fake_ports_t fake;
    app_host_ports_t ports;
    app_host_server_t *server = create_subscribe_server(&loop, fds, &fake, &ports);
    (void)server;
    make_sync_pipe(sync);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_withholds_ack(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '1');
    app_host_sample_t first = event_sample(42.5, 6000);
    fake.sink(fake.sink_ctx, 7, &first);
    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '2');

    /* Queued behind the unacked event. No keepalive subscription is active, so
     * only the ack deadline can release it. */
    app_host_sample_t second = event_sample(43.5, 7000);
    fake.sink(fake.sink_ctx, 7, &second);
    assert_int_equal(run_loop_until_byte(&loop, sync[0], 8000), '3');

    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    drain_loop(&loop);
    assert_int_equal(fake.unsubscribe_calls, 1);
    close(sync[0]);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_late_ack_ignored(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    fake_ports_t fake;
    app_host_ports_t ports;
    app_host_server_t *server = create_subscribe_server(&loop, fds, &fake, &ports);
    (void)server;
    make_sync_pipe(sync);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_late_ack(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '1');
    app_host_sample_t first = event_sample(42.5, 6000);
    fake.sink(fake.sink_ctx, 7, &first);
    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '2');

    /* The second event waits behind the first until the ack deadline passes. */
    app_host_sample_t second = event_sample(43.5, 7000);
    fake.sink(fake.sink_ctx, 7, &second);
    assert_int_equal(run_loop_until_byte(&loop, sync[0], 8000), '3');

    /* The child sent the first event's id after the timeout. The third event
     * must stay queued until the child acks the second event by its own id. */
    app_host_sample_t third = event_sample(44.5, 8000);
    fake.sink(fake.sink_ctx, 7, &third);

    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    drain_loop(&loop);
    assert_int_equal(fake.unsubscribe_calls, 1);
    close(sync[0]);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_teardown(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    fake_ports_t fake;
    app_host_ports_t ports;
    app_host_server_t *server = create_subscribe_server(&loop, fds, &fake, &ports);
    make_sync_pipe(sync);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_wait_for_eof(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), 'R');
    app_host_server_destroy(server);
    app_host_sample_t late = event_sample(1.0, 1);
    fake.sink(fake.sink_ctx, 7, &late);
    assert_int_equal(fake.unsubscribe_calls, 1);

    int status = run_loop_until(&loop, pid, 5000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    close(sync[0]);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_tick_delivery(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    make_sync_pipe(sync);
    app_host_ports_t ports = {0};
    ports.tick_ms = 50;
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_receives_ticks(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), 'D');
    uint64_t enqueued = 0;
    uint64_t skipped = 0;
    app_host_server_tick_counters(server, &enqueued, &skipped);
    assert_true(enqueued >= 2);

    app_host_server_destroy(server);
    int status = run_loop_until(&loop, pid, 3000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);
    close(sync[0]);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_tick_absent_without_manifest_field(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    make_sync_pipe(sync);
    app_host_ports_t ports = {0};
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_receives_no_tick(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), 'R');
    int status = run_loop_until(&loop, pid, 3000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);
    uint64_t enqueued = 99;
    uint64_t skipped = 99;
    app_host_server_tick_counters(server, &enqueued, &skipped);
    assert_int_equal(enqueued, 0);
    assert_int_equal(skipped, 0);

    app_host_server_destroy(server);
    close(sync[0]);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_tick_skips_when_unacked(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    int sync[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    make_sync_pipe(sync);
    app_host_ports_t ports = {0};
    ports.tick_ms = 50;
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        close(sync[0]);
        child_holds_tick_ack(fds[1], sync[1]);
    }
    close(fds[1]);
    close(sync[1]);

    assert_int_equal(run_loop_until_byte(&loop, sync[0], 5000), '1');
    for (int waited = 0; waited < 300; waited += 10) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    uint64_t enqueued = 0;
    uint64_t skipped = 0;
    app_host_server_tick_counters(server, &enqueued, &skipped);
    assert_int_equal(enqueued, 1);
    assert_true(skipped >= 2);

    app_host_server_destroy(server);
    int status = run_loop_until(&loop, pid, 3000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);
    close(sync[0]);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_rate_limit_requires_clock(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    app_host_ports_t ports = {0};
    ports.calls_per_sec = 1;

    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    assert_null(app_host_server_create(&loop, fds[0], &ports));

    close(fds[0]);
    close(fds[1]);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_host_unsupported_calls(void **state) {
    (void)state;
    run_rate_case(child_unsupported_calls, 0, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_host_abi_server),
        cmocka_unit_test(test_host_call_rate_limit),
        cmocka_unit_test(test_host_publish_rate_limit),
        cmocka_unit_test(test_host_unsupported_calls),
        cmocka_unit_test(test_host_subscribe),
        cmocka_unit_test(test_host_grant_revoked_event),
        cmocka_unit_test(test_host_subscribe_quota),
        cmocka_unit_test(test_host_event_filters),
        cmocka_unit_test(test_host_ack_timeout),
        cmocka_unit_test(test_host_late_ack_ignored),
        cmocka_unit_test(test_host_teardown),
        cmocka_unit_test(test_host_tick_delivery),
        cmocka_unit_test(test_host_tick_absent_without_manifest_field),
        cmocka_unit_test(test_host_tick_skips_when_unacked),
        cmocka_unit_test(test_host_rate_limit_requires_clock),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
