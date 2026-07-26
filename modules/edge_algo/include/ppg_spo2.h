#ifndef PPG_SPO2_H
#define PPG_SPO2_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float red_dc;
    float red_ac;
    float ir_dc;
    float ir_ac;
    float spo2_percentage; // 70.0 .. 100.0%
    float perfusion_index; // 0.02 .. 20.0%
    bool hypoxia_alert;
} ppg_spo2_analyzer_t;

void ppg_spo2_init(ppg_spo2_analyzer_t *analyzer);

void ppg_spo2_compute(ppg_spo2_analyzer_t *analyzer, float red_dc, float red_ac, float ir_dc, float ir_ac);

#ifdef __cplusplus
}
#endif

#endif // PPG_SPO2_H
