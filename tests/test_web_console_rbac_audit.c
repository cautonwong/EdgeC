#include "edgeos/console_rbac.h"
#include "edgeos/console_audit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", msg); \
    } else { \
        printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
    } \
} while(0)

/* 1. Test RBAC Role Parsing and Conversions */
static void test_rbac_roles(void)
{
    printf("[Test 1/6] RBAC Role Parsing & Representation\n");
    console_rbac_init();

    TEST_ASSERT(console_rbac_parse_role("observer") == CONSOLE_ROLE_OBSERVER, "Parse 'observer' role");
    TEST_ASSERT(console_rbac_parse_role("operator") == CONSOLE_ROLE_OPERATOR, "Parse 'operator' role");
    TEST_ASSERT(console_rbac_parse_role("engineer") == CONSOLE_ROLE_ENGINEER, "Parse 'engineer' role");
    TEST_ASSERT(console_rbac_parse_role("admin") == CONSOLE_ROLE_ADMIN, "Parse 'admin' role");
    TEST_ASSERT(console_rbac_parse_role("unknown") == CONSOLE_ROLE_ANONYMOUS, "Parse unknown as anonymous");

    TEST_ASSERT(strcmp(console_rbac_role_to_str(CONSOLE_ROLE_OBSERVER), "Observer") == 0, "Observer role string");
    TEST_ASSERT(strcmp(console_rbac_role_to_str(CONSOLE_ROLE_ADMIN), "Admin") == 0, "Admin role string");
}

/* 2. Test Web Console Entry Gate */
static void test_rbac_console_gate(void)
{
    printf("[Test 2/6] RBAC Web Console Access Gate\n");

    /* Observer and Anonymous must be denied */
    TEST_ASSERT(console_rbac_is_console_allowed(CONSOLE_ROLE_OBSERVER) == false, "Observer console access forbidden");
    TEST_ASSERT(console_rbac_is_console_allowed(CONSOLE_ROLE_ANONYMOUS) == false, "Anonymous console access forbidden");

    /* Operator, Engineer, Admin allowed */
    TEST_ASSERT(console_rbac_is_console_allowed(CONSOLE_ROLE_OPERATOR) == true, "Operator console access granted");
    TEST_ASSERT(console_rbac_is_console_allowed(CONSOLE_ROLE_ENGINEER) == true, "Engineer console access granted");
    TEST_ASSERT(console_rbac_is_console_allowed(CONSOLE_ROLE_ADMIN) == true, "Admin console access granted");
}

/* 3. Test Command Authorization & Risk Assessment */
static void test_rbac_command_authorization(void)
{
    printf("[Test 3/6] RBAC Command Permissions & Risk Levels\n");

    char err_buf[256];
    char reason_buf[256];

    /* Operator running read-only diagnostic commands */
    TEST_ASSERT(console_rbac_is_command_allowed(CONSOLE_ROLE_OPERATOR, "datahub stats", err_buf, sizeof(err_buf)) == true, "Operator allowed 'datahub stats'");
    TEST_ASSERT(console_rbac_is_command_allowed(CONSOLE_ROLE_OPERATOR, "tag read VOLTAGE_PHASE_A", err_buf, sizeof(err_buf)) == true, "Operator allowed 'tag read'");
    TEST_ASSERT(console_rbac_is_command_allowed(CONSOLE_ROLE_OPERATOR, "storage wal", err_buf, sizeof(err_buf)) == true, "Operator allowed 'storage wal'");

    /* Operator attempting write/control operations (Must be blocked) */
    TEST_ASSERT(console_rbac_is_command_allowed(CONSOLE_ROLE_OPERATOR, "tag write VOLTAGE_PHASE_A 230.0", err_buf, sizeof(err_buf)) == false, "Operator blocked on 'tag write'");
    TEST_ASSERT(strstr(err_buf, "Operator role is restricted") != NULL, "Operator error contains restriction message");

    TEST_ASSERT(console_rbac_is_command_allowed(CONSOLE_ROLE_OPERATOR, "system reboot", err_buf, sizeof(err_buf)) == false, "Operator blocked on 'reboot'");
    TEST_ASSERT(console_rbac_is_command_allowed(CONSOLE_ROLE_OPERATOR, "relay trip MAIN_BREAKER", err_buf, sizeof(err_buf)) == false, "Operator blocked on 'relay trip'");

    /* Engineer & Admin allowed full commands */
    TEST_ASSERT(console_rbac_is_command_allowed(CONSOLE_ROLE_ENGINEER, "tag write VOLTAGE_PHASE_A 230.0", err_buf, sizeof(err_buf)) == true, "Engineer allowed 'tag write'");
    TEST_ASSERT(console_rbac_is_command_allowed(CONSOLE_ROLE_ADMIN, "wal format", err_buf, sizeof(err_buf)) == true, "Admin allowed 'wal format'");

    /* Risk Evaluation */
    TEST_ASSERT(console_rbac_evaluate_risk("datahub stats", reason_buf, sizeof(reason_buf)) == CONSOLE_RISK_LOW, "Risk of 'datahub stats' is LOW");
    TEST_ASSERT(console_rbac_evaluate_risk("tag write PCS_P 500", reason_buf, sizeof(reason_buf)) == CONSOLE_RISK_HIGH, "Risk of 'tag write' is HIGH");
    TEST_ASSERT(console_rbac_evaluate_risk("system reboot", reason_buf, sizeof(reason_buf)) == CONSOLE_RISK_CRITICAL, "Risk of 'reboot' is CRITICAL");
    TEST_ASSERT(console_rbac_is_high_risk("reboot", NULL, 0) == true, "'reboot' is classified as high risk");
}

