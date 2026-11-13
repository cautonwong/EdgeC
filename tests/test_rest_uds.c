/**
 * @file test_rest_uds.c
 * @brief Management socket: framing, peer auth, and the appctl CLI journey.
 */

#include <cmocka.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "datahub_priv.h"
#include "edge_service.h"
#include "rest_control_port.h"
#include "rest_server.h"
#include "rest_uds_server.h"

typedef struct {
    uint8_t package[16];
    size_t package_len;
    uint8_t expected_pubkey[32];
    int have_expected_pubkey;
    char key[32];
    char app_id[32];
    char disposition[16];
    char export_path[256];
    char capability[32];
    char scope[64];
    char purpose[64];
    char status_app[32];
    int install_calls;
    int uninstall_calls;
    int start_calls;
    int upgrade_calls;
    int grant_calls;
    int status_calls;
    int snapshot_calls;
} fake_ctl_t;

static const char *kStatusJson = "{\"generation\":7,\"apps\":0}";
static const char *kSnapshotJson = "{\"generation\":7,\"status\":\"ok\",\"path\":\"/tmp/gen-7.json\"}";

static int fake_install_package(void *ctx, const uint8_t *data, size_t len, const char *key,
                                const uint8_t expected_pubkey[32]) {
    fake_ctl_t *fake = (fake_ctl_t *)ctx;
    fake->install_calls++;
    fake->package_len = len;
    if (len <= sizeof(fake->package)) memcpy(fake->package, data, len);
    snprintf(fake->key, sizeof(fake->key), "%s", key ? key : "");
    if (expected_pubkey) {
        memcpy(fake->expected_pubkey, expected_pubkey, 32);
        fake->have_expected_pubkey = 1;
    }
    return 0;
}

static int fake_upgrade_package(void *ctx, const char *app_id, const uint8_t *data, size_t len,
                                const char *key) {
    fake_ctl_t *fake = (fake_ctl_t *)ctx;
    fake->upgrade_calls++;
    fake->package_len = len;
    if (len <= sizeof(fake->package)) memcpy(fake->package, data, len);
    snprintf(fake->app_id, sizeof(fake->app_id), "%s", app_id ? app_id : "");
    snprintf(fake->key, sizeof(fake->key), "%s", key ? key : "");
    return 0;
}

static int fake_start_app(void *ctx, const char *app_id) {
    fake_ctl_t *fake = (fake_ctl_t *)ctx;
    fake->start_calls++;
    snprintf(fake->app_id, sizeof(fake->app_id), "%s", app_id ? app_id : "");
    return 0;
}

static int fake_uninstall_app(void *ctx, const char *app_id, const char *disposition,
                              const char *export_path) {
    fake_ctl_t *fake = (fake_ctl_t *)ctx;
    fake->uninstall_calls++;
    snprintf(fake->app_id, sizeof(fake->app_id), "%s", app_id ? app_id : "");
    snprintf(fake->disposition, sizeof(fake->disposition), "%s", disposition ? disposition : "");
    snprintf(fake->export_path, sizeof(fake->export_path), "%s",
             export_path ? export_path : "");
    return 0;
}

static int fake_approve_grant(void *ctx, const char *app_id, const char *capability,
                              const char *scope, const char *purpose, uint64_t expires_at,
                              uint8_t sensitivity, const char *actor) {
    (void)expires_at;
    (void)sensitivity;
    (void)actor;
    fake_ctl_t *fake = (fake_ctl_t *)ctx;
    fake->grant_calls++;
    snprintf(fake->app_id, sizeof(fake->app_id), "%s", app_id ? app_id : "");
    snprintf(fake->capability, sizeof(fake->capability), "%s", capability ? capability : "");
    snprintf(fake->scope, sizeof(fake->scope), "%s", scope ? scope : "");
    snprintf(fake->purpose, sizeof(fake->purpose), "%s", purpose ? purpose : "");
    return 0;
}

static char *fake_status_json(void *ctx, const char *app_id) {
    fake_ctl_t *fake = (fake_ctl_t *)ctx;
    fake->status_calls++;
    snprintf(fake->status_app, sizeof(fake->status_app), "%s", app_id ? app_id : "");
    return strdup(kStatusJson);
}

static char *fake_registry_snapshot(void *ctx) {
    fake_ctl_t *fake = (fake_ctl_t *)ctx;
    fake->snapshot_calls++;
    return strdup(kSnapshotJson);
}

static char *fake_apps_json(void *ctx) {
    (void)ctx;
    return strdup("[\"demo\"]");
}

static void put_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value & 0xffu);
    p[1] = (uint8_t)((value >> 8) & 0xffu);
    p[2] = (uint8_t)((value >> 16) & 0xffu);
    p[3] = (uint8_t)((value >> 24) & 0xffu);
}

