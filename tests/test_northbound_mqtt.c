#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "datahub.h"

static void test_mqtt_push_logic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    // 适配新签名
    datahub_init(&hub, loop, "test_mqtt", 128, 1024, NULL, 0, NULL, NULL, NULL);

    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mqtt_push_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
