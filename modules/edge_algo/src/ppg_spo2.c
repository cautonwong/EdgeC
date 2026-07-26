#include "ppg_spo2.h"
#include <string.h>

void ppg_spo2_init(ppg_spo2_analyzer_t *analyzer) {
    if (!analyzer) return;
    memset(analyzer, 0, sizeof(ppg_spo2_analyzer_t));
}

void ppg_spo2_compute(ppg_spo2_analyzer_t *analyzer, float red_dc, float red_ac, float ir_dc, float ir_ac) {
    if (!analyzer || red_dc <= 1e-4f || ir_dc <= 1e-4f) return;

    analyzer->red_dc = red_dc;
    analyzer->red_ac = red_ac;
    analyzer->ir_dc = ir_dc;
    analyzer->ir_ac = ir_ac;

    // Perfusion Index (PI) = (IR_AC / IR_DC) * 100%
    analyzer->perfusion_index = (ir_ac / ir_dc) * 100.0f;

    // Ratio-of-Ratios R = (Red_AC / Red_DC) / (IR_AC / IR_DC)
    float r = (red_ac / red_dc) / (ir_ac / ir_dc);

    // Standard Beer-Lambert Empirical Calibration Formula: SpO2 = 110 - 25 * R
    float spo2 = 110.0f - 25.0f * r;
    if (spo2 > 100.0f) spo2 = 100.0f;
    if (spo2 < 0.0f) spo2 = 0.0f;

    analyzer->spo2_percentage = spo2;

    // Hypoxia threshold: SpO2 < 92%
    if (spo2 < 92.0f) {
        analyzer->hypoxia_alert = true;
    } else {
        analyzer->hypoxia_alert = false;
    }
}
