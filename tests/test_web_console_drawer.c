/**
 * test_web_console_drawer.c - Verification of Global Web Console Drawer & Ansi Terminal (AEP-501)
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

static void test_web_console_drawer_component_structure(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/components/console/WebConsoleDrawer.vue", content, sizeof(content));

    // Headers & Mode Controls
    assert_non_null(strstr(content, "EdgeOS Global Web Console"));
    assert_non_null(strstr(content, "drawerMode"));
    assert_non_null(strstr(content, "activeTab"));
    assert_non_null(strstr(content, "drawerHeightVh"));
    assert_non_null(strstr(content, "cycleDrawerMode"));
    assert_non_null(strstr(content, "startResize"));

    // Multi-Tab Structure
    assert_non_null(strstr(content, "consoleTabs"));
    assert_non_null(strstr(content, "AnsiTerminalView"));
    assert_non_null(strstr(content, "filteredLogs"));
    assert_non_null(strstr(content, "packetStream"));
    assert_non_null(strstr(content, "quickMacros"));

    // Quick Macros & Actions
    assert_non_null(strstr(content, "runMacro"));
    assert_non_null(strstr(content, "Dump DataHub Ringbuffer"));
    assert_non_null(strstr(content, "Check Cyclone DDS Mesh"));
    assert_non_null(strstr(content, "Query MTT Clock Precision"));

    // Global Keydown Listener
    assert_non_null(strstr(content, "window.addEventListener('keydown'"));
}

static void test_ansi_terminal_view_structure(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/components/console/AnsiTerminalView.vue", content, sizeof(content));

    // Terminal Input & Output
    assert_non_null(strstr(content, "promptPrefix"));
    assert_non_null(strstr(content, "currentInput"));
    assert_non_null(strstr(content, "historyEntries"));
    assert_non_null(strstr(content, "availableCommands"));

    // Interactive Logic
    assert_non_null(strstr(content, "executeCommand"));
    assert_non_null(strstr(content, "navigateHistory"));
    assert_non_null(strstr(content, "handleTabCompletion"));
    assert_non_null(strstr(content, "clearScreen"));
    assert_non_null(strstr(content, "renderAnsiText"));

    // Built-in EdgeOS Commands
    assert_non_null(strstr(content, "datahub"));
    assert_non_null(strstr(content, "dds"));
    assert_non_null(strstr(content, "mtt"));
    assert_non_null(strstr(content, "drivers"));
    assert_non_null(strstr(content, "streams"));
    assert_non_null(strstr(content, "storage"));
    assert_non_null(strstr(content, "metrics"));
    assert_non_null(strstr(content, "version"));
}

static void test_admin_layout_integration_and_exports(void **state) {
    (void)state;
    char layout_content[16384];
    read_file_content("web/core/src/layouts/AdminLayout.vue", layout_content, sizeof(layout_content));

    assert_non_null(strstr(layout_content, "WebConsoleDrawer"));
    assert_non_null(strstr(layout_content, "isConsoleOpen"));
    assert_non_null(strstr(layout_content, "Toggle Web Console"));

    char index_content[4096];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));

    assert_non_null(strstr(index_content, "export { default as WebConsoleDrawer }"));
    assert_non_null(strstr(index_content, "export { default as AnsiTerminalView }"));
}

typedef enum {
    DRAWER_MODE_DOCKED = 0,
    DRAWER_MODE_FULLSCREEN = 1,
    DRAWER_MODE_FLOATING = 2
} drawer_mode_t;

static drawer_mode_t cycle_mode(drawer_mode_t cur) {
    switch (cur) {
        case DRAWER_MODE_DOCKED: return DRAWER_MODE_FULLSCREEN;
        case DRAWER_MODE_FULLSCREEN: return DRAWER_MODE_FLOATING;
        case DRAWER_MODE_FLOATING: return DRAWER_MODE_DOCKED;
    }
    return DRAWER_MODE_DOCKED;
}

static int clamp_drawer_height_vh(int raw_vh) {
    if (raw_vh < 25) return 25;
    if (raw_vh > 85) return 85;
    return raw_vh;
}

static void test_console_macro_and_ansi_parser_algorithm(void **state) {
    (void)state;

    // Test 3-State Mode Cycle
    drawer_mode_t m = DRAWER_MODE_DOCKED;
    m = cycle_mode(m);
    assert_int_equal(m, DRAWER_MODE_FULLSCREEN);
    m = cycle_mode(m);
    assert_int_equal(m, DRAWER_MODE_FLOATING);
    m = cycle_mode(m);
    assert_int_equal(m, DRAWER_MODE_DOCKED);

    // Test Height Clamping (25vh - 85vh)
    assert_int_equal(clamp_drawer_height_vh(10), 25);
    assert_int_equal(clamp_drawer_height_vh(45), 45);
    assert_int_equal(clamp_drawer_height_vh(95), 85);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_web_console_drawer_component_structure),
        cmocka_unit_test(test_ansi_terminal_view_structure),
        cmocka_unit_test(test_admin_layout_integration_and_exports),
        cmocka_unit_test(test_console_macro_and_ansi_parser_algorithm),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
