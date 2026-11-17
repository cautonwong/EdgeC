/**
 * @file test_oom_injection.c
 * @brief Allocation-failure injection for the app host server create and request paths.
 */

#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

#include "edge_app_host.h"

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *ptr, size_t size);
void *__wrap_malloc(size_t size);
void *__wrap_calloc(size_t count, size_t size);
void *__wrap_realloc(void *ptr, size_t size);

static _Atomic long alloc_seq;
static _Atomic long fail_at = -1;

static int fail_this_alloc(void) {
    long at = atomic_load_explicit(&fail_at, memory_order_relaxed);
    long seq = atomic_fetch_add_explicit(&alloc_seq, 1, memory_order_relaxed);
    return at >= 0 && seq == at;
}

void *__wrap_malloc(size_t size) {
    return fail_this_alloc() ? NULL : __real_malloc(size);
}

void *__wrap_calloc(size_t count, size_t size) {
    return fail_this_alloc() ? NULL : __real_calloc(count, size);
}

void *__wrap_realloc(void *ptr, size_t size) {
    return fail_this_alloc() ? NULL : __real_realloc(ptr, size);
}

static void inject_reset(void) {
    atomic_store_explicit(&alloc_seq, 0, memory_order_relaxed);
    atomic_store_explicit(&fail_at, -1, memory_order_relaxed);
}

static void inject_arm(long index) {
    atomic_store_explicit(&alloc_seq, 0, memory_order_relaxed);
    atomic_store_explicit(&fail_at, index, memory_order_relaxed);
}

static void inject_off(void) {
    atomic_store_explicit(&fail_at, -1, memory_order_relaxed);
}

static long inject_count(void) {
    return atomic_load_explicit(&alloc_seq, memory_order_relaxed);
}

static int stub_resolve(void *ctx, const char *name, uint32_t *handle, uint8_t *access,
                        char *reason, size_t reason_size) {
    (void)ctx;
    (void)reason;
    (void)reason_size;
    if (strcmp(name, "U_A") != 0) return -1;
    *handle = 7;
    *access = APP_HOST_ACCESS_READ;
    return 0;
}

static int stub_read(void *ctx, uint32_t handle, app_host_sample_t *out, char *reason,
                     size_t reason_size) {
    (void)ctx;
    (void)reason;
    (void)reason_size;
    if (handle != 7) return -1;
    memset(out, 0, sizeof(*out));
    out->kind = APP_HOST_VALUE_FLOAT;
    out->number = 220.5;
    out->quality = APP_HOST_QUALITY_GOOD;
    out->source_ts = 1720000000000ULL;
    return 0;
}

static uint64_t stub_now(void *ctx) {
    (void)ctx;
    return 1720000000123ULL;
}

static app_host_ports_t stub_ports(void) {
    app_host_ports_t ports = {0};
    ports.resolve = stub_resolve;
    ports.read = stub_read;
    ports.now_ms = stub_now;
    return ports;
}

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    assert_true(flags >= 0);
    assert_int_equal(fcntl(fd, F_SETFL, flags | O_NONBLOCK), 0);
}

static void close_if_open(int fd) {
    if (fd >= 0 && fcntl(fd, F_GETFD) != -1) close(fd);
}

static int write_frame(int fd, const char *json) {
    size_t len = strlen(json);
    uint8_t frame[512];
    if (len + 4 > sizeof(frame)) return -1;
    frame[0] = (uint8_t)(len >> 24);
    frame[1] = (uint8_t)(len >> 16);
    frame[2] = (uint8_t)(len >> 8);
    frame[3] = (uint8_t)len;
    memcpy(frame + 4, json, len);
    size_t sent = 0;
    while (sent < len + 4) {
        ssize_t n = send(fd, frame + sent, len + 4 - sent, MSG_NOSIGNAL);
        if (n > 0) {
            sent += (size_t)n;
            continue;
        }
        if (n < 0 && errno == EAGAIN) {
            usleep(1000);
            continue;
        }
        return -1;
    }
    return 0;
}

typedef struct {
    uint8_t data[4096];
    size_t len;
} rx_t;

static int rx_take(rx_t *rx, char *out, size_t cap) {
    if (rx->len < 4) return 0;
    uint32_t len = read_be32(rx->data);
    if (rx->len < 4 + (size_t)len) return 0;
    size_t copy = len < cap - 1 ? len : cap - 1;
    memcpy(out, rx->data + 4, copy);
    out[copy] = '\0';
    memmove(rx->data, rx->data + 4 + len, rx->len - 4 - len);
    rx->len -= 4 + (size_t)len;
    return 1;
}

/* Reads one framed response while driving the loop. Returns 0 on timeout or EOF. */
static int wait_frame(uv_loop_t *loop, int fd, rx_t *rx, int timeout_ms, char *out, size_t cap) {
    for (int elapsed = 0; elapsed < timeout_ms; elapsed++) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (rx_take(rx, out, cap)) return 1;
        for (;;) {
            size_t room = sizeof(rx->data) - rx->len;
            if (room == 0) return -1;
            ssize_t n = recv(fd, rx->data + rx->len, room, 0);
            if (n > 0) {
                rx->len += (size_t)n;
                if (rx_take(rx, out, cap)) return 1;
                continue;
            }
            if (n == 0) return 0;
            if (errno == EAGAIN) break;
            return -1;
        }
        usleep(1000);
    }
    return 0;
}