static uint32_t get_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int write_all(int fd, const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    size_t written = 0;
    while (written < len) {
        ssize_t chunk = write(fd, bytes + written, len - written);
        if (chunk < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        written += (size_t)chunk;
    }
    return 0;
}

static int read_all(int fd, void *data, size_t len) {
    uint8_t *bytes = (uint8_t *)data;
    size_t got = 0;
    while (got < len) {
        ssize_t chunk = read(fd, bytes + got, len - got);
        if (chunk < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (chunk == 0) return -1;
        got += (size_t)chunk;
    }
    return 0;
}

static int connect_sock(const char *path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int child_call(const char *path, const char *method, const char *route, const char *body,
                      int out_fd) {
    int fd = connect_sock(path);
    if (fd < 0) return 2;
    size_t body_len = body ? strlen(body) : 0;
    uint8_t header[16];
    memcpy(header, REST_UDS_MAGIC, 4);
    put_u32(header + 4, (uint32_t)strlen(method));
    put_u32(header + 8, (uint32_t)strlen(route));
    put_u32(header + 12, (uint32_t)body_len);
    if (write_all(fd, header, sizeof(header)) != 0 || write_all(fd, method, strlen(method)) != 0 ||
        write_all(fd, route, strlen(route)) != 0 ||
        (body_len && write_all(fd, body, body_len) != 0)) {
        close(fd);
        return 3;
    }
    uint8_t reply[12];
    if (read_all(fd, reply, sizeof(reply)) != 0 || memcmp(reply, REST_UDS_MAGIC, 4) != 0) {
        close(fd);
        return 4;
    }
    uint32_t response_len = get_u32(reply + 8);
    char *text = malloc((size_t)response_len + 1);
    if (!text) {
        close(fd);
        return 5;
    }
    if (response_len && read_all(fd, text, response_len) != 0) {
        free(text);
        close(fd);
        return 6;
    }
    text[response_len] = '\0';
    ssize_t ignored = write(out_fd, text, response_len);
    (void)ignored;
    free(text);
    close(fd);
    return 0;
}

static int child_unauthorized(const char *path) {
    if (setgid(65534) != 0 || setuid(65534) != 0) return 77;
    signal(SIGPIPE, SIG_IGN);
    int fd = connect_sock(path);
    if (fd < 0) return 0; /* permission denied is also a rejection */
    uint8_t header[16];
    memcpy(header, REST_UDS_MAGIC, 4);
    put_u32(header + 4, 3);
    put_u32(header + 8, 19);
    put_u32(header + 12, 0);
    ssize_t n = -1;
    if (write_all(fd, header, sizeof(header)) == 0 && write_all(fd, "GET", 3) == 0 &&
        write_all(fd, "/api/v1/apps/list", 19) == 0) {
        char byte;
        n = read(fd, &byte, 1);
    }
    close(fd);
    return n <= 0 ? 0 : 65;
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

static char *read_fd_all(int fd) {
    char *buffer = malloc(8192);
    assert_non_null(buffer);
    size_t len = 0;
    while (len + 1 < 8192) {
        ssize_t chunk = read(fd, buffer + len, 8192 - len - 1);
        if (chunk <= 0) break;
        len += (size_t)chunk;
    }
    buffer[len] = '\0';
    return buffer;
}

static int run_appctl(uv_loop_t *loop, const char *sock, char *const argv[], char **output) {
    const char *appctl = getenv("EDGEC_APPCTL_PATH");
    assert_non_null(appctl);

    int pipefd[2];
    assert_int_equal(pipe(pipefd), 0);
    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        setenv("EDGEC_APPCTL_SOCK", sock, 1);
        execv(appctl, argv);
        _exit(127);
    }
    close(pipefd[1]);
    int status = run_loop_until(loop, pid, 5000);
    *output = read_fd_all(pipefd[0]);
    close(pipefd[0]);
    return WEXITSTATUS(status);
}

static void test_uds_management_channel(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    char path[] = "/tmp/edgeos_ctl_XXXXXX";
    int fd = mkstemp(path);
    assert_true(fd >= 0);
    close(fd);
    unlink(path);

    datahub_t *hub = calloc(1, sizeof(datahub_t));
    assert_non_null(hub);
    service_t *svc = service_new(&loop, hub, (arbiter_t *)0x1234, NULL, NULL);
    assert_non_null(svc);

    fake_ctl_t fake;
    memset(&fake, 0, sizeof(fake));
    rest_control_port_t control;
    memset(&control, 0, sizeof(control));
    control.ctx = &fake;
    control.install_package = fake_install_package;
    control.upgrade_package = fake_upgrade_package;
    control.start_app = fake_start_app;
    control.uninstall_app = fake_uninstall_app;
    control.approve_grant = fake_approve_grant;
    control.registry_status_json = fake_status_json;
    control.registry_snapshot = fake_registry_snapshot;
    control.apps_json = fake_apps_json;
    service_set_control_port(svc, &control);

    rest_uds_server_t *srv = rest_uds_server_start(&loop, svc, path);
    assert_non_null(srv);

    int pipefd[2];
    assert_int_equal(pipe(pipefd), 0);
    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(pipefd[0]);
        _exit(child_call(path, "GET", "/api/v1/apps/status", NULL, pipefd[1]));
    }
    close(pipefd[1]);
    int status = run_loop_until(&loop, pid, 5000);
    char *response = read_fd_all(pipefd[0]);
    close(pipefd[0]);
    assert_int_equal(WEXITSTATUS(status), 0);
    assert_non_null(strstr(response, "\"generation\":7"));
    free(response);

    if (geteuid() == 0) {
        /* Open the socket file so only the peer-uid check can reject. */
        chmod(path, 0666);
        pid = fork();
        assert_true(pid >= 0);
        if (pid == 0) _exit(child_unauthorized(path));
        status = run_loop_until(&loop, pid, 5000);
        assert_int_equal(WEXITSTATUS(status), 0);
        chmod(path, 0600);
    }

    char package_path[] = "/tmp/edgeos_pkg_XXXXXX";
    fd = mkstemp(package_path);
    assert_true(fd >= 0);
    const uint8_t payload[4] = {0x45, 0x44, 0x47, 0x45};
    assert_int_equal(write_all(fd, payload, sizeof(payload)), 0);
    close(fd);

    char key_path[] = "/tmp/edgeos_key_XXXXXX";
    fd = mkstemp(key_path);
    assert_true(fd >= 0);
    const char *key_hex =
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f\n";
    assert_int_equal(write_all(fd, key_hex, strlen(key_hex)), 0);
    close(fd);

    char *output = NULL;
    char *install_args[] = {"appctl", "install", package_path, "--key", key_path,
                            "--idempotency-key", "req-1", NULL};
    assert_int_equal(run_appctl(&loop, path, install_args, &output), 0);
    assert_non_null(strstr(output, "\"status\":\"ok\""));
    free(output);
    assert_int_equal(fake.install_calls, 1);
    assert_int_equal(fake.package_len, sizeof(payload));
    assert_memory_equal(fake.package, payload, sizeof(payload));
    assert_string_equal(fake.key, "req-1");
    assert_int_equal(fake.have_expected_pubkey, 1);
    assert_int_equal(fake.expected_pubkey[0], 0x00);
    assert_int_equal(fake.expected_pubkey[31], 0x1f);

    char *status_args[] = {"appctl", "status", NULL};
    assert_int_equal(run_appctl(&loop, path, status_args, &output), 0);
    assert_non_null(strstr(output, "\"generation\":7"));
    free(output);
    assert_string_equal(fake.status_app, "");

    char *app_status_args[] = {"appctl", "status", "demo", NULL};
    assert_int_equal(run_appctl(&loop, path, app_status_args, &output), 0);
    free(output);
    assert_string_equal(fake.status_app, "demo");

    char *start_args[] = {"appctl", "start", "demo", NULL};
    assert_int_equal(run_appctl(&loop, path, start_args, &output), 0);
    free(output);
    assert_int_equal(fake.start_calls, 1);
    assert_string_equal(fake.app_id, "demo");

    char *upgrade_args[] = {"appctl", "upgrade", "demo", package_path, NULL};
    assert_int_equal(run_appctl(&loop, path, upgrade_args, &output), 0);
    free(output);
    assert_int_equal(fake.upgrade_calls, 1);
    assert_int_equal(fake.package_len, sizeof(payload));
    assert_memory_equal(fake.package, payload, sizeof(payload));
    assert_string_equal(fake.app_id, "demo");

    char *grant_args[] = {"appctl",      "grant", "demo", "tags.read",
                          "--scope",     "[\"U_A\",\"U_B\"]",
                          "--purpose",   "say \"hi\"", NULL};
    assert_int_equal(run_appctl(&loop, path, grant_args, &output), 0);
    free(output);
    assert_int_equal(fake.grant_calls, 1);
    assert_string_equal(fake.capability, "tags.read");
    assert_string_equal(fake.scope, "[\"U_A\",\"U_B\"]");
    assert_string_equal(fake.purpose, "say \"hi\"");

    char *uninstall_args[] = {"appctl", "uninstall", "demo", "--disposition", "keep", NULL};
    assert_int_equal(run_appctl(&loop, path, uninstall_args, &output), 0);
    assert_non_null(strstr(output, "\"status\":\"ok\""));
    free(output);
    assert_int_equal(fake.uninstall_calls, 1);
    assert_string_equal(fake.app_id, "demo");
    assert_string_equal(fake.disposition, "keep");

    char *snapshot_args[] = {"appctl", "snapshot", NULL};
    assert_int_equal(run_appctl(&loop, path, snapshot_args, &output), 0);
    assert_non_null(strstr(output, "\"generation\":7"));
    free(output);
    assert_int_equal(fake.snapshot_calls, 1);

    unlink(package_path);
    unlink(key_path);
    rest_uds_server_stop(srv);
    for (int i = 0; i < 20; i++) uv_run(&loop, UV_RUN_NOWAIT);
    assert_int_equal(uv_loop_close(&loop), 0);
    free(hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_uds_management_channel),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
