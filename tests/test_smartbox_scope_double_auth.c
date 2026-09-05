/**
 * test_smartbox_scope_double_auth.c - Verification of SmartBox 100Hz Scope & Double-Auth Safety Modal (AEP-495)
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

static void test_waveform_scope_100hz_canvas_and_controls(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/products/smart_box/src/components/WaveformScope100Hz.vue", content, sizeof(content));

    // 100Hz sampling & Canvas 2D engine
    assert_non_null(strstr(content, "100Hz HARDWARE"));
    assert_non_null(strstr(content, "OSCILLOSCOPE"));
    assert_non_null(strstr(content, "canvasRef"));
    assert_non_null(strstr(content, "requestAnimationFrame"));
    assert_non_null(strstr(content, "BUFFER_SIZE"));
    assert_non_null(strstr(content, "window.devicePixelRatio"));

    // 3 physical channels
    assert_non_null(strstr(content, "CH1: Gait (100Hz)"));
    assert_non_null(strstr(content, "CH2: ECG (100Hz)"));
    assert_non_null(strstr(content, "CH3: EMG (100Hz)"));

    // Oscilloscope knobs
    assert_non_null(strstr(content, "Timebase"));
    assert_non_null(strstr(content, "Voltage Scale"));
    assert_non_null(strstr(content, "Trigger Mode"));
    assert_non_null(strstr(content, "toggleFullscreen"));
}

static void test_double_auth_safety_interlock_modal(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/products/smart_box/src/components/DoubleAuthModal.vue", content, sizeof(content));

    // Safety Interlock & Four-Eyes Principle
    assert_non_null(strstr(content, "CRITICAL SAFETY INTERLOCK"));
    assert_non_null(strstr(content, "Four-Eyes Principle (双人复核)"));
    assert_non_null(strstr(content, "HARDWARE_RELAY_CUTOFF"));
    assert_non_null(strstr(content, "Operator PIN"));
    assert_non_null(strstr(content, "Supervisor PIN"));

    // 3s safety countdown & dual validation logic
    assert_non_null(strstr(content, "countdown"));
    assert_non_null(strstr(content, "startCountdown"));
    assert_non_null(strstr(content, "防误触等待"));
    assert_non_null(strstr(content, "operatorPin"));
    assert_non_null(strstr(content, "supervisorPin"));
}

static void test_smartbox_app_integration_and_privacy_interlock(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/products/smart_box/src/App.vue", content, sizeof(content));

    // Integration of 100Hz Scope and Double Auth Modal
    assert_non_null(strstr(content, "WaveformScope100Hz"));
    assert_non_null(strstr(content, "DoubleAuthModal"));
    assert_non_null(strstr(content, "requestPrivacyToggle"));
    assert_non_null(strstr(content, "onAuthConfirmed"));
    assert_non_null(strstr(content, "isAuthModalOpen"));
    assert_non_null(strstr(content, "GameBridge"));
    assert_non_null(strstr(content, "PositiveFeedback"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_waveform_scope_100hz_canvas_and_controls),
        cmocka_unit_test(test_double_auth_safety_interlock_modal),
        cmocka_unit_test(test_smartbox_app_integration_and_privacy_interlock),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
