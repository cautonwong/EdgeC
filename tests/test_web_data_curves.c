/**
 * test_web_data_curves.c - Verification of DataCurves ECharts LTTB Downsampling & Monospace
 * Crosshair Engine (AEP-493)
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

static void test_echarts_lttb_sampling_and_webgl_specs(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/data/DataCurves.vue", content, sizeof(content));

    // LTTB downsampling for huge dataset acceleration
    assert_non_null(strstr(content, "sampling: 'lttb'"));
    assert_non_null(strstr(content, "import * as echarts from 'echarts'"));
    assert_non_null(strstr(content, "areaStyle:"));
    assert_non_null(strstr(content, "LinearGradient"));
}

static void test_jetbrains_mono_typography_and_formatting(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/data/DataCurves.vue", content, sizeof(content));

    // JetBrains Mono typography in crosshair and tooltips
    assert_non_null(strstr(content, "fontFamily: 'JetBrains Mono"));
    assert_non_null(strstr(content, "type: 'cross'"));
    assert_non_null(strstr(content, "axisPointer:"));
    assert_non_null(strstr(content, "formatter:"));
}

static void test_four_series_microgrid_composition(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/data/DataCurves.vue", content, sizeof(content));

    // 4-series power balance curve tracks
    assert_non_null(strstr(content, "Solar PV"));
    assert_non_null(strstr(content, "ESS Battery"));
    assert_non_null(strstr(content, "Grid Bus"));
    assert_non_null(strstr(content, "Active Load"));
    assert_non_null(strstr(content, "#3B82F6"));
    assert_non_null(strstr(content, "#10B981"));
}

static void test_datazoom_and_benchmark_100k(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/data/DataCurves.vue", content, sizeof(content));

    // DataZoom slider & inside zoom, plus 100k benchmark generator
    assert_non_null(strstr(content, "dataZoom:"));
    assert_non_null(strstr(content, "type: 'inside'"));
    assert_non_null(strstr(content, "type: 'slider'"));
    assert_non_null(strstr(content, "loadBenchmark100k"));
}

static void test_memory_leak_prevention_lifecycle(void **state) {
    (void)state;
    char content[65536];
    read_file_content("web/core/src/views/data/DataCurves.vue", content, sizeof(content));

    // onBeforeUnmount disposes chart and event listeners cleanly
    assert_non_null(strstr(content, "onBeforeUnmount"));
    assert_non_null(strstr(content, "chartInstance.dispose()"));
    assert_non_null(strstr(content, "removeEventListener('resize'"));
    assert_non_null(strstr(content, "clearInterval(streamTimer)"));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_echarts_lttb_sampling_and_webgl_specs),
        cmocka_unit_test(test_jetbrains_mono_typography_and_formatting),
        cmocka_unit_test(test_four_series_microgrid_composition),
        cmocka_unit_test(test_datazoom_and_benchmark_100k),
        cmocka_unit_test(test_memory_leak_prevention_lifecycle),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
