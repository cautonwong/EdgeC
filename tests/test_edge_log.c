#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

static void test_log_basic_lifecycle(void** state) {
    (void)state;
    edge_log_t* logger = edge_log_create(NULL, true);
    assert_non_null(logger);

    edge_log_out(logger, LOG_CAT_CORE, 0, "Test Log Message");

    edge_log_destroy(logger);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_log_basic_lifecycle),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
