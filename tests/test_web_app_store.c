/**
 * test_web_app_store.c - Edge app store page and registration contract.
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

static void test_app_store_page_contract(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/modules/apps/AppStore.vue", content, sizeof(content));

    assert_non_null(strstr(content, "应用商店"));
    assert_non_null(strstr(content, "data-testid=\"app-search\""));
    assert_non_null(strstr(content, "data-testid=\"app-card\""));
    assert_non_null(strstr(content, "core.app:execute"));
    assert_non_null(strstr(content, "useDialogA11y"));
    assert_non_null(strstr(content, "role=\"dialog\""));
    assert_non_null(strstr(content, "edgeos.appstore.installed"));
}

static void test_app_catalog_contract(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/modules/apps/catalog.ts", content, sizeof(content));

    assert_non_null(strstr(content, "voltage_guard"));
    assert_non_null(strstr(content, "demand_forecast"));
    assert_non_null(strstr(content, "modbus_bridge"));
    assert_non_null(strstr(content, "audit_shield"));
    assert_non_null(strstr(content, "CATEGORY_LABELS"));
}

static void test_app_store_registration_and_permissions(void **state) {
    (void)state;
    char manifest[65536];
    read_file_content("web/core/modules.manifest.yaml", manifest, sizeof(manifest));
    assert_non_null(strstr(manifest, "core.apps"));
    assert_non_null(strstr(manifest, "/apps"));
    assert_non_null(strstr(manifest, "apps/AppStore.vue"));
    assert_non_null(strstr(manifest, "core.app:read"));

    char catalog[65536];
    read_file_content("web/permissions/catalog.yaml", catalog, sizeof(catalog));
    assert_non_null(strstr(catalog, "core.app:read"));
    assert_non_null(strstr(catalog, "core.app:execute"));

    char roles[65536];
    read_file_content("web/permissions/role-presets.yaml", roles, sizeof(roles));
    assert_non_null(strstr(roles, "core.app:read"));
    assert_non_null(strstr(roles, "core.app:execute"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_app_store_page_contract),
        cmocka_unit_test(test_app_catalog_contract),
        cmocka_unit_test(test_app_store_registration_and_permissions),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
