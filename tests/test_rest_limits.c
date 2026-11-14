#include <arpa/inet.h>
#include <cmocka.h>
#include <errno.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "datahub_priv.h"
#include "edge_service.h"
#include "rest_server.h"

typedef struct {
    uv_loop_t loop;
    service_t *svc;
    datahub_t *hub;
    rest_server_storage_t *storage;
    rest_server_t *srv;
    int port;
} rest_fixture_t;

static void fixture_start(rest_fixture_t *f, const rest_limits_t *limits) {
    memset(f, 0, sizeof(*f));
    assert_int_equal(uv_loop_init(&f->loop), 0);
    f->hub = calloc(1, sizeof(datahub_t));
    assert_non_null(f->hub);
    f->svc = service_new(&f->loop, f->hub, (arbiter_t *)0x1234, NULL, NULL);
    assert_non_null(f->svc);
    f->storage = malloc(sizeof(rest_server_storage_t));
    assert_non_null(f->storage);
    f->srv = rest_server_create_in_ex(f->storage, &f->loop, f->svc, 0, limits);
    assert_non_null(f->srv);
    char ip[64] = {0};
    assert_int_equal(rest_server_get_bound_addr(f->srv, ip, sizeof(ip), &f->port), 0);
    assert_true(f->port > 0);
}

static void fixture_stop(rest_fixture_t *f) {
    if (f->srv) rest_server_destroy(f->srv);
    if (f->svc) service_delete(f->svc);
    f->svc = NULL;
    int rc = uv_loop_close(&f->loop);
    for (int i = 0; i < 100000 && rc == UV_EBUSY; i++) {
        uv_run(&f->loop, UV_RUN_NOWAIT);
        rc = uv_loop_close(&f->loop);
    }
    free(f->hub);
    free(f->storage);
    f->hub = NULL;
    f->storage = NULL;
    f->srv = NULL;
}

static int connect_client(rest_fixture_t *f) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(fd >= 0);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)f->port);
    assert_int_equal(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr), 1);
    assert_int_equal(connect(fd, (struct sockaddr *)&addr, sizeof(addr)), 0);
    return fd;
}

static void send_all(int fd, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);
        assert_true(n > 0);
        sent += (size_t)n;
    }
}

static size_t read_response(rest_fixture_t *f, int fd, char *buf, size_t cap, int timeout_ms) {
    size_t total = 0;
    int idle_ms = 0;
    buf[0] = '\0';
    while (total + 1 < cap && idle_ms < timeout_ms) {
        uv_run(&f->loop, UV_RUN_NOWAIT);
        ssize_t n = recv(fd, buf + total, cap - 1 - total, MSG_DONTWAIT);
        if (n > 0) {
            total += (size_t)n;
            idle_ms = 0;
            continue;
        }
        if (n == 0) break;
        assert_true(errno == EAGAIN || errno == EWOULDBLOCK);
        usleep(2000);
        idle_ms += 2;
    }
    buf[total] = '\0';
    return total;
}

static void pump_loop(rest_fixture_t *f, int ms) {
    for (int elapsed = 0; elapsed < ms; elapsed += 2) {
        uv_run(&f->loop, UV_RUN_NOWAIT);
        usleep(2000);
    }
}

static void expect_status(rest_fixture_t *f, const char *request, const char *status_line) {
    int fd = connect_client(f);
    send_all(fd, request, strlen(request));
    char resp[8192];
    size_t n = read_response(f, fd, resp, sizeof(resp), 2000);
    assert_true(n > 0);
    assert_non_null(strstr(resp, status_line));
    close(fd);
}

static void test_default_bind_is_loopback(void **state) {
    (void)state;
    rest_fixture_t f;
    fixture_start(&f, NULL);

    char ip[64] = {0};
    int port = 0;
    assert_int_equal(rest_server_get_bound_addr(f.srv, ip, sizeof(ip), &port), 0);
    assert_string_equal(ip, "127.0.0.1");
    assert_true(port > 0);

    int fd = connect_client(&f);
    const char *request = "GET /api/v1/devices HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send_all(fd, request, strlen(request));
    char resp[8192];
    size_t n = read_response(&f, fd, resp, sizeof(resp), 2000);
    assert_true(n > 0);
    assert_non_null(strstr(resp, "HTTP/1.1 200 OK"));
    assert_non_null(strstr(resp, "\"meter_01\""));
    assert_null(strstr(resp, "Access-Control-Allow-Origin"));
    close(fd);

    fixture_stop(&f);
}

