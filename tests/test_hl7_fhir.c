#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"

extern int hl7_fhir_driver_register(driver_manager_t* mgr);

static void test_fhir_json_export(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    datahub_t hub;
    datahub_init(&hub, loop, "test_fhir", 64, 10000, NULL, 0, NULL, NULL, NULL);

    driver_manager_t* mgr = driver_manager_create();
    hl7_fhir_driver_register(mgr);

    // Node ID 42 (Patient Monitor)
    edge_driver_ctx_t* ctx = driver_instance_create(mgr, "HL7_FHIR", 42);
    assert_non_null(ctx);
    ctx->user_data = &hub;
    ctx->pool = net_buf_pool_new(10, 2048, 0);

    // 1. Simulate an Anomaly Alarm in DataHub (Tag 9999, Event Type)
    pipe_msg_t msg;
    pipe_msg_init_event(&msg, 0xE002, 3500, uv_hrtime());  // Energy = 3500
    msg.tag_id = 9999;
    datahub_publish(&hub, &msg);

    // 2. Trigger driver tick to check if it formulates the FHIR payload
    net_buf_t* out_tx = NULL;
    int rc = ctx->driver->on_tick(ctx, &out_tx);

    assert_int_equal(rc, 0);
    assert_non_null(out_tx);

    // 3. Verify JSON contents
    char* json = (char*)out_tx->b.data;
    printf("Generated FHIR Payload:\n%s\n", json);

    assert_non_null(strstr(json, "RiskAssessment"));
    assert_non_null(strstr(json, "Device/42"));
    assert_non_null(strstr(json, "ECG Morphological Anomaly"));
    assert_non_null(strstr(json, "Calculated Energy: 3500"));

    // Cleanup
    if (out_tx) net_buf_unref(out_tx);
    net_buf_pool_unref(ctx->pool);
    driver_instance_destroy(ctx, "HL7_FHIR");
    driver_manager_destroy(mgr);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fhir_json_export),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
