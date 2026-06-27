#include "edge_wasm_sdk.h"

__attribute__((export_name("on_tick"))) void on_tick() {
    volatile int a = 1;
    volatile int b = 0;
    volatile int c = a / b; // Division by zero
    (void)c;
}
