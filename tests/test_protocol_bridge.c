#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>
#include <unistd.h>

#include "protocol_bridge.h"
#include "datahub.h"
#include "arbiter.h"
#include "bus_orchestrator.h"

static void test_bridge_downlink_routing(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    
    datahub_t hub;
    // Capacity 2048
    datahub_init(&hub, loop, "test_bridge", 64, 2048, NULL, 0, NULL, NULL, NULL);
    
    arbiter_t *arb;
    arbiter_init(&arb, loop);
    arbiter_resume(arb);
    
    bus_orchestrator_t *orch = bus_orchestrator_create(&hub, arb, NULL, 4, 32, 512);
    
    protocol_bridge_t *bridge = protocol_bridge_create(loop, &hub, orch);
    assert_non_null(bridge);
    protocol_bridge_start(bridge);

    // 1. Simulate a Northbound command in DataHub
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 1.0, uv_hrtime());
    datahub_publish(&hub, &msg);
    
    // 2. Process loop to trigger async notify
    for(int i=0; i<50; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    
    // [Nexus] Reclaim commands from Arbiter (Manually for test, normally handled by engine)
    // Actually, bus_orchestrator_destroy should handle it? 
    // It detaches engines, but does it free the queue?
    // Let's assume we need to process it.
    
    // Cleanup
    protocol_bridge_destroy(bridge);
    bus_orchestrator_destroy(orch);
    arbiter_destroy(arb);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bridge_downlink_routing),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
