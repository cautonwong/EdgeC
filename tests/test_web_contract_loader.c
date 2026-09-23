/**
 * test_web_contract_loader.c - Verification of Web ContractLoader & Dynamic UI Aggregation
 * (AEP-491)
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

static void test_ts_compiler_backend_ui_metadata(void **state) {
    (void)state;
    char content[16384];
    read_file_content("tools/contract-compiler-v2/internal/backend/ts_backend.go", content,
                      sizeof(content));

    // TS backend generates TagUIMetadata interface
    assert_non_null(strstr(content, "export interface TagUIMetadata"));
    assert_non_null(strstr(content, "group?: string;"));
    assert_non_null(strstr(content, "widget?: string;"));
    assert_non_null(strstr(content, "precision?: number;"));
    assert_non_null(strstr(content, "TAG_METADATA"));
}

static void test_contract_loader_pinia_store(void **state) {
    (void)state;
    char content[16384];
    read_file_content("web/core/src/services/ContractLoader.ts", content, sizeof(content));

    // Verify Pinia contract store and dynamic grouping
    assert_non_null(strstr(content, "export const useContractStore"));
    assert_non_null(strstr(content, "groups: new Map<string, TagMeta[]>()"));
    assert_non_null(strstr(content, "branding: (state) =>"));
    assert_non_null(strstr(content, "loadStaticFallback"));
    assert_non_null(strstr(content, "filteredTags: (state)"));
}

static void test_dashboard_vue_contract_driven(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/Dashboard.vue", content, sizeof(content));

    // Verify dynamic contract-driven rendering without hardcoded physical names
    assert_non_null(strstr(content, "useContractStore()"));
    assert_non_null(strstr(content, "v-for=\"groupName in contractStore.groupList\""));
    assert_non_null(strstr(content, "v-for=\"tag in tags\""));
    assert_non_null(strstr(content, ":key=\"tag.id\""));
    assert_non_null(strstr(content, "bg-[#020617]"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ts_compiler_backend_ui_metadata),
        cmocka_unit_test(test_contract_loader_pinia_store),
        cmocka_unit_test(test_dashboard_vue_contract_driven),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
