/**
 * @file test_archetype_generator.c
 * @brief AEP-402: CMocka Unit Test for Product Application Archetype Generator (edge-cli create-product)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>

#define TEST_PROD_NAME "sample_archetype_bess"
#define TEST_DRV_NAME  "sample_sunspec_pv"

/* -------------------------------------------------------------------------- */
/* Helper Functions                                                           */
/* -------------------------------------------------------------------------- */

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

static int cleanup_test_archetypes(void **state) {
    (void)state;
    char root[1024];
    get_repo_root(root, sizeof(root));

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "rm -rf %s/cmd/%s %s/contracts/products/%s.yaml %s/build/products/%s %s/modules/edge_io/src/drivers/%s_driver.c %s/modules/edge_io/include/drivers/%s_driver.h %s/contracts/bindings/modbus/%s.yaml > /dev/null 2>&1",
             root, TEST_PROD_NAME, root, TEST_PROD_NAME, root, TEST_PROD_NAME, root, TEST_DRV_NAME, root, TEST_DRV_NAME, root, TEST_DRV_NAME);
    int rc = system(cmd);
    (void)rc;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* CMocka Tests                                                               */
/* -------------------------------------------------------------------------- */

static void test_archetype_product_scaffolding(void **state) {
    (void)state;

    char root[2048];
    get_repo_root(root, sizeof(root));

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "%s/.local/linux-x86_64/bin/edge-cli create-product %s --base=%s > /dev/null 2>&1",
             root, TEST_PROD_NAME, root);
    int rc = system(cmd);
    assert_int_equal(rc, 0);

    // Verify all 5 key files exist with expected content
    char path[4096];
    snprintf(path, sizeof(path), "%s/cmd/%s/prj.conf", root, TEST_PROD_NAME);
    assert_true(file_exists_and_contains(path, "CONFIG_EDGEC_TIER"));

    snprintf(path, sizeof(path), "%s/cmd/%s/manifest.yaml", root, TEST_PROD_NAME);
    assert_true(file_exists_and_contains(path, "telemetry"));

    snprintf(path, sizeof(path), "%s/cmd/%s/CMakeLists.txt", root, TEST_PROD_NAME);
    assert_true(file_exists_and_contains(path, "edge_add_contract"));

    snprintf(path, sizeof(path), "%s/cmd/%s/main.c", root, TEST_PROD_NAME);
    assert_true(file_exists_and_contains(path, "<edgeos/edgeos.h>"));

    snprintf(path, sizeof(path), "%s/contracts/products/%s.yaml", root, TEST_PROD_NAME);
    assert_true(file_exists_and_contains(path, "product:"));
}

static void test_archetype_contract_linter(void **state) {
    (void)state;

    char root[2048];
    get_repo_root(root, sizeof(root));

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "%s/.local/linux-x86_64/bin/edge-cli lint-contract %s/contracts/products/%s.yaml --base=%s --strict > /dev/null 2>&1",
             root, root, TEST_PROD_NAME, root);
    int rc = system(cmd);
    assert_int_equal(rc, 0);
}

static void test_archetype_contract_compilation(void **state) {
    (void)state;

    char root[2048];
    get_repo_root(root, sizeof(root));

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s/build/products/%s/generated && %s/.local/linux-x86_64/bin/contract-compiler --product %s/contracts/products/%s.yaml --local %s/cmd/%s/manifest.yaml --out %s/build/products/%s/generated --base %s > /dev/null 2>&1",
             root, TEST_PROD_NAME, root, root, TEST_PROD_NAME, root, TEST_PROD_NAME, root, TEST_PROD_NAME, root);
    int rc = system(cmd);
    assert_int_equal(rc, 0);

    char path[4096];
    snprintf(path, sizeof(path), "%s/build/products/%s/generated/contracts.c", root, TEST_PROD_NAME);
    assert_true(file_exists_and_contains(path, "g_tag_metadata"));
}

static void test_archetype_driver_scaffolding(void **state) {
    (void)state;

    char root[2048];
    get_repo_root(root, sizeof(root));

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "%s/.local/linux-x86_64/bin/edge-cli create-driver %s --type=tcp --protocol=modbus --base=%s > /dev/null 2>&1",
             root, TEST_DRV_NAME, root);
    int rc = system(cmd);
    assert_int_equal(rc, 0);

    char path[4096];
    snprintf(path, sizeof(path), "%s/modules/edge_io/src/drivers/%s_driver.c", root, TEST_DRV_NAME);
    assert_true(file_exists_and_contains(path, "driver"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_archetype_product_scaffolding),
        cmocka_unit_test(test_archetype_contract_linter),
        cmocka_unit_test(test_archetype_contract_compilation),
        cmocka_unit_test(test_archetype_driver_scaffolding),
    };
    cleanup_test_archetypes(NULL);
    int ret = cmocka_run_group_tests(tests, NULL, cleanup_test_archetypes);
    return ret;
}
