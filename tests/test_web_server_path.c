#include <arpa/inet.h>
#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <uv.h>

#include "web_server.h"

/* mkdtemp replaces the six Xs in place, so the template length is the maximum. */
static char g_root[sizeof("/tmp/webrootXXXXXX")];

static void join(char *out, size_t size, const char *relative) {
    snprintf(out, size, "%s%s", g_root, relative);
}

static void write_file(const char *path, const char *text) {
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    assert_true(fd >= 0);
    assert_int_equal(write(fd, text, strlen(text)), (ssize_t)strlen(text));
    close(fd);
}

static ssize_t read_all(int fd, char *buf, size_t cap) {
    size_t used = 0;
    while (used + 1 < cap) {
        ssize_t n = read(fd, buf + used, cap - 1 - used);
        if (n < 0) return -1;
        if (n == 0) break;
        used += (size_t)n;
    }
    buf[used] = '\0';
    return (ssize_t)used;
}

static void assert_serves(const char *request_path, const char *expected) {
    int fd = web_server_open_path(g_root, request_path);
    assert_true(fd >= 0);
    char buf[64];
    assert_int_equal(read_all(fd, buf, sizeof(buf)), (ssize_t)strlen(expected));
    assert_string_equal(buf, expected);
    close(fd);
}

static void assert_rejected(const char *request_path) {
    assert_int_equal(web_server_open_path(g_root, request_path), -1);
}

static int setup(void **state) {
    (void)state;
    snprintf(g_root, sizeof(g_root), "/tmp/webrootXXXXXX");
    assert_non_null(mkdtemp(g_root));

    char path[PATH_MAX];
    join(path, sizeof(path), "/assets");
    assert_int_equal(mkdir(path, 0755), 0);
    join(path, sizeof(path), "/index.html");
    write_file(path, "index");
    join(path, sizeof(path), "/assets/app.js");
    write_file(path, "ok");
    join(path, sizeof(path), "/fifo");
    assert_int_equal(mkfifo(path, 0644), 0);
    write_file("/tmp/webroot_outside.txt", "secret");

    join(path, sizeof(path), "/link");
    assert_int_equal(symlink("/tmp/webroot_outside.txt", path), 0);
    join(path, sizeof(path), "/inside_link");
    assert_int_equal(symlink("assets", path), 0);
    join(path, sizeof(path), "/dirlink");
    assert_int_equal(symlink("/tmp", path), 0);
    join(path, sizeof(path), "/assets_link");
    assert_int_equal(symlink("assets", path), 0);
    return 0;
}

static int teardown(void **state) {
    (void)state;
    char path[PATH_MAX];
    const char *links[] = {"/link", "/inside_link", "/dirlink", "/assets_link"};
    for (size_t i = 0; i < sizeof(links) / sizeof(links[0]); i++) {
        join(path, sizeof(path), links[i]);
        unlink(path);
    }
    join(path, sizeof(path), "/assets/app.js");
    unlink(path);
    join(path, sizeof(path), "/fifo");
    unlink(path);
    join(path, sizeof(path), "/assets");
    rmdir(path);
    join(path, sizeof(path), "/index.html");
    unlink(path);
    rmdir(g_root);
    unlink("/tmp/webroot_outside.txt");
    return 0;
}

static void test_serves_regular_files(void **state) {
    (void)state;
    assert_serves("/", "index");
    assert_serves("/index.html", "index");
    assert_serves("/assets/app.js", "ok");
}

static void test_rejects_traversal_and_bad_shapes(void **state) {
    (void)state;
    assert_rejected("/../webroot_outside.txt");
    assert_rejected("/assets/../index.html");
    assert_rejected("/.");
    assert_rejected("/./index.html");
    assert_rejected("//index.html");
    assert_rejected("/assets//app.js");
    assert_rejected("/assets/");
    assert_rejected("");
    assert_rejected("index.html");
    assert_rejected("http://127.0.0.1/etc/passwd");
}

static void test_rejects_every_symlink_component(void **state) {
    (void)state;
    assert_rejected("/link");
    assert_rejected("/inside_link");
    assert_rejected("/inside_link/app.js");
    assert_rejected("/dirlink/webroot_outside.txt");
    assert_rejected("/assets_link");
    assert_rejected("/assets_link/app.js");
}

static void test_rejects_fifo_without_blocking(void **state) {
    (void)state;
    assert_rejected("/fifo");
}

static int connect_client(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(fd >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    assert_int_equal(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr), 1);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    assert_int_equal(connect(fd, (struct sockaddr *)&addr, sizeof(addr)), 0);
    return fd;
}

static int pump_response(uv_loop_t *loop, int fd, char *out, size_t cap) {
    size_t used = 0;
    out[0] = '\0';
    for (int i = 0; i < 2000; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        ssize_t n = recv(fd, out + used, cap - 1 - used, 0);
        if (n > 0) {
            used += (size_t)n;
            out[used] = '\0';
            if (strstr(out, "\r\n\r\n")) return 1;
            continue;
        }
        if (n == 0) break;
        /* A signal is not an answer: retry, the loop gets another turn. */
        if (errno != EAGAIN && errno != EINTR) break;
        usleep(1000);
    }
    return used > 0;
}

static void fetch(uv_loop_t *loop, int port, const char *request, char *out, size_t cap) {
    int fd = connect_client(port);
    assert_int_equal(send(fd, request, strlen(request), 0), (ssize_t)strlen(request));
    assert_true(pump_response(loop, fd, out, cap));
    close(fd);
}

static void drain_loop(uv_loop_t *loop) {
    for (int i = 0; i < 100; i++) uv_run(loop, UV_RUN_NOWAIT);
}

static void test_http_serves_files_and_rejects_symlinks(void **state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    web_server_t *srv = web_server_create(&loop, 0, g_root);
    assert_non_null(srv);
    int port = web_server_port(srv);
    assert_true(port > 0);

    char out[2048];
    fetch(&loop, port, "GET /assets/app.js HTTP/1.1\r\nHost: x\r\n\r\n", out, sizeof(out));
    assert_true(strncmp(out, "HTTP/1.1 200", 12) == 0);
    assert_non_null(strstr(out, "Content-Type: application/javascript"));
    assert_non_null(strstr(out, "Content-Length: 2"));

    fetch(&loop, port, "GET /link HTTP/1.1\r\nHost: x\r\n\r\n", out, sizeof(out));
    assert_true(strncmp(out, "HTTP/1.1 404", 12) == 0);

    fetch(&loop, port, "GET /dirlink/webroot_outside.txt HTTP/1.1\r\nHost: x\r\n\r\n", out,
          sizeof(out));
    assert_true(strncmp(out, "HTTP/1.1 404", 12) == 0);

    fetch(&loop, port, "GET /fifo HTTP/1.1\r\nHost: x\r\n\r\n", out, sizeof(out));
    assert_true(strncmp(out, "HTTP/1.1 404", 12) == 0);

    web_server_destroy(srv);
    drain_loop(&loop);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_serves_regular_files, setup, teardown),
        cmocka_unit_test_setup_teardown(test_rejects_traversal_and_bad_shapes, setup, teardown),
        cmocka_unit_test_setup_teardown(test_rejects_every_symlink_component, setup, teardown),
        cmocka_unit_test_setup_teardown(test_rejects_fifo_without_blocking, setup, teardown),
        cmocka_unit_test_setup_teardown(test_http_serves_files_and_rejects_symlinks, setup,
                                        teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
