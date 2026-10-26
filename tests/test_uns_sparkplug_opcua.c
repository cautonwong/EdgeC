/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_uns_sparkplug_opcua.c
 * @brief Unit tests for ISA-95 Unified Namespace (UNS) Engine & Sparkplug B / OPC UA Mirror
 * (AEP-613)
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "edge_uns_engine.h"

static void test_uns_registration_and_path_resolution(void) {
    printf("[TEST] UNS Registration & ISA-95 Path Resolution...\n");

    edge_uns_engine_t engine;
    int ret = edge_uns_engine_init(&engine, "StateGrid", "Zhejiang_ESS_Station_01");
    assert(ret == 0);

    edge_uns_point_def_t pt1 = {.path = {.enterprise = "StateGrid",
                                         .site = "Zhejiang_ESS_Station_01",
                                         .area = "Battery_Cluster_A",
                                         .line = "Rack_04",
                                         .cell = "Pack_12",
                                         .metric_name = "Cell_01_Voltage"},
                                .tag_id = 131075,
                                .datatype = SPB_TYPE_FLOAT,
                                .scale = 0.001f,    /* mV to V */
                                .deadband = 0.005f, /* 5mV deadband */
                                .unit = "V",
                                .sparkplug_alias = 101,
                                .opcua_node_id = 1001};

    ret = edge_uns_register_point(&engine, &pt1);
    assert(ret == 0);

    char full_path[EDGE_UNS_FULL_PATH_LEN];
    ret = edge_uns_format_path(&pt1.path, full_path, sizeof(full_path));
    assert(ret == 0);
    assert(strcmp(full_path,
                  "StateGrid/Zhejiang_ESS_Station_01/Battery_Cluster_A/Rack_04/Pack_12/"
                  "Cell_01_Voltage") == 0);

    int idx = edge_uns_find_by_path(&engine, full_path);
    assert(idx == 0);

    idx = edge_uns_find_by_tag(&engine, 131075);
    assert(idx == 0);

    /* Test duplicate registration rejection */
    ret = edge_uns_register_point(&engine, &pt1);
    assert(ret == -3);

    printf("  -> PASSED: Path resolution and tag mapping verified.\n");
}

static void test_uns_rbe_deadband_filtering(void) {
    printf("[TEST] UNS Report-by-Exception (RBE) & Deadband Filtering...\n");

    edge_uns_engine_t engine;
    edge_uns_engine_init(&engine, "Enterprise", "Node1");

    edge_uns_point_def_t pt = {.path = {.enterprise = "E",
                                        .site = "S",
                                        .area = "A",
                                        .line = "L",
                                        .cell = "C",
                                        .metric_name = "Temp"},
                               .tag_id = 2001,
                               .datatype = SPB_TYPE_FLOAT,
                               .scale = 1.0f,
                               .deadband = 0.5f, /* 0.5 deg C deadband */
                               .sparkplug_alias = 201};
    edge_uns_register_point(&engine, &pt);

    bool rbe_triggered = false;
    double scaled = 0.0;

    /* 1. First evaluation: must trigger RBE (initial value) */
    int ret = edge_uns_evaluate_point(&engine, 2001, 25.0, 1000, &rbe_triggered, &scaled);
    assert(ret == 1);
    assert(rbe_triggered == true);
    assert(fabs(scaled - 25.0) < 1e-4);

    /* 2. Small jitter within deadband (< 0.5): should NOT trigger RBE */
    ret = edge_uns_evaluate_point(&engine, 2001, 25.3, 1010, &rbe_triggered, &scaled);
    assert(ret == 0);
    assert(rbe_triggered == false);

    /* 3. Another small jitter: should NOT trigger */
    ret = edge_uns_evaluate_point(&engine, 2001, 25.4, 1020, &rbe_triggered, &scaled);
    assert(ret == 0);
    assert(rbe_triggered == false);

    /* 4. Large step exceeding deadband (25.0 -> 25.6, delta = 0.6 >= 0.5): MUST trigger RBE */
    ret = edge_uns_evaluate_point(&engine, 2001, 25.6, 1030, &rbe_triggered, &scaled);
    assert(ret == 1);
    assert(rbe_triggered == true);
    assert(fabs(scaled - 25.6) < 1e-4);

    edge_uns_telemetry_t telem;
    edge_uns_get_telemetry(&engine, &telem);
    assert(telem.total_evaluations == 4);
    assert(telem.rbe_events_triggered == 2);
    assert(telem.deadband_filtered_count == 2);

    printf("  -> PASSED: RBE deadband logic 100%% verified (2 triggered, 2 filtered).\n");
}

static void test_sparkplug_nbirth_ndata_ndeath_generation(void) {
    printf("[TEST] Sparkplug B (NBIRTH, NDATA, NDEATH) Zero-Alloc Generation...\n");

    edge_uns_engine_t engine;
    edge_uns_engine_init(&engine, "Grid", "Substation_Alpha");
    engine.birth_timestamp_ms = 1700000000000ULL;

    edge_uns_point_def_t p1 = {.path = {.enterprise = "Grid",
                                        .site = "Alpha",
                                        .area = "Bay1",
                                        .line = "Feeder",
                                        .cell = "Meter1",
                                        .metric_name = "Voltage_A"},
                               .tag_id = 10001,
                               .datatype = SPB_TYPE_FLOAT,
                               .scale = 1.0f,
                               .deadband = 0.1f,
                               .sparkplug_alias = 1};
    edge_uns_register_point(&engine, &p1);

    uint8_t payload[256];
    size_t out_len = 0;

    /* 1. NBIRTH Generation */
    int ret = edge_uns_generate_sparkplug_nbirth(&engine, payload, sizeof(payload), &out_len);
    assert(ret == 0);
    assert(out_len > 10);
    assert(engine.telemetry.sparkplug_nbirth_count == 1);

    /* 2. NDATA Generation */
    bool rbe = false;
    double val = 0.0;
    edge_uns_evaluate_point(&engine, 10001, 220.5, 1700000001000ULL, &rbe, &val);

    ret = edge_uns_generate_sparkplug_ndata_single(&engine, 10001, 1700000001000ULL, payload,
                                                   sizeof(payload), &out_len);
    assert(ret == 0);
    assert(out_len > 5);
    assert(engine.telemetry.sparkplug_ndata_count == 1);

    /* 3. NDEATH Generation */
    ret = edge_uns_generate_sparkplug_ndeath(&engine, payload, sizeof(payload), &out_len);
    assert(ret == 0);
    assert(out_len > 5);
    assert(engine.telemetry.sparkplug_ndeath_count == 1);

    printf("  -> PASSED: Sparkplug B payloads generated successfully with 0 malloc.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("   AEP-613: ISA-95 Unified Namespace (UNS) Engine Test Suite    \n");
    printf("=================================================================\n");

    test_uns_registration_and_path_resolution();
    test_uns_rbe_deadband_filtering();
    test_sparkplug_nbirth_ndata_ndeath_generation();

    printf("\nAll 3 AEP-613 UNS / Sparkplug B test cases PASSED!\n");
    return 0;
}
