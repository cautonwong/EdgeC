/**
 * test_web_event_stream.c - Verification of Global SOE Event Stream & Alarm Waterfall Hub (AEP-497)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcu_soe_event_engine.h"

static void read_file_content(const char *rel_path, char *buf, size_t max_len) {
    char path[512];
    FILE *fp = NULL;

    const char *prefixes[] = {"", "../", "../../", "/workspaces/edgeos/EdgeC/", NULL};

    for (int i = 0; prefixes[i] != NULL; i++) {
        snprintf(path, sizeof(path), "%s%s", prefixes[i], rel_path);
        fp = fopen(path, "r");
        if (fp) break;
    }

    assert_non_null(fp);
    size_t n = fread(buf, 1, max_len - 1, fp);
    buf[n] = '\0';
    fclose(fp);
}

static void test_event_stream_hub_component_structure(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/events/EventStreamHub.vue", content, sizeof(content));

    // Headers & High-Frequency Stream Metrics
    assert_non_null(strstr(content, "SOE EVENT STREAM"));
    assert_non_null(strstr(content, "streamRateEps"));
    assert_non_null(strstr(content, "unackedCount"));
    assert_non_null(strstr(content, "toggleStream"));

    // Multi-domain and severity filters
    assert_non_null(strstr(content, "filterDomain"));
    assert_non_null(strstr(content, "filterSeverity"));
    assert_non_null(strstr(content, "filterAck"));
    assert_non_null(strstr(content, "searchQuery"));
    assert_non_null(strstr(content, "IEMS_EMS"));
    assert_non_null(strstr(content, "SMARTBOX_SAFETY"));
    assert_non_null(strstr(content, "DCU_CHANNEL"));
    assert_non_null(strstr(content, "DDS_TOPOLOGY"));

    // Four severity states
    assert_non_null(strstr(content, "CRITICAL"));
    assert_non_null(strstr(content, "WARNING"));
    assert_non_null(strstr(content, "INFO"));
    assert_non_null(strstr(content, "ACTION"));

    // Batch ACK & Export
    assert_non_null(strstr(content, "batchAcknowledgeAll"));
    assert_non_null(strstr(content, "exportEventsCSV"));
    assert_non_null(strstr(content, "exportEventsJSON"));
    assert_non_null(strstr(content, "selectedEvent"));
}

static void test_event_stream_hub_routing_and_exports(void **state) {
    (void)state;
    char router_content[8192];
    read_file_content("web/core/src/router/index.ts", router_content, sizeof(router_content));
    assert_non_null(strstr(router_content, "events"));
    assert_non_null(strstr(router_content, "EventStreamHub.vue"));

    char index_content[8192];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));
    assert_non_null(strstr(index_content, "EventStreamHub"));
}

static void test_dcu_soe_c_engine_event_recording_and_active_alarms(void **state) {
    (void)state;
    dcu_soe_event_engine_t engine;
    dcu_soe_init(&engine);
    assert_int_equal(engine.total_events_logged, 0);
    assert_int_equal(dcu_soe_get_active_alarms(&engine), 0);

    // Record Outage Last-Gasp event (0x3011)
    int rc = dcu_soe_record_event(&engine, OAD_EVT_OUTAGE_LASTGASP, "000000000001", 101,
                                  EVT_STATE_TRIGGERED, 0.0, 1725525612314ULL,
                                  "Power Outage Last-Gasp Alarm");
    assert_int_equal(rc, 0);
    assert_int_equal(engine.total_events_logged, 1);
    assert_int_equal(dcu_soe_get_active_alarms(&engine), 1);

    // Record Unbalance event (0x3005)
    rc = dcu_soe_record_event(&engine, OAD_EVT_UNBALANCE, "000000000002", 102, EVT_STATE_TRIGGERED,
                              38.5, 1725525612400ULL, "3-Phase Current Unbalance Exceeded 15%");
    assert_int_equal(rc, 0);
    assert_int_equal(engine.total_events_logged, 2);
    assert_int_equal(dcu_soe_get_active_alarms(&engine), 2);
}

static void test_dcu_soe_event_state_machine_transitions(void **state) {
    (void)state;
    dcu_soe_event_engine_t engine;
    dcu_soe_init(&engine);

    // Trigger Overtemperature
    dcu_soe_record_event(&engine, OAD_EVT_TEMP_EXCEEDED, "000000000003", 103, EVT_STATE_TRIGGERED,
                         96.2, 1725525615000ULL, "Transformer Winding Overheating");
    assert_int_equal(dcu_soe_get_active_alarms(&engine), 1);

    // Restore Overtemperature
    dcu_soe_record_event(&engine, OAD_EVT_TEMP_EXCEEDED, "000000000003", 103, EVT_STATE_RESTORED,
                         84.0, 1725525619000ULL, "Transformer Winding Temperature Normal");
    assert_int_equal(dcu_soe_get_active_alarms(&engine), 0);
    assert_int_equal(engine.total_events_logged, 2);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_event_stream_hub_component_structure),
        cmocka_unit_test(test_event_stream_hub_routing_and_exports),
        cmocka_unit_test(test_dcu_soe_c_engine_event_recording_and_active_alarms),
        cmocka_unit_test(test_dcu_soe_event_state_machine_transitions),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
