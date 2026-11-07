/**
 * test_web_design_tokens.c - Verification of the EdgeOS Web token system.
 *
 * The single source of truth is web/core/src/ui/tokens.css. Product Tailwind
 * configs only bridge the legacy palette to those tokens.
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

static void test_token_source_semantics(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/ui/tokens.css", content, sizeof(content));

    assert_non_null(strstr(content, "--color-primary:"));
    assert_non_null(strstr(content, "--bg-page:"));
    assert_non_null(strstr(content, "--bg-surface:"));
    assert_non_null(strstr(content, "--color-success:"));
    assert_non_null(strstr(content, "--color-warning:"));
    assert_non_null(strstr(content, "--color-danger:"));
    assert_non_null(strstr(content, "--color-primary-rgb:"));
    assert_non_null(strstr(content, "--chart-1:"));
    assert_non_null(strstr(content, "'Edge Sans'"));
    assert_non_null(strstr(content, "'Edge Mono'"));
    assert_non_null(strstr(content, "'Edge Emoji'"));
}

static void test_token_source_dark_theme_and_fonts(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/ui/tokens.css", content, sizeof(content));

    assert_non_null(strstr(content, "[data-theme='dark']"));
    assert_non_null(strstr(content, "color-scheme: dark"));
    assert_non_null(strstr(content, "--bg-page-rgb:"));
    assert_non_null(strstr(content, "[data-brand]"));
    assert_non_null(strstr(content, "--brand-on-primary"));
    assert_non_null(strstr(content, "@font-face"));
    assert_non_null(strstr(content, "edge-sans-400.woff2"));
    assert_non_null(strstr(content, "edge-mono-400.woff2"));
}

static void test_tailwind_preset_bridge(void **state) {
    (void)state;
    char preset[16384];
    read_file_content("web/tooling/tailwind.preset.cjs", preset, sizeof(preset));
    assert_non_null(strstr(preset, "var(--"));
    assert_non_null(strstr(preset, "Edge Sans"));
    assert_non_null(strstr(preset, "Edge Mono"));
    assert_non_null(strstr(preset, "color-success"));
    assert_non_null(strstr(preset, "bg-surface"));

    char iems_cfg[4096];
    read_file_content("web/products/iems/tailwind.config.cjs", iems_cfg, sizeof(iems_cfg));
    assert_non_null(strstr(iems_cfg, "tailwind.preset.cjs"));

    char box_cfg[4096];
    read_file_content("web/products/smart_box/tailwind.config.cjs", box_cfg, sizeof(box_cfg));
    assert_non_null(strstr(box_cfg, "tailwind.preset.cjs"));

    char dcu_cfg[4096];
    read_file_content("web/products/dcu/tailwind.config.cjs", dcu_cfg, sizeof(dcu_cfg));
    assert_non_null(strstr(dcu_cfg, "tailwind.preset.cjs"));
}

static void test_html_theme_entrypoints(void **state) {
    (void)state;
    char iems_html[8192];
    read_file_content("web/products/iems/index.html", iems_html, sizeof(iems_html));
    assert_null(strstr(iems_html, "fonts.googleapis.com"));
    assert_null(strstr(iems_html, "fonts.gstatic.com"));
    assert_non_null(strstr(iems_html, "dataset.theme"));
    assert_non_null(strstr(iems_html, "edgeos.theme"));

    char box_html[4096];
    read_file_content("web/products/smart_box/index.html", box_html, sizeof(box_html));
    assert_null(strstr(box_html, "fonts.googleapis.com"));
    assert_null(strstr(box_html, "class=\"dark\""));
    assert_non_null(strstr(box_html, "data-theme=\"dark\""));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_token_source_semantics),
        cmocka_unit_test(test_token_source_dark_theme_and_fonts),
        cmocka_unit_test(test_tailwind_preset_bridge),
        cmocka_unit_test(test_html_theme_entrypoints),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
