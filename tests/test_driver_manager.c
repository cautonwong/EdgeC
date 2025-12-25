#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <unistd.h> // For close

#include "driver_manager.h"
#include "shmring.h"

// Dummy global ring
shm_ring_t *global_ring = NULL;

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
    driver_manager_destroy(dm);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_driver_manager_lifecycle),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}