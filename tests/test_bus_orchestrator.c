#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <uv.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "arbiter.h"

static void test_bus_orchestrator_dynamic_create(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    arbiter_t *arb = NULL;
    
    // 模拟初始化
    datahub_init(&hub, loop, "test_hub", 16);
    arbiter_init(&arb, loop);

    // 测试动态创建：16条总线，池大小32，缓冲区4096
    bus_orchestrator_t *orch = bus_orchestrator_create(&hub, arb, 16, 32, 4096);
    assert_non_null(orch);

    // 验证 attach_fd 在动态范围内有效
    bus_orchestrator_attach_fd(orch, 15, 1); 
    
    bus_orchestrator_destroy(orch);
    arbiter_destroy(arb);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bus_orchestrator_dynamic_create),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
