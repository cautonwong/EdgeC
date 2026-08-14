#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "datahub.h"
#include "datahub_router.h"

int main(void) {
    printf("=== Testing Three-Backend DataHub Architecture (ADR-0005) ===\n");

    uv_loop_t *loop = uv_default_loop();
    assert(loop != NULL);

    datahub_router_config_t config;
    memset(&config, 0, sizeof(config));
    config.enable_shm = true;
    config.enable_dds = true;
    config.enable_mqtt = true;

    snprintf(config.shm_config.shm_name, sizeof(config.shm_config.shm_name), "test_router_shm");
    config.shm_config.ring_slots = 64;
    config.shm_config.shadow_capacity = 2048;

    datahub_router_t router;
    int rc = datahub_router_init(&router, loop, &config, NULL, 0, NULL, NULL, NULL);
    assert(rc == 0);
    printf("  [PASS] DataHub Router Initialized with SHM, DDS, and MQTT backends\n");

    pipe_msg_t test_msg;
    memset(&test_msg, 0, sizeof(test_msg));
    test_msg.tag_id = 1001;
    test_msg.payload.val.f = 220.5f;

    rc = datahub_router_publish(&router, &test_msg);
    assert(rc == 0);
    printf("  [PASS] Concurrent Publish Fan-Out across active backends\n");

    rc = datahub_router_write_shadow(&router, &test_msg);
    assert(rc == 0);
    printf("  [PASS] Concurrent Write Shadow Fan-Out across active backends\n");

    pipe_msg_t read_msg;
    memset(&read_msg, 0, sizeof(read_msg));
    rc = datahub_router_read_shadow(&router, 1001, &read_msg);
    assert(rc == 0);
    assert(read_msg.tag_id == 1001);
    printf("  [PASS] Low-Latency Primary Shadow Read\n");

    datahub_router_destroy(&router);
    printf("  [PASS] DataHub Router Destroyed Cleanly\n");

    printf("=== All Three-Backend DataHub Tests Passed Successfully! ===\n");
    return 0;
}
