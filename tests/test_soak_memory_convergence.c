/**
 * @file test_soak_memory_convergence.c
 * @brief Soak Test & Memory Fragmentation / RSS Convergence Verification (AEP-411 / Issue #133)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#include "datahub.h"

static long get_current_rss_kb(void) {
    FILE *fp = fopen("/proc/self/statm", "r");
    if (!fp) return 0;
    long total_pages = 0, resident_pages = 0;
    if (fscanf(fp, "%ld %ld", &total_pages, &resident_pages) != 2) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    long page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
    return resident_pages * page_size_kb;
}

static void test_soak_memory_rss_convergence(void) {
    printf("[TEST] 100,000 High-Frequency Message Push & RSS Stability...\n");

    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    int rc = datahub_init(&hub, loop, "soak_hub", 128, 10000, NULL, 0, NULL, NULL, NULL);
    assert(rc == 0);

    // Warm-up 5,000 iterations to allow initialization allocations to stabilize
    pipe_msg_t msg;
    for (int i = 0; i < 5000; i++) {
        pipe_msg_init_scalar(&msg, 1001, (double)i * 0.1, 1000 + (uint64_t)i);
        datahub_write_shadow(&hub, &msg);
    }

    long baseline_rss = get_current_rss_kb();
    printf("  -> Baseline RSS after warm-up: %ld KB\n", baseline_rss);

    const int soak_cycles = 100000;
    for (int i = 0; i < soak_cycles; i++) {
        pipe_msg_init_scalar(&msg, 1000 + (uint32_t)(i % 50), (double)(i % 1000) * 0.5,
                             2000 + (uint64_t)i);
        datahub_write_shadow(&hub, &msg);

        // Read shadow back to test bidirectional cycle
        pipe_msg_t readback;
        datahub_read_shadow(&hub, 1000 + (uint32_t)(i % 50), &readback);
    }

    long final_rss = get_current_rss_kb();
    printf("  -> Final RSS after %d soak cycles: %ld KB\n", soak_cycles, final_rss);

    long diff_kb = labs(final_rss - baseline_rss);
    double pct_change = baseline_rss > 0 ? ((double)diff_kb / (double)baseline_rss) * 100.0 : 0.0;
    printf("  -> RSS Delta: %ld KB (%.2f%% change, threshold < 2.0%%)\n", diff_kb, pct_change);

    // Verify steady-state convergence: ΔRSS < 2.0%
    assert(pct_change < 2.0);

    datahub_destroy(&hub);
    printf("  -> PASSED: Memory convergence verified (ΔRSS < 2.0%%) with zero unbounded growth.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-411: Soak Test & RSS Convergence Verification         \n");
    printf("=================================================================\n");

    test_soak_memory_rss_convergence();

    printf("\nAll AEP-411 Soak Convergence tests PASSED!\n");
    return 0;
}
