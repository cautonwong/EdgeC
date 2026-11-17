/**
 * @file test_app_host_process.c
 * @brief Spawns app-host with the ABI probe wasm and checks the forwarded calls.
 */

#include <cmocka.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "edge_app_host.h"

typedef struct {
    int resolve_calls;
    int read_calls;
    int publish_calls;
    int subscribe_calls;
    int unsubscribe_calls;
    int log_calls;
    uint32_t read_handle;
    uint32_t publish_handle;
    uint32_t subscribe_handle;
    app_host_sample_t last_sample;
    app_host_event_cb_t sink;
    void *sink_ctx;
    char last_log[512];
} fake_ports_t;

static int fake_resolve(void *ctx, const char *name, uint32_t *handle, uint8_t *access,
                        char *reason, size_t reason_size) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->resolve_calls++;
    if (strcmp(name, "app.demo.value") == 0) {
        *handle = 131072;
        *access = APP_HOST_ACCESS_PUBLISH;
        return 0;
    }
    if (strcmp(name, "U_A") == 0) {
        *handle = 1001;
        *access = APP_HOST_ACCESS_READ;
        return 0;
    }
    snprintf(reason, reason_size, "not_found");
    return -1;
}

static int fake_read(void *ctx, uint32_t handle, app_host_sample_t *out, char *reason,
                     size_t reason_size) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->read_calls++;
    fake->read_handle = handle;
    if (handle != 1001) {
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
    fake->subscribe_calls++;
    fake->subscribe_handle = handle;
    fake->sink = on_event;
    fake->sink_ctx = event_ctx;
    *out_sub_id = 7;
    (void)reason;
    (void)reason_size;
    return 0;
}

static int fake_unsubscribe(void *ctx, int32_t sub_id) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    (void)sub_id;
    fake->unsubscribe_calls++;
    return 0;
}

static int fake_publish(void *ctx, uint32_t handle, const app_host_sample_t *sample, char *reason,
                        size_t reason_size) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    fake->publish_calls++;
    fake->publish_handle = handle;
    fake->last_sample = *sample;
    (void)reason;
    (void)reason_size;
    return 0;
}

static void fake_log(void *ctx, int level, const char *message) {
    fake_ports_t *fake = (fake_ports_t *)ctx;
    (void)level;
    fake->log_calls++;
    snprintf(fake->last_log, sizeof(fake->last_log), "%s", message);
}

static uint64_t fake_now(void *ctx) {
    (void)ctx;
    return 1720000000123ULL;
}

static int wait_for_calls(uv_loop_t *loop, fake_ports_t *fake, int timeout_ms) {
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (fake->resolve_calls >= 1 && fake->read_calls >= 1 && fake->publish_calls >= 1 &&
            fake->subscribe_calls >= 1 && fake->log_calls >= 1) {
            return 0;
        }
        usleep(10000);
    }
    return -1;
}

/* The second event is only delivered after the child acks the first. */
static int wait_for_log(uv_loop_t *loop, fake_ports_t *fake, const char *needle, int timeout_ms) {
    for (int waited = 0; waited < timeout_ms; waited += 10) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (strstr(fake->last_log, needle) != NULL) return 0;
        usleep(10000);
    }
    return -1;
}

