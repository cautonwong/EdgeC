#include "edge_wasm_sdk.h"

// Dimension 8.1 Verification: Read Tag 1001, multiply by 2, write to Tag 1002.
__attribute__((export_name("on_tick"))) void on_tick() {
    double val = 0.0;

    // Read from shadow table
    if (host_get_tag(1001, &val) == 0) {
        ext_log(LOG_INFO, "Micro-App: Successfully read Tag 1001");

        // Calculate
        double new_val = val * 2.0;

        // Write to shadow table
        if (host_set_tag(1002, new_val) == 0) {
            ext_log(LOG_INFO, "Micro-App: Successfully updated Tag 1002");
        } else {
            ext_log(LOG_ERROR, "Micro-App: Failed to update Tag 1002");
        }
    } else {
        ext_log(LOG_ERROR, "Micro-App: Failed to read Tag 1001");
    }
}
