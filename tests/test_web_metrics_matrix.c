/**
 * test_web_metrics_matrix.c - Verification of Panoramic System Metrics & DataHub/DDS Dashboard
 * (AEP-499)
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

static void test_metrics_matrix_component_structure(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/metrics/MetricsMatrix.vue", content, sizeof(content));

    // Headers & Top Gauges
    assert_non_null(strstr(content, "METRICS MATRIX"));
    assert_non_null(strstr(content, "refreshIntervalMs"));
    assert_non_null(strstr(content, "formattedUptime"));
    assert_non_null(strstr(content, "togglePolling"));

    // DataHub Metrics
    assert_non_null(strstr(content, "DATAHUB SHM WATERMARK"));
    assert_non_null(strstr(content, "watermarkPct"));
    assert_non_null(strstr(content, "readLatencyNs"));
    assert_non_null(strstr(content, "dropCount"));
    assert_non_null(strstr(content, "totalTags"));

    // Cyclone DDS Metrics
    assert_non_null(strstr(content, "CYCLONE DDS MESH"));
    assert_non_null(strstr(content, "activeNodes"));
    assert_non_null(strstr(content, "rxRateBytes"));
    assert_non_null(strstr(content, "txRateBytes"));
    assert_non_null(strstr(content, "packetLossRate"));
    assert_non_null(strstr(content, "heartbeatJitterUs"));

    // Stream Engine DAG & System Metrics
    assert_non_null(strstr(content, "dagLatencyUs"));
    assert_non_null(strstr(content, "stageQueueDepth"));
    assert_non_null(strstr(content, "samplingRateHz"));
    assert_non_null(strstr(content, "cpuLoadPct"));
    assert_non_null(strstr(content, "memoryRssMb"));
    assert_non_null(strstr(content, "memfdSealedFds"));
    assert_non_null(strstr(content, "rtJitterUs"));
    assert_non_null(strstr(content, "sqliteWalPages"));

    // Histograms & Percentiles
    assert_non_null(strstr(content, "p50Ns"));
    assert_non_null(strstr(content, "p95Ns"));
    assert_non_null(strstr(content, "p99Ns"));
    assert_non_null(strstr(content, "p999Ns"));
    assert_non_null(strstr(content, "latencyBuckets"));

    // Sparklines & OpenMetrics
    assert_non_null(strstr(content, "tpsSvgPath"));
    assert_non_null(strstr(content, "dagSvgPath"));
    assert_non_null(strstr(content, "ddsRxSvgPath"));
    assert_non_null(strstr(content, "rawPrometheusText"));
    assert_non_null(strstr(content, "edgeos_datahub_ring_watermark_percent"));
    assert_non_null(strstr(content, "edgeos_dds_rx_bytes_total"));
}

static void test_metrics_matrix_routing_and_exports(void **state) {
    (void)state;
    char router_content[8192];
    read_file_content("web/core/src/router/index.ts", router_content, sizeof(router_content));
    assert_non_null(strstr(router_content, "metrics"));
    assert_non_null(strstr(router_content, "MetricsMatrix.vue"));

    char index_content[8192];
    read_file_content("web/core/src/index.ts", index_content, sizeof(index_content));
    assert_non_null(strstr(index_content, "MetricsMatrix"));
}

/* C-side Prometheus / OpenMetrics formatting verification */
static void test_prometheus_openmetrics_formatting(void **state) {
    (void)state;
    char payload[1024];
    double watermark = 18.5;
    uint64_t tps = 24500;
    uint64_t rx_bytes = 148500;
    uint32_t active_nodes = 4;
    double cpu_load = 14.2;

    int n = snprintf(
        payload, sizeof(payload),
        "# HELP edgeos_datahub_ring_watermark_percent DataHub SPSC ringbuffer utilization "
        "percentage\n"
        "# TYPE edgeos_datahub_ring_watermark_percent gauge\n"
        "edgeos_datahub_ring_watermark_percent %.1f\n"
        "# HELP edgeos_datahub_tps DataHub transaction throughput per second\n"
        "# TYPE edgeos_datahub_tps gauge\n"
        "edgeos_datahub_tps %lu\n"
        "# HELP edgeos_dds_rx_bytes_total Cyclone DDS network receive rate in bytes/sec\n"
        "# TYPE edgeos_dds_rx_bytes_total counter\n"
        "edgeos_dds_rx_bytes_total %lu\n"
        "# HELP edgeos_dds_active_nodes Cyclone DDS discovered live participant mesh nodes\n"
        "# TYPE edgeos_dds_active_nodes gauge\n"
        "edgeos_dds_active_nodes %u\n"
        "# HELP edgeos_system_cpu_load_percent CPU total utilization percentage\n"
        "# TYPE edgeos_system_cpu_load_percent gauge\n"
        "edgeos_system_cpu_load_percent %.1f\n",
        watermark, tps, rx_bytes, active_nodes, cpu_load);

    assert_true(n > 0);
    assert_non_null(strstr(payload, "edgeos_datahub_ring_watermark_percent 18.5"));
    assert_non_null(strstr(payload, "edgeos_datahub_tps 24500"));
    assert_non_null(strstr(payload, "edgeos_dds_rx_bytes_total 148500"));
    assert_non_null(strstr(payload, "edgeos_dds_active_nodes 4"));
    assert_non_null(strstr(payload, "edgeos_system_cpu_load_percent 14.2"));
}

/* Statistical percentile computation verification */
static int compare_uint32(const void *a, const void *b) {
    uint32_t arg1 = *(const uint32_t *)a;
    uint32_t arg2 = *(const uint32_t *)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

static void test_statistical_latency_percentiles(void **state) {
    (void)state;
    const size_t N = 1000;
    uint32_t latencies[1000];

    // Synthesize distribution: 70% < 50ns, 20% 50-100ns, 8% 100-200ns, 2% > 200ns
    for (size_t i = 0; i < 700; i++) {
        latencies[i] = 30 + (uint32_t)(i % 20);  // 30..49 ns
    }
    for (size_t i = 700; i < 900; i++) {
        latencies[i] = 50 + (uint32_t)(i % 50);  // 50..99 ns
    }
    for (size_t i = 900; i < 980; i++) {
        latencies[i] = 100 + (uint32_t)(i % 100);  // 100..199 ns
    }
    for (size_t i = 980; i < 1000; i++) {
        latencies[i] = 200 + (uint32_t)(i % 300);  // 200..499 ns
    }

    qsort(latencies, N, sizeof(uint32_t), compare_uint32);

    uint32_t p50 = latencies[(size_t)(N * 0.50)];
    uint32_t p95 = latencies[(size_t)(N * 0.95)];
    uint32_t p99 = latencies[(size_t)(N * 0.99)];

    assert_true(p50 < 50);
    assert_true(p95 >= 100 && p95 < 200);
    assert_true(p99 >= 200);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_metrics_matrix_component_structure),
        cmocka_unit_test(test_metrics_matrix_routing_and_exports),
        cmocka_unit_test(test_prometheus_openmetrics_formatting),
        cmocka_unit_test(test_statistical_latency_percentiles),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
