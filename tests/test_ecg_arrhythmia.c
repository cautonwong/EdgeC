/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

#include "ecg_arrhythmia.h"

int main(void) {
    printf("[TEST] Testing ECG Arrhythmia Detector...\n");

    ecg_arrhythmia_result_t res;
    ecg_arrhythmia_init(&res);

    // Normal ECG: RR=800ms (75 BPM), QRS=90ms, QT=400ms
    ecg_arrhythmia_analyze(&res, 800.0f, 90.0f, 400.0f);

    printf("Heart Rate: %.1f BPM, QRS: %.1f ms, PVC: %s, Bradycardia: %s, Tachycardia: %s\n",
           res.heart_rate_bpm, res.qrs_duration_ms, res.pvc_detected ? "YES" : "NO",
           res.bradycardia_alert ? "YES" : "NO", res.tachycardia_alert ? "YES" : "NO");

    assert(res.heart_rate_bpm == 75.0f);
    assert(!res.pvc_detected);
    assert(!res.bradycardia_alert);
    assert(!res.tachycardia_alert);

    // Test PVC (Premature Ventricular Contraction): RR=450ms, QRS=135ms
    ecg_arrhythmia_analyze(&res, 450.0f, 135.0f, 380.0f);
    assert(res.pvc_detected);
    assert(res.tachycardia_alert);

    printf("[TEST] ECG Arrhythmia Detector PASSED!\n");
    return 0;
}
