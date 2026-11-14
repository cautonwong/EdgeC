/**
 * @file test_app_host_server.c
 * @brief Host ABI server driven by the real C SDK over a socketpair.
 */

#include <cmocka.h>
#include <errno.h>
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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_host_abi_server),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
