/**
 * test_web_design_tokens.c - Verification of EdgeOS Web Design System Tokens (AEP-490)
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

static void test_tokens_ts_structure(void **state) {
    (void)state;
    char content[8192];
    read_file_content("web/core/src/tokens.ts", content, sizeof(content));

    // Base OLED Palette
    assert_non_null(strstr(content, "base: '#020617'"));
    assert_non_null(strstr(content, "surface: '#0f172a'"));
    assert_non_null(strstr(content, "elevated: '#1e293b'"));

    // Domain Status
    assert_non_null(strstr(content, "normal: '#10b981'"));
    assert_non_null(strstr(content, "warning: '#f59e0b'"));
    assert_non_null(strstr(content, "fault: '#ef4444'"));

    // Power Flow Vectors
    assert_non_null(strstr(content, "grid: '#38bdf8'"));
    assert_non_null(strstr(content, "pv: '#fbbf24'"));
    assert_non_null(strstr(content, "essCharge: '#34d399'"));
    assert_non_null(strstr(content, "essDischarge: '#f97316'"));

    // Dual-track Typography
    assert_non_null(strstr(content, "Inter"));
    assert_non_null(strstr(content, "JetBrains Mono"));
}

static void test_tokens_css_variables(void **state) {
    (void)state;
    char content[8192];
    read_file_content("web/core/src/styles/tokens.css", content, sizeof(content));

    assert_non_null(strstr(content, "--edge-bg-base: #020617;"));
    assert_non_null(strstr(content, "--edge-bg-surface: #0f172a;"));
    assert_non_null(strstr(content, "--edge-status-normal: #10b981;"));
    assert_non_null(strstr(content, "--edge-status-fault: #ef4444;"));
    assert_non_null(strstr(content, "--edge-flow-grid: #38bdf8;"));
    assert_non_null(strstr(content, "--font-ui: 'Inter'"));
    assert_non_null(strstr(content, "--font-mono: 'JetBrains Mono'"));
    assert_non_null(strstr(content, "font-feature-settings: 'tnum' on, 'zero' on;"));
}

static void test_tailwind_configurations(void **state) {
    (void)state;
    char iems_cfg[8192];
    read_file_content("web/products/iems/tailwind.config.cjs", iems_cfg, sizeof(iems_cfg));
    assert_non_null(strstr(iems_cfg, "'#020617'"));
    assert_non_null(strstr(iems_cfg, "JetBrains Mono"));
    assert_non_null(strstr(iems_cfg, "Inter"));

    char box_cfg[8192];
    read_file_content("web/products/smart_box/tailwind.config.cjs", box_cfg, sizeof(box_cfg));
    assert_non_null(strstr(box_cfg, "'#020617'"));
    assert_non_null(strstr(box_cfg, "JetBrains Mono"));
}

static void test_html_typography_links(void **state) {
    (void)state;
    char iems_html[4096];
    read_file_content("web/products/iems/index.html", iems_html, sizeof(iems_html));
    assert_non_null(strstr(iems_html, "family=Inter"));
    assert_non_null(strstr(iems_html, "family=JetBrains+Mono"));
    assert_non_null(strstr(iems_html, "class=\"dark\""));

    char box_html[4096];
    read_file_content("web/products/smart_box/index.html", box_html, sizeof(box_html));
    assert_non_null(strstr(box_html, "family=Inter"));
    assert_non_null(strstr(box_html, "family=JetBrains+Mono"));
    assert_non_null(strstr(box_html, "class=\"dark\""));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tokens_ts_structure),
        cmocka_unit_test(test_tokens_css_variables),
        cmocka_unit_test(test_tailwind_configurations),
        cmocka_unit_test(test_html_typography_links),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
