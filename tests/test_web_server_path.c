#include <cmocka.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
    write_file("/tmp/webroot_outside.txt", "secret");
    return 0;
}

static int teardown(void **state) {
    (void)state;
    char path[PATH_MAX];
    join(path, sizeof(path), "/link");
    unlink(path);
    join(path, sizeof(path), "/assets/app.js");
    unlink(path);
    join(path, sizeof(path), "/assets");
    rmdir(path);
    join(path, sizeof(path), "/index.html");
    unlink(path);
    rmdir(g_root);
    unlink("/tmp/webroot_outside.txt");
    return 0;
}

static void test_resolve_rejects_traversal(void **state) {
    (void)state;
    char expected[PATH_MAX];
    char out[PATH_MAX];

    join(expected, sizeof(expected), "/index.html");
    assert_int_equal(web_server_resolve_path(g_root, "/", out, sizeof(out)), 0);
    assert_string_equal(out, expected);

    join(expected, sizeof(expected), "/assets/app.js");
    assert_int_equal(web_server_resolve_path(g_root, "/assets/app.js", out, sizeof(out)), 0);
    assert_string_equal(out, expected);

    assert_int_equal(web_server_resolve_path(g_root, "/../webroot_outside.txt", out, sizeof(out)),
                     -1);
    assert_int_equal(web_server_resolve_path(g_root, "/assets/../index.html", out, sizeof(out)),
                     -1);

    char link[PATH_MAX];
    join(link, sizeof(link), "/link");
    assert_int_equal(symlink("/tmp/webroot_outside.txt", link), 0);
    assert_int_equal(web_server_resolve_path(g_root, "/link", out, sizeof(out)), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_resolve_rejects_traversal, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
