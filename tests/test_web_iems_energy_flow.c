/**
 * test_web_iems_energy_flow.c - Verification of IEMS Dynamic Energy Flow & TOU Dashboard (AEP-494)
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

static void test_sankey_power_flow_gpu_acceleration_and_bidirectional_mapping(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/components/SankeyEnergyFlow.vue", content, sizeof(content));

    // Dynamic SVG Sankey topology nodes
    assert_non_null(strstr(content, "SOLAR PV"));
    assert_non_null(strstr(content, "UTILITY GRID"));
    assert_non_null(strstr(content, "ESS BESS"));
    assert_non_null(strstr(content, "MG BUS"));
    assert_non_null(strstr(content, "LOAD"));
    assert_non_null(strstr(content, "EV CHARGER"));

    // GPU hardware acceleration & bidirectional flow classes
    assert_non_null(strstr(content, "stroke-dasharray"));
    assert_non_null(strstr(content, "will-change: stroke-dashoffset"));
    assert_non_null(strstr(content, "flow-forward"));
    assert_non_null(strstr(content, "flow-reverse"));
    assert_non_null(strstr(content, "getStrokeWidth"));
    assert_non_null(strstr(content, "getSpeed"));
}

static void test_tou_arbitrage_gantt_and_four_tier_tariff_schedule(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/components/TOUArbitrageWidget.vue", content, sizeof(content));

    // 4-stage tariff classification (Valley, Flat, Peak, Critical Peak)
    assert_non_null(strstr(content, "VALLEY (谷)"));
    assert_non_null(strstr(content, "FLAT (平)"));
    assert_non_null(strstr(content, "PEAK (峰)"));
    assert_non_null(strstr(content, "CRITICAL PEAK (尖)"));

    // 24H Gantt bar & economic calculations
    assert_non_null(strstr(content, "24H TOU Tariff Distribution"));
    assert_non_null(strstr(content, "hourlySchedule"));
    assert_non_null(strstr(content, "dailyRevenue"));
    assert_non_null(strstr(content, "Round-Trip RTE"));
}

static void test_anti_reverse_pid_deadband_and_curtailment_loop(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/components/AntiReversePIDGauge.vue", content, sizeof(content));

    // 100ms PID deadband & zero-export protection
    assert_non_null(strstr(content, "Loop: 100ms"));
    assert_non_null(strstr(content, "DEAD-BAND"));
    assert_non_null(strstr(content, "CURTAILMENT ACTIVE"));
    assert_non_null(strstr(content, "Kp / Ki / Kd"));
    assert_non_null(strstr(content, "describeArc"));
    assert_non_null(strstr(content, "PCC POINT POWER"));
}

static void test_demand_peak_shaving_and_transformer_redline_monitoring(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/components/DemandPeakShavingWidget.vue", content,
                      sizeof(content));

    // 15-min rolling demand & 500kVA transformer protection
    assert_non_null(strstr(content, "15-Min Rolling Demand"));
    assert_non_null(strstr(content, "Transformer Load Rate"));
    assert_non_null(strstr(content, "400 kW (80% REDLINE)"));
    assert_non_null(strstr(content, "500 kVA (MAX)"));
    assert_non_null(strstr(content, "historyDemand"));
}

static void test_energy_flow_view_assembly_and_router_export(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/energy/EnergyFlowView.vue", content, sizeof(content));

    // View component composition
    assert_non_null(strstr(content, "SankeyEnergyFlow"));
    assert_non_null(strstr(content, "TOUArbitrageWidget"));
    assert_non_null(strstr(content, "AntiReversePIDGauge"));
    assert_non_null(strstr(content, "DemandPeakShavingWidget"));

    // Router & Index export checks
    char index_content[16384];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));
    assert_non_null(strstr(index_content, "SankeyEnergyFlow"));
    assert_non_null(strstr(index_content, "TOUArbitrageWidget"));
    assert_non_null(strstr(index_content, "AntiReversePIDGauge"));
    assert_non_null(strstr(index_content, "DemandPeakShavingWidget"));
    assert_non_null(strstr(index_content, "EnergyFlowView"));

    char router_content[16384];
    read_file_content("web/core/src/router/index.ts", router_content, sizeof(router_content));
    assert_non_null(strstr(router_content, "EnergyFlowView.vue"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sankey_power_flow_gpu_acceleration_and_bidirectional_mapping),
        cmocka_unit_test(test_tou_arbitrage_gantt_and_four_tier_tariff_schedule),
        cmocka_unit_test(test_anti_reverse_pid_deadband_and_curtailment_loop),
        cmocka_unit_test(test_demand_peak_shaving_and_transformer_redline_monitoring),
        cmocka_unit_test(test_energy_flow_view_assembly_and_router_export),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