static void test_split_request_accumulates(void **state) {
    (void)state;
    rest_fixture_t f;
    fixture_start(&f, NULL);

    int fd = connect_client(&f);
    const char *part1 = "POST /api/v1/apps/start HTTP/1.1\r\nHost: localhost\r\n";
    const char *part2 = "Content-Length: 17\r\n\r\n{\"app_id\"";
    const char *part3 = ":\"demo\"}";
    send_all(fd, part1, strlen(part1));
    pump_loop(&f, 20);
    send_all(fd, part2, strlen(part2));
    pump_loop(&f, 20);
    send_all(fd, part3, strlen(part3));

    char resp[8192];
    assert_true(read_response(&f, fd, resp, sizeof(resp), 2000) > 0);
    assert_non_null(strstr(resp, "HTTP/1.1 200 OK"));
    // The handler only answers start_failed when the whole body parsed as JSON with app_id.
    assert_non_null(strstr(resp, "start_failed"));
    close(fd);

    fixture_stop(&f);
}

static void test_oversized_header_431(void **state) {
    (void)state;
    rest_limits_t limits;
    rest_limits_default(&limits);
    limits.max_header_bytes = 512;
    rest_fixture_t f;
    fixture_start(&f, &limits);

    char request[1024];
    int n = snprintf(request, sizeof(request), "GET /api/v1/devices HTTP/1.1\r\nX-Fill: ");
    memset(request + n, 'a', 600);
    n += 600;
    memcpy(request + n, "\r\n\r\n", 4);
    n += 4;

    int fd = connect_client(&f);
    send_all(fd, request, (size_t)n);
    char resp[8192];
    assert_true(read_response(&f, fd, resp, sizeof(resp), 2000) > 0);
    assert_non_null(strstr(resp, "HTTP/1.1 431"));
    close(fd);

    fixture_stop(&f);
}

static void test_oversized_body_413(void **state) {
    (void)state;
    rest_limits_t limits;
    rest_limits_default(&limits);
    limits.max_body_bytes = 1024;
    rest_fixture_t f;
    fixture_start(&f, &limits);

    expect_status(&f,
                  "POST /api/v1/system/health HTTP/1.1\r\n"
                  "Host: localhost\r\n"
                  "Content-Length: 1025\r\n\r\n",
                  "HTTP/1.1 413");

    fixture_stop(&f);
}

static void test_slow_request_408(void **state) {
    (void)state;
    rest_limits_t limits;
    rest_limits_default(&limits);
    limits.read_timeout_ms = 100;
    rest_fixture_t f;
    fixture_start(&f, &limits);

    int fd = connect_client(&f);
    const char *partial = "GET /api/v1/devices HTTP/1.1\r\nHost: localhost\r\n";
    send_all(fd, partial, strlen(partial));
    char resp[8192];
    assert_true(read_response(&f, fd, resp, sizeof(resp), 1500) > 0);
    assert_non_null(strstr(resp, "HTTP/1.1 408"));
    close(fd);

    fixture_stop(&f);
}

static void test_invalid_request_400(void **state) {
    (void)state;
    rest_fixture_t f;
    fixture_start(&f, NULL);

    expect_status(&f, "GARBAGE\r\n\r\n", "HTTP/1.1 400");
    expect_status(&f,
                  "POST /api/v1/devices HTTP/1.1\r\nContent-Length: abc\r\n\r\n",
                  "HTTP/1.1 400");

    fixture_stop(&f);
}

static void test_method_not_allowed_405(void **state) {
    (void)state;
    rest_fixture_t f;
    fixture_start(&f, NULL);

    expect_status(&f, "PUT /api/v1/devices HTTP/1.1\r\nHost: localhost\r\n\r\n",
                  "HTTP/1.1 405");

    fixture_stop(&f);
}

static void test_legacy_upload_closed_410(void **state) {
    (void)state;
    rest_fixture_t f;
    fixture_start(&f, NULL);

    const char *body = "{\"name\":\"legacy\",\"payload_hex\":\"00\"}";
    char request[512];
    int n = snprintf(request, sizeof(request),
                     "POST /api/v1/apps/upload HTTP/1.1\r\n"
                     "Host: localhost\r\n"
                     "Content-Length: %zu\r\n\r\n%s",
                     strlen(body), body);
    assert_true(n > 0 && (size_t)n < sizeof(request));

    int fd = connect_client(&f);
    send_all(fd, request, (size_t)n);
    char resp[8192];
    assert_true(read_response(&f, fd, resp, sizeof(resp), 2000) > 0);
    assert_non_null(strstr(resp, "HTTP/1.1 410"));
    assert_non_null(strstr(resp, "/api/v1/apps/package"));
    close(fd);

    fixture_stop(&f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_default_bind_is_loopback),
        cmocka_unit_test(test_split_request_accumulates),
        cmocka_unit_test(test_oversized_header_431),
        cmocka_unit_test(test_oversized_body_413),
        cmocka_unit_test(test_slow_request_408),
        cmocka_unit_test(test_invalid_request_400),
        cmocka_unit_test(test_method_not_allowed_405),
        cmocka_unit_test(test_legacy_upload_closed_410),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
