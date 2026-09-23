/**
 * @file test_compiler_dx_diagnostics.c
 * @brief AEP-404: CMocka Unit Test for Contract Compiler DX Diagnostic Engine & Typo Tolerance
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

#define TMP_DIAG_DIR "/tmp/edge_diag_test"

/* -------------------------------------------------------------------------- */
/* Helper Functions                                                           */
/* -------------------------------------------------------------------------- */

static void get_repo_root(char *out, size_t max_len) {
    char cwd[2048];
    if (!getcwd(cwd, sizeof(cwd))) {
        snprintf(out, max_len, ".");
        return;
    }
    if (access(".local/linux-x86_64/bin/contract-compiler", X_OK) == 0) {
        snprintf(out, max_len, "%s", cwd);
    } else if (access("../.local/linux-x86_64/bin/contract-compiler", X_OK) == 0) {
        snprintf(out, max_len, "%s/..", cwd);
    } else if (access("../../.local/linux-x86_64/bin/contract-compiler", X_OK) == 0) {
        snprintf(out, max_len, "%s/../..", cwd);
    } else {
        snprintf(out, max_len, ".");
    }
}

static bool output_contains(const char *log_path, const char *keyword) {
    FILE *fp = fopen(log_path, "r");
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

static int cleanup_diag_sandbox(void **state) {
    (void)state;
    int rc = system("rm -rf " TMP_DIAG_DIR " > /dev/null 2>&1");
    (void)rc;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* CMocka Tests                                                               */
/* -------------------------------------------------------------------------- */

static void test_compiler_typo_diagnostics(void **state) {
    (void)state;

    char root[2048];
    get_repo_root(root, sizeof(root));

    int rc = system("mkdir -p " TMP_DIAG_DIR);
    (void)rc;

    // Create YAML with typo 'flot'
    const char *typo_yaml = TMP_DIAG_DIR "/typo_contract.yaml";
    FILE *fp = fopen(typo_yaml, "w");
    assert_non_null(fp);
    fprintf(fp,
            "product: \"typo_test\"\n"
            "tags:\n"
            "  - id: 1001\n"
            "    name: \"VOLTAGE_PHASE_A\"\n"
            "    type: \"flot\"\n");
    fclose(fp);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/contract-compiler --product %s --out " TMP_DIAG_DIR
             "/out --base %s > " TMP_DIAG_DIR "/typo.log 2>&1",
             root, typo_yaml, root);
    int exit_code = system(cmd);
    assert_int_not_equal(exit_code, 0);

    assert_true(output_contains(TMP_DIAG_DIR "/typo.log", "Did you mean 'float'?"));
}

static void test_compiler_domain_range_diagnostics(void **state) {
    (void)state;

    char root[2048];
    get_repo_root(root, sizeof(root));

    // Create YAML with out-of-range power tag
    const char *domain_yaml = TMP_DIAG_DIR "/domain_contract.yaml";
    FILE *fp = fopen(domain_yaml, "w");
    assert_non_null(fp);
    fprintf(fp,
            "product: \"domain_test\"\n"
            "tags:\n"
            "  - id: 50\n"
            "    name: \"VOLTAGE_OUT_OF_RANGE\"\n"
            "    type: \"float32\"\n");
    fclose(fp);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/contract-compiler --product %s --out " TMP_DIAG_DIR
             "/out --base %s > " TMP_DIAG_DIR "/domain.log 2>&1",
             root, domain_yaml, root);
    int exit_code = system(cmd);
    assert_int_not_equal(exit_code, 0);

    assert_true(output_contains(TMP_DIAG_DIR "/domain.log", "Power Domain range 1000..1999"));
}

static void test_compiler_duplicate_tag_diagnostics(void **state) {
    (void)state;

    char root[2048];
    get_repo_root(root, sizeof(root));

    // Create YAML with duplicate tag ID
    const char *dup_yaml = TMP_DIAG_DIR "/dup_contract.yaml";
    FILE *fp = fopen(dup_yaml, "w");
    assert_non_null(fp);
    fprintf(fp,
            "product: \"dup_test\"\n"
            "tags:\n"
            "  - id: 1001\n"
            "    name: \"VOLTAGE_PHASE_A\"\n"
            "    type: \"float32\"\n"
            "  - id: 1001\n"
            "    name: \"VOLTAGE_PHASE_B\"\n"
            "    type: \"float32\"\n");
    fclose(fp);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/contract-compiler --product %s --out " TMP_DIAG_DIR
             "/out --base %s > " TMP_DIAG_DIR "/dup.log 2>&1",
             root, dup_yaml, root);
    int exit_code = system(cmd);
    assert_int_not_equal(exit_code, 0);

    assert_true(output_contains(TMP_DIAG_DIR "/dup.log", "Duplicate tag ID 1001"));
}

static void test_compiler_valid_compilation(void **state) {
    (void)state;

    char root[2048];
    get_repo_root(root, sizeof(root));

    // Create valid YAML
    const char *valid_yaml = TMP_DIAG_DIR "/valid_contract.yaml";
    FILE *fp = fopen(valid_yaml, "w");
    assert_non_null(fp);
    fprintf(fp,
            "product: \"valid_test\"\n"
            "tags:\n"
            "  - id: 1001\n"
            "    name: \"VOLTAGE_PHASE_A\"\n"
            "    type: \"float32\"\n"
            "    unit: \"V\"\n");
    fclose(fp);

    char cmd[8192];
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/contract-compiler --product %s --out " TMP_DIAG_DIR
             "/out_valid --base %s > " TMP_DIAG_DIR "/valid.log 2>&1",
             root, valid_yaml, root);
    int exit_code = system(cmd);
    assert_int_equal(exit_code, 0);

    assert_true(output_contains(TMP_DIAG_DIR "/out_valid/contracts.c", "g_tag_metadata"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_compiler_typo_diagnostics),
        cmocka_unit_test(test_compiler_domain_range_diagnostics),
        cmocka_unit_test(test_compiler_duplicate_tag_diagnostics),
        cmocka_unit_test(test_compiler_valid_compilation),
    };
    cleanup_diag_sandbox(NULL);
    int ret = cmocka_run_group_tests(tests, NULL, cleanup_diag_sandbox);
    return ret;
}
