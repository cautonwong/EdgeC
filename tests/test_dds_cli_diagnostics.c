/**
 * @file test_dds_cli_diagnostics.c
 * @brief Empirical Verification for edge-cli dds Observability & PCAP Diagnostics Suite (AEP-454)
 */

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmocka.h>

static void find_edge_root(char *out, size_t max_len) {
    char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) {
        snprintf(out, max_len, ".");
        return;
    }
    if (access("tools/edge-cli/cmd/main.go", F_OK) == 0) {
        snprintf(out, max_len, "%s", cwd);
    } else if (access("../tools/edge-cli/cmd/main.go", F_OK) == 0) {
        snprintf(out, max_len, "%s/..", cwd);
    } else if (access("../../tools/edge-cli/cmd/main.go", F_OK) == 0) {
        snprintf(out, max_len, "%s/../..", cwd);
    } else {
        snprintf(out, max_len, ".");
    }
}

static void test_edge_cli_dds_discovery(void **state) {
    (void)state;

    char root[1024];
    find_edge_root(root, sizeof(root));

    // 1. 测试 Table 格式输出
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "cd %s/tools/edge-cli && go run ./cmd dds discovery --format=table", root);
    FILE *fp = popen(cmd, "r");
    assert_non_null(fp);

    char buffer[4096] = {0};
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    assert_true(bytes_read > 0);
    int status = pclose(fp);
    assert_int_equal(status, 0);

    assert_non_null(strstr(buffer, "Cyclone DDS Distributed Participants"));
    assert_non_null(strstr(buffer, "IEMS_PCS_Controller"));
    assert_non_null(strstr(buffer, "DCU_Feeder_Gateway"));

    // 2. 测试 JSON 格式输出
    snprintf(cmd, sizeof(cmd), "cd %s/tools/edge-cli && go run ./cmd dds discovery --format=json", root);
    fp = popen(cmd, "r");
    assert_non_null(fp);

    memset(buffer, 0, sizeof(buffer));
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    assert_true(bytes_read > 0);
    status = pclose(fp);
    assert_int_equal(status, 0);

    assert_non_null(strstr(buffer, "\"name\": \"IEMS_PCS_Controller\""));
    assert_non_null(strstr(buffer, "\"partition\": \"Bay/Line1\""));
}

static void test_edge_cli_dds_stats(void **state) {
    (void)state;

    char root[1024];
    find_edge_root(root, sizeof(root));

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "cd %s/tools/edge-cli && go run ./cmd dds stats", root);
    FILE *fp = popen(cmd, "r");
    assert_non_null(fp);

    char buffer[4096] = {0};
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    assert_true(bytes_read > 0);
    int status = pclose(fp);
    assert_int_equal(status, 0);

    assert_non_null(strstr(buffer, "Cyclone DDS Real-Time Performance"));
    assert_non_null(strstr(buffer, "Packets TX"));
    assert_non_null(strstr(buffer, "Fast-Track Zero-Copy Lat"));
}

static void test_edge_cli_dds_dump(void **state) {
    (void)state;

    char root[1024];
    find_edge_root(root, sizeof(root));

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "cd %s/tools/edge-cli && go run ./cmd dds dump --topic=edgeos/telemetry/yc --count=3", root);
    FILE *fp = popen(cmd, "r");
    assert_non_null(fp);

    char buffer[4096] = {0};
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    assert_true(bytes_read > 0);
    int status = pclose(fp);
    assert_int_equal(status, 0);

    assert_non_null(strstr(buffer, "Live RTPS CDR Packet Snooper"));
    assert_non_null(strstr(buffer, "Topic: edgeos/telemetry/yc"));
}

static void test_edge_cli_dds_pcap(void **state) {
    (void)state;

    char root[1024];
    find_edge_root(root, sizeof(root));

    const char *pcap_path = "/tmp/test_cli_trace.pcap";
    unlink(pcap_path);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "cd %s/tools/edge-cli && go run ./cmd dds pcap --output=%s --count=10", root, pcap_path);
    int status = system(cmd);
    assert_int_equal(status, 0);

    struct stat st;
    int rc = stat(pcap_path, &st);
    assert_int_equal(rc, 0);
    assert_true(st.st_size > 100);

    // 验证 PCAP Magic 0xA1B2C3D4
    FILE *fp = fopen(pcap_path, "rb");
    assert_non_null(fp);
    uint32_t magic = 0;
    size_t n = fread(&magic, sizeof(uint32_t), 1, fp);
    assert_int_equal(n, 1);
    fclose(fp);
    assert_int_equal(magic, 0xA1B2C3D4);

    unlink(pcap_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_edge_cli_dds_discovery),
        cmocka_unit_test(test_edge_cli_dds_stats),
        cmocka_unit_test(test_edge_cli_dds_dump),
        cmocka_unit_test(test_edge_cli_dds_pcap),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
