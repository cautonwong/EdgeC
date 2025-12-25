#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <unistd.h> // For close

#include "driver_manager.h"

static void test_driver_manager_lifecycle(void **state) {
    (void)state; // Unused

    // 1. Create the driver manager
    driver_manager_t* dm = driver_manager_create();
    assert_non_null(dm);

    // 2. Get FD for a configured bus
    int fd1 = driver_manager_get_fd(dm, BUS_RS485_1);
    // The implementation opens /dev/null, which should succeed and return a valid FD
    assert_true(fd1 >= 0);

    // 3. Get FD for another configured bus
    int fd2 = driver_manager_get_fd(dm, BUS_RS485_2);
    assert_true(fd2 >= 0);
    assert_true(fd1 != fd2); // Ensure they are different descriptors

    // 4. Get FD for an unconfigured bus (should return -1)
    int fd_unconfigured = driver_manager_get_fd(dm, BUS_CAN);
    assert_int_equal(fd_unconfigured, -1);

    // 5. Get FD for an invalid bus
    int fd_invalid = driver_manager_get_fd(dm, BUS_MAX);
    assert_int_equal(fd_invalid, -1);

    // 6. Destroy the driver manager
    // We can't easily check if fds are closed without complex OS-level checks,
    // but we can ensure the function call doesn't crash.
    // The valgrind/ASan will detect if we leak file descriptors on exit.
    driver_manager_destroy(dm);

    // After destroy, attempting to use the fd should fail if it was closed,
    // but the test process might get a new fd with the same number.
    // So we don't assert on post-close behavior.
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_driver_manager_lifecycle),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
