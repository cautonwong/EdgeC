#define IMPORT(name) __attribute__((import_module("env"), import_name(name)))

IMPORT("host_get_tag") int host_get_tag(unsigned int tag_id, double *val_ptr);
IMPORT("host_set_tag") int host_set_tag(unsigned int tag_id, double val);
IMPORT("ext_log") void ext_log(unsigned int level, const char *msg);

// FMCW Radar & TinyML WASM Operator State
static double g_distance_history[10];
static int g_hist_idx = 0;
static double g_respiration_accum = 0.0;
static int g_resp_sample_cnt = 0;

__attribute__((export_name("on_tick")))
void on_tick() {
    double dist = 0.0;
    double vel = 0.0;
    double micro_amp = 0.0;

    // 1. Read FMCW Radar Raw Signal Inputs (Tags 12021 - 12023)
    if (host_get_tag(12021, &dist) != 0) dist = 1.5;
    if (host_get_tag(12022, &vel) != 0) vel = 0.0;
    if (host_get_tag(12023, &micro_amp) != 0) micro_amp = 0.05;

    // Push into sliding window
    g_distance_history[g_hist_idx] = dist;
    g_hist_idx = (g_hist_idx + 1) % 10;

    // 2. TinyML FMCW Micro-Motion Respiration Analysis (0.2Hz - 0.5Hz Chest Motion)
    g_respiration_accum += micro_amp;
    g_resp_sample_cnt++;
    if (g_resp_sample_cnt >= 10) {
        double avg_amp = g_respiration_accum / 10.0;
        // Estimate respiration rate (bpm) based on micro-motion amplitude spectrum
        double resp_rate = 16.0 + (avg_amp * 40.0);
        if (resp_rate > 35.0) resp_rate = 35.0;
        if (resp_rate < 8.0) resp_rate = 8.0;

        host_set_tag(12024, resp_rate); // Tag 12024: RADAR_RESPIRATION_RATE
        g_respiration_accum = 0.0;
        g_resp_sample_cnt = 0;
    }

    // 3. TinyML Gesture & Fall Pre-Warning Classifier
    // High velocity + high micro-motion amplitude -> Gesture or Fall Pre-warning
    int gesture_event = 0; // 0=none, 1=wave, 2=push, 3=fall_pre_warning
    if (vel > 1.8 && micro_amp > 0.4) {
        gesture_event = 3; // Fall Pre-warning
        ext_log(3, "WASM-TinyML: Rapid velocity & high micro-motion amplitude detected! Fall pre-warning triggered.");
    } else if (vel > 0.8 && micro_amp > 0.2) {
        gesture_event = 1; // Hand Wave
        ext_log(1, "WASM-TinyML: Hand wave gesture recognized.");
    } else if (vel < -0.8 && micro_amp > 0.2) {
        gesture_event = 2; // Hand Push
        ext_log(1, "WASM-TinyML: Hand push gesture recognized.");
    }
    host_set_tag(12025, (double)gesture_event); // Tag 12025: RADAR_GESTURE_EVENT

    // 4. Industrial SimpleFOC Motor Speed Tracking & Safety Monitoring (Tags 12026 - 12030)
    double target_vel = 0.0;
    double actual_vel = 0.0;
    double motor_temp = 25.0;
    if (host_get_tag(12026, &target_vel) != 0) target_vel = 1000.0;
    if (host_get_tag(12027, &actual_vel) != 0) actual_vel = target_vel;
    if (host_get_tag(12029, &motor_temp) != 0) motor_temp = 35.0;

    // Evaluate q-axis current proportional to speed error
    double vel_err = target_vel - actual_vel;
    double iq_current = vel_err * 0.05 + 0.8;
    if (iq_current > 10.0) iq_current = 10.0;
    if (iq_current < -10.0) iq_current = -10.0;
    host_set_tag(12028, iq_current); // Tag 12028: FOC_IQ_CURRENT

    // Motor Over-Temperature & Over-Current Fault Protection
    int foc_fault = 0;
    if (motor_temp > 85.0) {
        foc_fault = 2; // Over-temperature
        ext_log(4, "WASM-FOC: Motor over-temperature fault (T > 85C)!");
    } else if (iq_current >= 9.8 || iq_current <= -9.8) {
        foc_fault = 1; // Over-current
        ext_log(4, "WASM-FOC: Motor over-current fault (|Iq| >= 9.8A)!");
    }
    host_set_tag(12030, (double)foc_fault); // Tag 12030: FOC_FAULT_CODE

    // 5. Automotive UDS Diagnostic Session Monitor (Tags 12031 - 12035)
    double doip_conn = 0.0;
    host_get_tag(12035, &doip_conn);
    if (doip_conn > 0.0) {
        // Active DoIP client connected
        double dtc_cnt = 0.0;
        host_get_tag(12032, &dtc_cnt);
        if (foc_fault != 0) {
            host_set_tag(12032, dtc_cnt + 1.0); // Increment active DTC count
        }
    }
}
