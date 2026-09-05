/**
 * test_web_cli_bridge.c - Verification of Interactive Web CLI & PTY Bridge (AEP-502)
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
#include "edgeos/console_ws.h"

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

static void test_web_interactive_shell_component_structure(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/components/console/InteractiveShell.vue", content, sizeof(content));

    // Headers & PTY Bridge Modes
    assert_non_null(strstr(content, "EdgeOS Industrial Interactive Web Shell Bridge"));
    assert_non_null(strstr(content, "channelMode"));
    assert_non_null(strstr(content, "/api/v1/ws/cli"));
    assert_non_null(strstr(content, "/api/v1/debug/cli/exec"));
    assert_non_null(strstr(content, "/api/v1/debug/cli/completions"));

    // Interactive Terminal Features
    assert_non_null(strstr(content, "promptPrefix"));
    assert_non_null(strstr(content, "currentInput"));
    assert_non_null(strstr(content, "historyEntries"));
    assert_non_null(strstr(content, "commandHistory"));
    assert_non_null(strstr(content, "navigateHistory"));
    assert_non_null(strstr(content, "handleTabCompletion"));
    assert_non_null(strstr(content, "handleInterrupt"));
    assert_non_null(strstr(content, "executeCurrentCommand"));
    assert_non_null(strstr(content, "simulateLocalCommandDispatch"));
    assert_non_null(strstr(content, "renderAnsiText"));

    // Check index.ts export
    char index_content[4096];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));
    assert_non_null(strstr(index_content, "export { default as InteractiveShell }"));
}

static void test_c_console_ws_dispatch(void **state) {
    (void)state;
    console_cmd_result_t result;

    assert_int_equal(console_ws_init(), 0);

    // Test 'help'
    assert_int_equal(console_ws_dispatch_command("help", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "EdgeOS Built-in Shell Commands"));
    assert_true(result.timestamp_ns > 0);

    // Test 'version'
    assert_int_equal(console_ws_dispatch_command("version", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "EdgeOS Microkernel"));

    // Test 'uptime'
    assert_int_equal(console_ws_dispatch_command("uptime", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "Control Plane Uptime"));

    // Test 'datahub stats'
    assert_int_equal(console_ws_dispatch_command("datahub stats", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "DataHub Shadow Bus Summary"));
    assert_non_null(strstr(result.output, "Watermark"));

    // Test 'dds peers'
    assert_int_equal(console_ws_dispatch_command("dds peers", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "Cyclone DDS Mesh Status"));

    // Test 'mtt clock'
    assert_int_equal(console_ws_dispatch_command("mtt clock", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "MTT Deterministic Clock"));

    // Test 'drivers list'
    assert_int_equal(console_ws_dispatch_command("drivers list", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "Registered Drivers"));

    // Test 'streams dag'
    assert_int_equal(console_ws_dispatch_command("streams dag", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "Stream Engine DAG"));

    // Test 'storage wal'
    assert_int_equal(console_ws_dispatch_command("storage wal", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "Telemetry Storage & WAL"));

    // Test 'metrics top'
    assert_int_equal(console_ws_dispatch_command("metrics top", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "edgeos_datahub_watermark_ratio"));

    // Test 'tag read'
    assert_int_equal(console_ws_dispatch_command("tag read FREQ_HZ", &result), 0);
    assert_int_equal(result.status_code, 0);
    assert_non_null(strstr(result.output, "Tag [FREQ_HZ]"));

    // Test unknown command
    assert_int_equal(console_ws_dispatch_command("nonexistent_command_123", &result), 0);
    assert_int_equal(result.status_code, 127);
    assert_non_null(strstr(result.output, "Command not found"));
}

static void test_c_console_ws_completions(void **state) {
    (void)state;
    char completions[CONSOLE_WS_MAX_COMPLETIONS][64];
    size_t count = 0;

    // Prefix "data"
    assert_int_equal(console_ws_get_completions("data", completions, CONSOLE_WS_MAX_COMPLETIONS, &count), 0);
    assert_true(count >= 3);
    assert_string_equal(completions[0], "datahub");
    assert_string_equal(completions[1], "datahub stats");
    assert_string_equal(completions[2], "datahub dump");

    // Prefix "dds"
    assert_int_equal(console_ws_get_completions("dds", completions, CONSOLE_WS_MAX_COMPLETIONS, &count), 0);
    assert_true(count >= 4);
    assert_string_equal(completions[0], "dds");
    assert_string_equal(completions[1], "dds peers");

    // Prefix "unknown"
    assert_int_equal(console_ws_get_completions("unknown_xyz", completions, CONSOLE_WS_MAX_COMPLETIONS, &count), 0);
    assert_int_equal(count, 0);

    // Empty prefix -> all commands up to limit
    assert_int_equal(console_ws_get_completions("", completions, 10, &count), 0);
    assert_int_equal(count, 10);
}

static void test_c_console_ws_json_formatting(void **state) {
    (void)state;
    console_cmd_result_t res;
    memset(&res, 0, sizeof(res));
    res.status_code = 0;
    res.timestamp_ns = 1725500000000000000ULL;
    snprintf(res.output, sizeof(res.output), "Line 1: \"EdgeOS\"\nLine 2: Ready");

    char json_buf[1024];
    assert_int_equal(console_ws_format_json_response(&res, json_buf, sizeof(json_buf)), 0);

    assert_non_null(strstr(json_buf, "\"code\":0"));
    assert_non_null(strstr(json_buf, "\"timestamp\":1725500000000000000"));
    assert_non_null(strstr(json_buf, "\\\"EdgeOS\\\""));
    assert_non_null(strstr(json_buf, "\\nLine 2: Ready"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_web_interactive_shell_component_structure),
        cmocka_unit_test(test_c_console_ws_dispatch),
        cmocka_unit_test(test_c_console_ws_completions),
        cmocka_unit_test(test_c_console_ws_json_formatting),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
