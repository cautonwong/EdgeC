#include <stdio.h>
#include <assert.h>
#include "ppg_spo2.h"

int main(void) {
    printf("[TEST] Testing PPG SpO2 Perfusion Index Filter...\n");

    ppg_spo2_analyzer_t analyzer;
    ppg_spo2_init(&analyzer);

    // Normal SpO2 calibration values (Red_DC=1000, Red_AC=5, IR_DC=1000, IR_AC=10 -> R=0.5 -> SpO2 = 110 - 12.5 = 97.5%)
    ppg_spo2_compute(&analyzer, 1000.0f, 5.0f, 1000.0f, 10.0f);

    printf("SpO2: %.1f%%, Perfusion Index: %.2f%%, Hypoxia: %s\n",
           analyzer.spo2_percentage, analyzer.perfusion_index, analyzer.hypoxia_alert ? "YES" : "NO");

    assert(analyzer.spo2_percentage >= 97.0f && analyzer.spo2_percentage <= 98.0f);
    assert(analyzer.perfusion_index == 1.0f);
    assert(!analyzer.hypoxia_alert);

    // Hypoxia condition (Red_AC=9 -> R=0.9 -> SpO2 = 110 - 22.5 = 87.5%)
    ppg_spo2_compute(&analyzer, 1000.0f, 9.0f, 1000.0f, 10.0f);
    assert(analyzer.hypoxia_alert);

    printf("[TEST] PPG SpO2 Perfusion Index Filter PASSED!\n");
    return 0;
}
