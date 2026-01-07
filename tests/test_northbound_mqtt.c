#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_service.h"
#include "northbound_mgr.h"

static void test_mqtt_push_logic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_mqtt", 128);

    arbiter_t *arb = NULL;
    arbiter_init(&arb, loop);

    // 修正：注入所有核心组件
    service_t *svc = service_new(loop, &hub, arb, NULL, NULL, NULL, NULL);
    assert_non_null(svc);

    service_delete(svc);
    arbiter_destroy(arb);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mqtt_push_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}