static int wait_child(uv_loop_t *loop, pid_t pid, int timeout_ms) {
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

static void test_app_host_process(void **state) {
    (void)state;
    /* The test container blocks seccomp; app-host fails closed without this. */
    setenv("EDGEC_APP_HOST_ALLOW_DEGRADED", "1", 1);
    const char *app_host = getenv("EDGEC_APP_HOST_PATH");
    const char *probe = getenv("EDGEC_ABI_PROBE_WASM");
    assert_non_null(app_host);
    assert_non_null(probe);

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
        .subscribe = fake_subscribe,
        .unsubscribe = fake_unsubscribe,
        .log = fake_log,
        .now_ms = fake_now,
    };
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        char fd_text[16];
        snprintf(fd_text, sizeof(fd_text), "%d", fds[1]);
        char *args[] = {"app-host", "--fd", fd_text, "--wasm", (char *)probe, NULL};
        execv(app_host, args);
        _exit(127);
    }
    close(fds[1]);

    assert_int_equal(wait_for_calls(&loop, &fake, 3000), 0);
    assert_int_equal(fake.resolve_calls, 1);
    assert_int_equal(fake.read_calls, 1);
    assert_int_equal(fake.read_handle, 1001);
    assert_int_equal(fake.publish_calls, 1);
    assert_int_equal(fake.publish_handle, 131072);
    assert_int_equal(fake.last_sample.kind, APP_HOST_VALUE_FLOAT);
    assert_true(fake.last_sample.number == 221.5);
    assert_int_equal(fake.log_calls, 1);
    assert_string_equal(fake.last_log, "probe done");
    assert_int_equal(fake.subscribe_calls, 1);
    assert_int_equal(fake.subscribe_handle, 1001);
    assert_non_null(fake.sink);

    app_host_sample_t event = {0};
    event.kind = APP_HOST_VALUE_FLOAT;
    event.number = 244.75;
    event.quality = APP_HOST_QUALITY_GOOD;
    event.source_ts = 6000;
    fake.sink(fake.sink_ctx, 1001, &event);
    assert_int_equal(wait_for_log(&loop, &fake, "244.75", 3000), 0);
    assert_non_null(strstr(fake.last_log, "point_change"));

    event.number = 245.25;
    event.source_ts = 7000;
    fake.sink(fake.sink_ctx, 1001, &event);
    assert_int_equal(wait_for_log(&loop, &fake, "245.25", 3000), 0);

    /* No tick_ms in the launch spec: the probe must not receive tick events. */
    for (int waited = 0; waited < 200; waited += 10) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_null(strstr(fake.last_log, "tick"));

    /* Closing the host end lets the probe leave its wait loop and exit. */
    app_host_server_destroy(server);
    int status = wait_child(&loop, pid, 3000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(fake.unsubscribe_calls, 1);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_app_host_process_tick(void **state) {
    (void)state;
    setenv("EDGEC_APP_HOST_ALLOW_DEGRADED", "1", 1);
    const char *app_host = getenv("EDGEC_APP_HOST_PATH");
    const char *probe = getenv("EDGEC_ABI_PROBE_WASM");
    assert_non_null(app_host);
    assert_non_null(probe);

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
        .subscribe = fake_subscribe,
        .unsubscribe = fake_unsubscribe,
        .log = fake_log,
        .now_ms = fake_now,
        .tick_ms = 50,
    };
    app_host_server_t *server = app_host_server_create(&loop, fds[0], &ports);
    assert_non_null(server);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        char fd_text[16];
        snprintf(fd_text, sizeof(fd_text), "%d", fds[1]);
        char *args[] = {"app-host", "--fd", fd_text, "--wasm", (char *)probe, NULL};
        execv(app_host, args);
        _exit(127);
    }
    close(fds[1]);

    assert_int_equal(wait_for_log(&loop, &fake, "tick_ms", 3000), 0);
    assert_non_null(strstr(fake.last_log, "\"kind\":\"tick\""));
    uint64_t enqueued = 0;
    uint64_t skipped = 0;
    app_host_server_tick_counters(server, &enqueued, &skipped);
    assert_true(enqueued >= 1);

    app_host_server_destroy(server);
    int status = wait_child(&loop, pid, 3000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/* Without the degraded override, app-host must not run when the no-exec filter
 * cannot be installed. Where the filter is available it reaches arg parsing. */
static void test_no_exec_filter_fails_closed(void **state) {
    (void)state;
    const char *app_host = getenv("EDGEC_APP_HOST_PATH");
    assert_non_null(app_host);
    unsetenv("EDGEC_APP_HOST_ALLOW_DEGRADED");

    int fds[2];
    assert_int_equal(pipe(fds), 0);
    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], STDERR_FILENO);
        execl(app_host, "app-host", (char *)NULL);
        _exit(127);
    }
    close(fds[1]);
    char buffer[1024];
    ssize_t n = read(fds[0], buffer, sizeof(buffer) - 1);
    if (n < 0) n = 0;
    buffer[n] = '\0';
    close(fds[0]);

    int status = 0;
    assert_int_equal(waitpid(pid, &status, 0), pid);
    assert_true(WIFEXITED(status));
    assert_int_not_equal(WEXITSTATUS(status), 0);
    assert_true(strstr(buffer, "failed to install no-exec filter") != NULL ||
                strstr(buffer, "usage:") != NULL);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_app_host_process),
        cmocka_unit_test(test_app_host_process_tick),
        cmocka_unit_test(test_no_exec_filter_fails_closed),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
