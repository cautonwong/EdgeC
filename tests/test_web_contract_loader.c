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
    read_file_content("web/core/src/services/ContractLoader.ts", content,
                      sizeof(content));

    // ContractLoader defines TagUIMeta interface for autonomous UI widgets
    assert_non_null(strstr(content, "export interface TagUIMeta"));
    assert_non_null(strstr(content, "group?: string;"));
    assert_non_null(strstr(content, "widget?:"));
    assert_non_null(strstr(content, "precision?: number;"));
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

static void test_dashboard_vue_telemetry_driven(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/modules/Dashboard.vue", content, sizeof(content));

    // 概览页已从"契约点位轮询"迁到遥测快照 + 推送：不再依赖 ContractStore，
    // 也不再硬编码任何物理点位名（渲染由数据驱动）。
    assert_non_null(strstr(content, "/api/v1/telemetry/current"));
    assert_non_null(strstr(content, "useLiveFeed"));
    assert_non_null(strstr(content, "v-for=\"point in visible\""));
    assert_non_null(strstr(content, ":key=\"point.pointId\""));
    assert_null(strstr(content, "contractStore"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ts_compiler_backend_ui_metadata),
        cmocka_unit_test(test_contract_loader_pinia_store),
        cmocka_unit_test(test_dashboard_vue_telemetry_driven),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
