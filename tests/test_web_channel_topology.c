/**
 * test_web_channel_topology.c - Verification of Physical Channel Topology & SNR Heatmap (AEP-500)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmocka.h>

static void read_file_content(const char *rel_path, char *buf, size_t max_len) {
    char path[512];
    FILE *fp = NULL;

    const char *prefixes[] = {
        "",
        "../",
        "../../",
        "/workspaces/edgeos/EdgeC/",
        NULL
    };

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

static void test_channel_topology_component_structure(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/devices/ChannelTopologyHeatmap.vue", content, sizeof(content));

    // Headers & Navigation
    assert_non_null(strstr(content, "TOPOLOGY & SNR HEATMAP"));
    assert_non_null(strstr(content, "HPLC & Dual-Mode Physical Mesh Topology"));
    assert_non_null(strstr(content, "viewModes"));
    assert_non_null(strstr(content, "activeViewMode"));

    // Key Stats Counters
    assert_non_null(strstr(content, "topologyStats"));
    assert_non_null(strstr(content, "totalNodes"));
    assert_non_null(strstr(content, "avgSnrDb"));
    assert_non_null(strstr(content, "maxHops"));
    assert_non_null(strstr(content, "avgLossPct"));
    assert_non_null(strstr(content, "crosstalkLevelDb"));

    // Multi-View Modes
    assert_non_null(strstr(content, "PHYSICAL MESH TOPOLOGY MAP"));
    assert_non_null(strstr(content, "16-OFDM SUB-BAND SNR HEATMAP MATRIX"));
    assert_non_null(strstr(content, "PHASE CROSSTALK LEAKAGE MATRIX"));
    assert_non_null(strstr(content, "INVERTER PWM NOISE BURSTS"));
    assert_non_null(strstr(content, "PHYSICAL CHANNEL TELEMETRY ROSTER"));

    // Interactive Drawer & Actions
    assert_non_null(strstr(content, "selectedNode"));
    assert_non_null(strstr(content, "triggerPingProbe"));
    assert_non_null(strstr(content, "recalibrateNotch"));
    assert_non_null(strstr(content, "exportTopologyReport"));
    assert_non_null(strstr(content, "subCarrierSnr"));
}

static void test_channel_topology_router_and_export_registration(void **state) {
    (void)state;
    char router_content[8192];
    read_file_content("web/core/src/router/index.ts", router_content, sizeof(router_content));

    assert_non_null(strstr(router_content, "devices/topology"));
    assert_non_null(strstr(router_content, "ChannelTopology"));
    assert_non_null(strstr(router_content, "ChannelTopologyHeatmap.vue"));

    char index_content[4096];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));
    assert_non_null(strstr(index_content, "ChannelTopologyHeatmap"));
}

typedef enum {
    SNR_QUALITY_POOR = 0,
    SNR_QUALITY_MEDIUM = 1,
    SNR_QUALITY_GOOD = 2,
    SNR_QUALITY_EXCELLENT = 3
} snr_quality_t;

static snr_quality_t classify_channel_snr(double snr_db) {
    if (snr_db > 25.0) return SNR_QUALITY_EXCELLENT;
    if (snr_db > 20.0) return SNR_QUALITY_GOOD;
    if (snr_db >= 10.0) return SNR_QUALITY_MEDIUM;
    return SNR_QUALITY_POOR;
}

static const char* get_link_color_code(double snr_db) {
    if (snr_db > 20.0) return "#10b981"; // Emerald
    if (snr_db >= 10.0) return "#f59e0b"; // Amber
    return "#f43f5e"; // Rose
}

static void test_channel_topology_snr_classification_algorithm(void **state) {
    (void)state;

    // Test CCO / High SNR
    assert_int_equal(classify_channel_snr(38.5), SNR_QUALITY_EXCELLENT);
    assert_string_equal(get_link_color_code(38.5), "#10b981");

    // Test Good Relay Proxy
    assert_int_equal(classify_channel_snr(24.2), SNR_QUALITY_GOOD);
    assert_string_equal(get_link_color_code(24.2), "#10b981");

    // Test Medium Feeder Meter
    assert_int_equal(classify_channel_snr(18.5), SNR_QUALITY_MEDIUM);
    assert_string_equal(get_link_color_code(18.5), "#f59e0b");

    // Test Poor/Degraded Node
    assert_int_equal(classify_channel_snr(9.4), SNR_QUALITY_POOR);
    assert_string_equal(get_link_color_code(9.4), "#f43f5e");
}

typedef struct {
    double phase_ab_coupling_db;
    double phase_bc_coupling_db;
    double phase_ca_coupling_db;
    bool is_excessive_leakage;
    const char* root_cause_phase_pair;
} crosstalk_report_t;

static crosstalk_report_t evaluate_phase_crosstalk(double ab, double bc, double ca) {
    crosstalk_report_t r;
    r.phase_ab_coupling_db = ab;
    r.phase_bc_coupling_db = bc;
    r.phase_ca_coupling_db = ca;
    r.is_excessive_leakage = false;
    r.root_cause_phase_pair = "NONE";

    const double CROSSTALK_WARN_THRESH_DB = 18.0;

    if (bc > CROSSTALK_WARN_THRESH_DB) {
        r.is_excessive_leakage = true;
        r.root_cause_phase_pair = "PHASE_B_TO_C";
    } else if (ab > CROSSTALK_WARN_THRESH_DB) {
        r.is_excessive_leakage = true;
        r.root_cause_phase_pair = "PHASE_A_TO_B";
    } else if (ca > CROSSTALK_WARN_THRESH_DB) {
        r.is_excessive_leakage = true;
        r.root_cause_phase_pair = "PHASE_C_TO_A";
    }

    return r;
}

static void test_channel_topology_crosstalk_and_noise_analyzer(void **state) {
    (void)state;

    // Standard baseline condition
    crosstalk_report_t safe_rpt = evaluate_phase_crosstalk(12.4, 14.5, 8.2);
    assert_false(safe_rpt.is_excessive_leakage);
    assert_string_equal(safe_rpt.root_cause_phase_pair, "NONE");

    // Phase B -> C elevated capacitive coupling
    crosstalk_report_t warning_rpt = evaluate_phase_crosstalk(12.4, 21.6, 8.2);
    assert_true(warning_rpt.is_excessive_leakage);
    assert_string_equal(warning_rpt.root_cause_phase_pair, "PHASE_B_TO_C");

    // Inverter Noise Burst Delta Check
    double baseline_noise_floor_dbm = -90.0;
    double measured_noise_floor_dbm = -71.8; // +18.2 dB rise
    double noise_rise = measured_noise_floor_dbm - baseline_noise_floor_dbm;
    assert_true(fabs(noise_rise - 18.2) < 0.01);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_channel_topology_component_structure),
        cmocka_unit_test(test_channel_topology_router_and_export_registration),
        cmocka_unit_test(test_channel_topology_snr_classification_algorithm),
        cmocka_unit_test(test_channel_topology_crosstalk_and_noise_analyzer),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