static void drain_loop(uv_loop_t *loop) {
    for (int i = 0; i < 20; i++) uv_run(loop, UV_RUN_NOWAIT);
}

static void mark_closed(void *ctx) {
    *(int *)ctx = 1;
}

static void test_create_path_sweep(void **state) {
    (void)state;
    long create_allocs = 0;

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    app_host_ports_t ports = stub_ports();
    inject_reset();
    app_host_server_t *server = app_host_server_create_ex(&loop, fds[0], &ports, NULL, NULL);
    create_allocs = inject_count();
    assert_non_null(server);
    app_host_server_destroy(server);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
    close(fds[1]);
    assert_true(create_allocs > 0);

    int saw_null = 0;
    int saw_ok = 0;
    for (long index = 0; index <= create_allocs; index++) {
        uv_loop_t probe_loop;
        assert_int_equal(uv_loop_init(&probe_loop), 0);
        int probe_fds[2];
        assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, probe_fds), 0);
        app_host_ports_t probe_ports = stub_ports();

        inject_arm(index);
        app_host_server_t *probe =
            app_host_server_create_ex(&probe_loop, probe_fds[0], &probe_ports, NULL, NULL);
        inject_off();

        if (!probe) {
            saw_null++;
            /* create_ex 取得 fd 所有权，失败路径必须关闭它。 */
            assert_int_equal(fcntl(probe_fds[0], F_GETFD), -1);
        } else {
            set_nonblocking(probe_fds[1]);
            assert_int_equal(
                write_frame(probe_fds[1],
                            "{\"id\":1,\"fn\":\"point_read\",\"args\":{\"handle\":7}}"),
                0);
            rx_t rx = {0};
            char response[1024];
            assert_int_equal(
                wait_frame(&probe_loop, probe_fds[1], &rx, 200, response, sizeof(response)), 1);
            assert_non_null(strstr(response, "\"ok\":true"));
            saw_ok++;
            app_host_server_destroy(probe);
        }
        close_if_open(probe_fds[1]);
        drain_loop(&probe_loop);
        close_if_open(probe_fds[0]);
        assert_int_equal(uv_loop_close(&probe_loop), 0);
    }
    assert_true(saw_null > 0);
    assert_true(saw_ok > 0);
}

static void test_request_path_sweep(void **state) {
    (void)state;
    long request_allocs = 0;

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    set_nonblocking(fds[1]);
    app_host_ports_t ports = stub_ports();
    int closed = 0;
    app_host_server_t *server =
        app_host_server_create_ex(&loop, fds[0], &ports, mark_closed, &closed);
    assert_non_null(server);
    assert_int_equal(
        write_frame(fds[1], "{\"id\":1,\"fn\":\"point_read\",\"args\":{\"handle\":7}}"), 0);
    inject_reset();
    rx_t rx = {0};
    char response[1024];
    assert_int_equal(wait_frame(&loop, fds[1], &rx, 200, response, sizeof(response)), 1);
    request_allocs = inject_count();
    assert_int_equal(closed, 0);
    app_host_server_destroy(server);
    close(fds[1]);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
    assert_true(request_allocs > 0);

    for (long index = 0; index < request_allocs; index++) {
        uv_loop_t probe_loop;
        assert_int_equal(uv_loop_init(&probe_loop), 0);
        int probe_fds[2];
        assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, probe_fds), 0);
        set_nonblocking(probe_fds[1]);
        app_host_ports_t probe_ports = stub_ports();
        int probe_closed = 0;
        app_host_server_t *probe = app_host_server_create_ex(
            &probe_loop, probe_fds[0], &probe_ports, mark_closed, &probe_closed);
        assert_non_null(probe);
        assert_int_equal(
            write_frame(probe_fds[1], "{\"id\":1,\"fn\":\"point_read\",\"args\":{\"handle\":7}}"),
            0);

        inject_arm(index);
        rx_t probe_rx = {0};
        char probe_response[1024];
        (void)wait_frame(&probe_loop, probe_fds[1], &probe_rx, 200, probe_response,
                         sizeof(probe_response));
        inject_off();
        drain_loop(&probe_loop);

        if (!probe_closed) {
            assert_int_equal(write_frame(probe_fds[1], "{\"id\":2,\"fn\":\"now\"}"), 0);
            int got = wait_frame(&probe_loop, probe_fds[1], &probe_rx, 200, probe_response,
                                 sizeof(probe_response));
            assert_int_equal(got, 1);
            assert_non_null(strstr(probe_response, "\"ok\":true"));
            app_host_server_destroy(probe);
        }
        close_if_open(probe_fds[1]);
        drain_loop(&probe_loop);
        assert_int_equal(uv_loop_close(&probe_loop), 0);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_create_path_sweep),
        cmocka_unit_test(test_request_path_sweep),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
