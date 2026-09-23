/**
 * @file test_cli_migrate.c
 * @brief Integration tests for edge-cli migrate wizard tool (AEP-405)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SANDBOX_DIR "/tmp/test_edge_migrate_sandbox"

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

static int setup_sandbox(void **state) {
    (void)state;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s && mkdir -p %s", SANDBOX_DIR, SANDBOX_DIR);
    int rc = system(cmd);
    (void)rc;

    // Create legacy C source file
    char c_path[512];
    snprintf(c_path, sizeof(c_path), "%s/legacy_inverter.c", SANDBOX_DIR);
    FILE *fc = fopen(c_path, "w");
    if (fc) {
        fputs(
            "#include \"edgeos.h\"\n"
            "#include \"datahub.h\"\n"
            "#include \"edge_driver.h\"\n"
            "#include \"edge_anti_corruption.h\"\n"
            "\n"
            "int custom_driver_run(void) {\n"
            "    return 0;\n"
            "}\n",
            fc);
        fclose(fc);
    }

    // Create legacy contract YAML file
    char yaml_path[512];
    snprintf(yaml_path, sizeof(yaml_path), "%s/legacy_contract.yaml", SANDBOX_DIR);
    FILE *fy = fopen(yaml_path, "w");
    if (fy) {
        fputs(
            "product: \"legacy_grid\"\n"
            "include: \"contracts/domains/power.yaml\"\n"
            "tags:\n"
            "  - id: 1001\n"
            "    name: \"VOLTAGE_A\"\n"
            "    type: \"float_32\"\n"
            "    unit: \"Volts\"\n"
            "  - id: 1002\n"
            "    name: \"CURRENT_A\"\n"
            "    type: \"float_64\"\n"
            "    unit: \"Amperes\"\n",
            fy);
        fclose(fy);
    }

    return 0;
}

static int teardown_sandbox(void **state) {
    (void)state;
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", SANDBOX_DIR);
    int rc = system(cmd);
    (void)rc;
    return 0;
}

static void test_migrate_dry_run_detection(void **state) {
    (void)state;
    char root[2048];
    get_repo_root(root, sizeof(root));

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/edge-cli migrate --dir=%s > %s/dry_run.log 2>&1", root,
             SANDBOX_DIR, SANDBOX_DIR);
    int rc = system(cmd);
    assert_int_equal(rc, 0);

    // Read log and verify detection
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/dry_run.log", SANDBOX_DIR);
    FILE *fp = fopen(log_path, "r");
    assert_non_null(fp);

    char buf[4096] = {0};
    size_t nr = fread(buf, 1, sizeof(buf) - 1, fp);
    (void)nr;
    fclose(fp);

    assert_non_null(strstr(buf, "EdgeOS Migration Scanner Report"));
    assert_non_null(strstr(buf, "⚠️ [DETECTED]"));
    assert_non_null(strstr(buf, "Run 'edge-cli migrate --fix'"));

    // Verify files on disk were NOT modified
    char c_path[512];
    snprintf(c_path, sizeof(c_path), "%s/legacy_inverter.c", SANDBOX_DIR);
    FILE *fc = fopen(c_path, "r");
    assert_non_null(fc);
    char c_buf[2048] = {0};
    size_t nrc = fread(c_buf, 1, sizeof(c_buf) - 1, fc);
    (void)nrc;
    fclose(fc);
    assert_non_null(strstr(c_buf, "#include \"edgeos.h\""));
}

static void test_migrate_auto_fix_execution(void **state) {
    (void)state;
    char root[2048];
    get_repo_root(root, sizeof(root));

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/edge-cli migrate --dir=%s --fix > %s/fix.log 2>&1", root,
             SANDBOX_DIR, SANDBOX_DIR);
    int rc = system(cmd);
    assert_int_equal(rc, 0);

    // Read fix log
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/fix.log", SANDBOX_DIR);
    FILE *fp = fopen(log_path, "r");
    assert_non_null(fp);

    char buf[4096] = {0};
    size_t nr = fread(buf, 1, sizeof(buf) - 1, fp);
    (void)nr;
    fclose(fp);

    assert_non_null(strstr(buf, "✅ [MIGRATED]"));
    assert_non_null(strstr(buf, "Successfully migrated"));

    // Verify C file on disk is properly migrated to public headers
    char c_path[512];
    snprintf(c_path, sizeof(c_path), "%s/legacy_inverter.c", SANDBOX_DIR);
    FILE *fc = fopen(c_path, "r");
    assert_non_null(fc);
    char c_buf[2048] = {0};
    size_t nrc = fread(c_buf, 1, sizeof(c_buf) - 1, fc);
    (void)nrc;
    fclose(fc);

    assert_non_null(strstr(c_buf, "#include <edgeos/edgeos.h>"));
    assert_non_null(strstr(c_buf, "#include <edgeos/datahub.h>"));
    assert_non_null(strstr(c_buf, "#include <edgeos/driver.h>"));
    assert_non_null(strstr(c_buf, "#include <edgeos/anti_corruption.h>"));
    assert_null(strstr(c_buf, "#include \"edgeos.h\""));

    // Verify YAML file on disk is properly migrated
    char yaml_path[512];
    snprintf(yaml_path, sizeof(yaml_path), "%s/legacy_contract.yaml", SANDBOX_DIR);
    FILE *fy = fopen(yaml_path, "r");
    assert_non_null(fy);
    char yaml_buf[2048] = {0};
    size_t nry = fread(yaml_buf, 1, sizeof(yaml_buf) - 1, fy);
    (void)nry;
    fclose(fy);

    assert_non_null(strstr(yaml_buf, "includes:"));
    assert_non_null(strstr(yaml_buf, "type: \"float32\""));
    assert_non_null(strstr(yaml_buf, "type: \"float64\""));
    assert_non_null(strstr(yaml_buf, "unit: \"V\""));
    assert_non_null(strstr(yaml_buf, "unit: \"A\""));
    assert_null(strstr(yaml_buf, "include: \"contracts/domains/power.yaml\""));
}

static void test_migrate_idempotent_clean_run(void **state) {
    (void)state;
    char root[2048];
    get_repo_root(root, sizeof(root));

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
             "%s/.local/linux-x86_64/bin/edge-cli migrate --dir=%s > %s/clean.log 2>&1", root,
             SANDBOX_DIR, SANDBOX_DIR);
    int rc = system(cmd);
    assert_int_equal(rc, 0);

    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/clean.log", SANDBOX_DIR);
    FILE *fp = fopen(log_path, "r");
    assert_non_null(fp);

    char buf[4096] = {0};
    size_t nr = fread(buf, 1, sizeof(buf) - 1, fp);
    (void)nr;
    fclose(fp);

    assert_non_null(
        strstr(buf, "✅ Clean! No deprecated SDK headers or legacy contract schemas detected."));
    assert_non_null(strstr(buf, "Issues found : 0"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_migrate_dry_run_detection, setup_sandbox, NULL),
        cmocka_unit_test_setup_teardown(test_migrate_auto_fix_execution, NULL, NULL),
        cmocka_unit_test_setup_teardown(test_migrate_idempotent_clean_run, NULL, teardown_sandbox),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
