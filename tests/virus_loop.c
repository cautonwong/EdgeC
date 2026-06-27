#include "edge_wasm_sdk.h"

__attribute__((export_name("on_tick"))) void on_tick() {
    double x = 0.0;
    while (1) {
        host_get_tag(1001, &x);
    }
}