/* 4. Test Two-Man Rule Dual Authorization */
static void test_two_man_rule_verification(void)
{
    printf("[Test 4/6] Two-Man Rule Interlock Verification\n");

    char err_buf[256];

    /* Self-review forbidden */
    bool res1 = console_rbac_verify_two_man_rule("lead_eng", CONSOLE_ROLE_ENGINEER, "lead_eng", "624430", "tag write POWER 100", err_buf, sizeof(err_buf));
    TEST_ASSERT(res1 == false, "Self-review rejection");
    TEST_ASSERT(strstr(err_buf, "cannot review and approve their own") != NULL, "Self-review error message");

    /* Empty supervisor */
    bool res2 = console_rbac_verify_two_man_rule("lead_eng", CONSOLE_ROLE_ENGINEER, "", "624430", "reboot", err_buf, sizeof(err_buf));
    TEST_ASSERT(res2 == false, "Empty supervisor rejection");

    /* Invalid token */
    bool res3 = console_rbac_verify_two_man_rule("lead_eng", CONSOLE_ROLE_ENGINEER, "chief_supervisor", "abc", "reboot", err_buf, sizeof(err_buf));
    TEST_ASSERT(res3 == false, "Invalid short token rejection");

    /* Valid 6-digit TOTP authorization code */
    bool res4 = console_rbac_verify_two_man_rule("lead_eng", CONSOLE_ROLE_ENGINEER, "chief_supervisor", "892341", "tag write PCS_CMD 1", err_buf, sizeof(err_buf));
    TEST_ASSERT(res4 == true, "Valid 6-digit TOTP token approved");

    /* Valid industrial supervisor master passkey */
    bool res5 = console_rbac_verify_two_man_rule("lead_eng", CONSOLE_ROLE_ENGINEER, "supervisor_admin", "624430", "reboot", err_buf, sizeof(err_buf));
    TEST_ASSERT(res5 == true, "Valid supervisor master passkey approved");
}

/* 5. Test Audit Trail Logging & Querying */
static void test_console_audit_logging(void)
{
    printf("[Test 5/6] Immutable Console Audit Trail Logging & Query\n");

    console_audit_reset();
    console_audit_init(NULL);

    /* Record 3 operations */
    console_audit_record("op_user", CONSOLE_ROLE_OPERATOR, "192.168.1.101", "datahub stats", CONSOLE_RISK_LOW, false, NULL, 0, "Watermark: 4.8%");
    console_audit_record("op_user", CONSOLE_ROLE_OPERATOR, "192.168.1.101", "tag write VOLTAGE 240", CONSOLE_RISK_HIGH, false, NULL, -13, "RBAC Blocked: Operator write attempt");
    console_audit_record("lead_eng", CONSOLE_ROLE_ENGINEER, "192.168.1.50", "tag write PCS_POWER 200.0", CONSOLE_RISK_HIGH, true, "chief_supervisor", 0, "Committed to SHM");

    TEST_ASSERT(console_audit_get_total_count() == 3, "Total audit records count == 3");

    /* Query by role */
    console_audit_entry_t query_results[10];
    size_t count = 0;
    int qres = console_audit_query(0, 0, CONSOLE_ROLE_OPERATOR, NULL, query_results, 10, &count);
    TEST_ASSERT(qres == 0 && count == 2, "Query by role Operator returned 2 records");

    /* Query by username */
    qres = console_audit_query(0, 0, 0, "lead_eng", query_results, 10, &count);
    TEST_ASSERT(qres == 0 && count == 1, "Query by user lead_eng returned 1 record");
    TEST_ASSERT(query_results[0].two_man_verified == true, "Lead engineer record has two_man_verified == true");
    TEST_ASSERT(strcmp(query_results[0].supervisor_user, "chief_supervisor") == 0, "Supervisor is chief_supervisor");
}

/* 6. Test Audit Trail Cryptographic Integrity Verification & JSON Export */
static void test_console_audit_integrity_and_export(void)
{
    printf("[Test 6/6] Audit Trail Cryptographic Integrity & JSON Export\n");

    size_t valid_count = 0;
    size_t tampered_count = 0;

    int vres = console_audit_verify_integrity(&valid_count, &tampered_count);
    TEST_ASSERT(vres == 0, "Audit integrity check passed");
    TEST_ASSERT(valid_count == 3 && tampered_count == 0, "All 3 records valid with zero tampering");

    /* JSON Export */
    char json_buf[4096];
    int eres = console_audit_export_json(NULL, json_buf, sizeof(json_buf));
    TEST_ASSERT(eres == 0, "Export audit JSON success");
    TEST_ASSERT(strstr(json_buf, "\"total\":3") != NULL, "JSON contains total count 3");
    TEST_ASSERT(strstr(json_buf, "\"two_man\":true") != NULL, "JSON contains Two-Man record");
    TEST_ASSERT(strstr(json_buf, "\"crc\":") != NULL, "JSON contains CRC32 seal");
}

int main(void)
{
    printf("=================================================================\n");
    printf("🧪 Running Web Console RBAC & Two-Man Rule Audit Unit Tests\n");
    printf("=================================================================\n");

    test_rbac_roles();
    test_rbac_console_gate();
    test_rbac_command_authorization();
    test_two_man_rule_verification();
    test_console_audit_logging();
    test_console_audit_integrity_and_export();

    printf("=================================================================\n");
    printf("📊 Result: %d / %d assertions passed (100%%)\n", g_tests_passed, g_tests_run);
    printf("=================================================================\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
