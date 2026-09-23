/**
 * @file test_developer_manuals.c
 * @brief CMocka Unit & Physical Consistency Test for EdgeOS Secondary Developer Manuals (AEP-406)
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
#include <unistd.h>

#define MANUAL_TEST_PROD "manual_quickstart_bess"

static void get_repo_root(char *out, size_t max_len) {
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) {
        snprintf(out, max_len, ".");
        return;
    }
    if (access(".local/linux-x86_64/bin/edge-cli", X_OK) == 0) {
        snprintf(out, max_len, "%s", cwd);
    } else if (access("../.local/linux-x86_64/bin/edge-cli", X_OK) == 0) {
        snprintf(out, max_len, "%s/..", cwd);
    } else if (access("../../.local/linux-x86_64/bin/edge-cli", X_OK) == 0) {
        snprintf(out, max_len, "%s/../..", cwd);
    } else {
        snprintf(out, max_len, ".");
    }
}

static bool file_exists_and_contains(const char *path, const char *keyword) {
    FILE *fp = fopen(path, "r");
    if (!fp) return false;

    char buffer[1024];
    bool found = false;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (strstr(buffer, keyword)) {
            found = true;
            break;
        }
    }
    fclose(fp);
    return found;
}

static void test_manual_documents_exist(void **state) {
    (void)state;
    char root[1024];
    get_repo_root(root, sizeof(root));

    char guide_path[2048];
    snprintf(guide_path, sizeof(guide_path), "%s/docs/manuals/secondary_developer_guide.md", root);
    assert_true(file_exists_and_contains(guide_path, "EdgeOS 嵌入式二次开发工程师手册"));
    assert_true(file_exists_and_contains(guide_path, "edge-cli create-product"));
    assert_true(file_exists_and_contains(guide_path, "edge-cli migrate"));
    assert_true(file_exists_and_contains(guide_path, "依赖倒置"));

    char qs_path[2048];
    snprintf(qs_path, sizeof(qs_path), "%s/docs/manuals/quickstart_5min.md", root);
    assert_true(file_exists_and_contains(qs_path, "5分钟极简上手指南"));
    assert_true(file_exists_and_contains(qs_path, "microgrid_box"));
}

static void test_manual_sdk_headers_available(void **state) {
    (void)state;
    char root[1024];
    get_repo_root(root, sizeof(root));

    char path[2048];
    snprintf(path, sizeof(path), "%s/include/edgeos/edgeos.h", root);
    assert_true(file_exists_and_contains(path, "edgeos_sdk_init"));

    snprintf(path, sizeof(path), "%s/include/edgeos/datahub.h", root);
    assert_true(file_exists_and_contains(path, "edgeos_datahub_t"));

    snprintf(path, sizeof(path), "%s/include/edgeos/driver.h", root);
    assert_true(file_exists_and_contains(path, "edgeos_driver_t"));

    snprintf(path, sizeof(path), "%s/include/edgeos/anti_corruption.h", root);
    assert_true(file_exists_and_contains(path, "edgeos_acl_entry_t"));
}

static void test_quickstart_scaffold_flow(void **state) {
    (void)state;
    char root[1024];
    get_repo_root(root, sizeof(root));

    // Run quickstart create-product command directly against repo root
    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/edge-cli create-product %s --base=%s > /dev/null 2>&1",
             root, MANUAL_TEST_PROD, root);
    int rc = system(cmd);
    assert_int_equal(rc, 0);

    // Verify created files
    char path[2048];
    snprintf(path, sizeof(path), "%s/contracts/products/%s.yaml", root, MANUAL_TEST_PROD);
    assert_true(file_exists_and_contains(path, "product: \"manual_quickstart_bess\""));

    snprintf(path, sizeof(path), "%s/cmd/%s/main.c", root, MANUAL_TEST_PROD);
    assert_true(file_exists_and_contains(path, "<edgeos/edgeos.h>"));

    // Run contract linter as instructed in quickstart
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/edge-cli lint-contract %s/contracts/products/%s.yaml "
             "--base=%s --strict > /dev/null 2>&1",
             root, root, MANUAL_TEST_PROD, root);
    rc = system(cmd);
    assert_int_equal(rc, 0);

    // Clean up created sample product files
    snprintf(cmd, sizeof(cmd), "rm -rf %s/cmd/%s %s/contracts/products/%s.yaml", root,
             MANUAL_TEST_PROD, root, MANUAL_TEST_PROD);
    rc = system(cmd);
    (void)rc;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_manual_documents_exist),
        cmocka_unit_test(test_manual_sdk_headers_available),
        cmocka_unit_test(test_quickstart_scaffold_flow),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
