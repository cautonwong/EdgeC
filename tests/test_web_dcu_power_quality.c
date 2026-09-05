/**
 * test_web_dcu_power_quality.c - Verification of DCU Packet Sniffer & Power Quality / Phase-Switching Views (AEP-496)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmocka.h>

#include "dcu_power_quality.h"

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

static void test_dcu_packet_capture_hex_dump_and_axdr_inspector(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/dcu/PacketCapture.vue", content, sizeof(content));

    // Sniffer headers & protocols
    assert_non_null(strstr(content, "PROTOCOL SNIFFER"));
    assert_non_null(strstr(content, "DL/T 645"));
    assert_non_null(strstr(content, "698.45-2017"));
    assert_non_null(strstr(content, "A-XDR"));

    // Filters and Hex colorization
    assert_non_null(strstr(content, "filterProtocol"));
    assert_non_null(strstr(content, "filterDirection"));
    assert_non_null(strstr(content, "searchAddress"));
    assert_non_null(strstr(content, "chunk.colorClass"));
    assert_non_null(strstr(content, "chunk.hex"));
    assert_non_null(strstr(content, "selectedPacket"));
    assert_non_null(strstr(content, "A-XDR DLT698 APDU TREE"));
}

static void test_dcu_power_quality_phasor_and_harmonics(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/dcu/PowerQuality.vue", content, sizeof(content));

    // Phasor polar diagram
    assert_non_null(strstr(content, "PHASOR POLAR DIAGRAM"));
    assert_non_null(strstr(content, "getPhasorX"));
    assert_non_null(strstr(content, "getPhasorY"));
    assert_non_null(strstr(content, "voltageA"));
    assert_non_null(strstr(content, "voltageB"));
    assert_non_null(strstr(content, "voltageC"));
    assert_non_null(strstr(content, "currentA"));

    // Symmetrical sequence decomposition
    assert_non_null(strstr(content, "SYMMETRICAL SEQUENCE DECOMPOSITION"));
    assert_non_null(strstr(content, "GB/T 15543"));
    assert_non_null(strstr(content, "posSeqVoltage"));
    assert_non_null(strstr(content, "negSeqVoltage"));
    assert_non_null(strstr(content, "zeroSeqVoltage"));
    assert_non_null(strstr(content, "neutralCurrent"));

    // 2~31th Harmonic Spectrum & GB/T 14549
    assert_non_null(strstr(content, "2~31st HARMONIC SPECTRUM"));
    assert_non_null(strstr(content, "GB/T 14549-1993"));
    assert_non_null(strstr(content, "thdVoltageTotal"));
    assert_non_null(strstr(content, "thdOddPct"));
    assert_non_null(strstr(content, "thdEvenPct"));
    assert_non_null(strstr(content, "hoveredHarmonic"));
}

static void test_dcu_power_quality_phase_commutation_optimizer(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/dcu/PowerQuality.vue", content, sizeof(content));

    // Phase-switching optimizer
    assert_non_null(strstr(content, "AUTOMATIC PHASE-SWITCHING OPTIMIZER"));
    assert_non_null(strstr(content, "switchableBranches"));
    assert_non_null(strstr(content, "isSwitchRecommended"));
    assert_non_null(strstr(content, "projectedUnbalancePct"));
    assert_non_null(strstr(content, "executeSwitchAction"));
    assert_non_null(strstr(content, "TRANSIENT PQ EVENTS"));
}

static void test_dcu_router_and_web_core_exports(void **state) {
    (void)state;
    char router_content[8192];
    read_file_content("web/core/src/router/index.ts", router_content, sizeof(router_content));
    assert_non_null(strstr(router_content, "dcu/packets"));
    assert_non_null(strstr(router_content, "dcu/power-quality"));
    assert_non_null(strstr(router_content, "PacketCapture.vue"));
    assert_non_null(strstr(router_content, "PowerQuality.vue"));

    char index_content[8192];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));
    assert_non_null(strstr(index_content, "PacketCapture"));
    assert_non_null(strstr(index_content, "PowerQuality"));
}

static void test_dcu_physical_unbalance_and_harmonic_algorithms(void **state) {
    (void)state;
    // Physical C algorithm verification
    dcu_power_quality_engine_t engine;
    dcu_power_quality_init(&engine, 220.0, 15.0);

    engine.ia = 84.2;
    engine.ib = 42.5;
    engine.ic = 36.8;

    double unbalance = dcu_calc_three_phase_unbalance(engine.ia, engine.ib, engine.ic);
    assert_true(unbalance > 15.0); // 84.2 vs ~54.5 is > 50% unbalance

    dcu_switchable_branch_t branches[3] = {
        { .meter_id = 101, .current_phase = PQ_PHASE_A, .current_amps = 18.5 },
        { .meter_id = 102, .current_phase = PQ_PHASE_A, .current_amps = 24.2 },
        { .meter_id = 103, .current_phase = PQ_PHASE_B, .current_amps = 12.0 },
    };

    dcu_phase_switch_plan_t plan;
    int rc = dcu_solve_phase_switch_plan(&engine, branches, 3, &plan);
    assert_int_equal(rc, 0);
    assert_true(plan.is_action_recommended);
    assert_int_equal(plan.actions[0].meter_id, 101);
    assert_int_equal(plan.actions[0].from_phase, PQ_PHASE_A);
    assert_int_equal(plan.actions[0].to_phase, PQ_PHASE_C);
    assert_true(plan.projected_unbalance_pct < plan.initial_unbalance_pct);

    // Harmonic THD calculation
    double harmonics[32] = {0};
    harmonics[3] = 6.27;  // 3rd harmonic (2.85% of 220V)
    harmonics[5] = 7.59;  // 5th harmonic (3.45% of 220V)
    harmonics[7] = 4.62;  // 7th harmonic (2.10% of 220V)

    dcu_harmonic_analysis_t analysis;
    rc = dcu_analyze_harmonics_thd(220.0, harmonics, &analysis);
    assert_int_equal(rc, 0);
    assert_true(analysis.thd_total_pct > 0.0);
    assert_true(analysis.thd_odd_pct > 0.0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dcu_packet_capture_hex_dump_and_axdr_inspector),
        cmocka_unit_test(test_dcu_power_quality_phasor_and_harmonics),
        cmocka_unit_test(test_dcu_power_quality_phase_commutation_optimizer),
        cmocka_unit_test(test_dcu_router_and_web_core_exports),
        cmocka_unit_test(test_dcu_physical_unbalance_and_harmonic_algorithms),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
