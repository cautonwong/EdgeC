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
    int log_calls;
    uint32_t read_handle;
    uint32_t publish_handle;
    app_host_sample_t last_sample;
    char last_log[128];
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
            fake->log_calls >= 1) {
            return 0;
        }
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

    /* Closing the host end lets the probe leave its wait loop and exit. */
    app_host_server_destroy(server);
    int status = wait_child(&loop, pid, 3000);
    assert_true(status != -1);
    assert_int_equal(WEXITSTATUS(status), 0);

    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_app_host_process),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
