#include <cmocka.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "no_exec_filter.h"

static void test_exec_denied(void **state) {
    (void)state;
    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        if (app_host_install_no_exec_filter() != 0) _exit(77);
        if (getpid() <= 0) _exit(1);
        execl("/bin/true", "true", (char *)NULL);
        _exit(errno == EPERM ? 0 : 1);
    }

    int status = 0;
    assert_int_equal(waitpid(pid, &status, 0), pid);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 77) {
        skip();
        return;
    }
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_exec_denied),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
