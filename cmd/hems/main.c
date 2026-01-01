#include <stdio.h>
#include <stdlib.h>

#include "contracts.h"
#include "edge_service.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("--- EdgeOS HEMS Gateway [契约化验证] ---\n");

    // 1. 验证 DLT645 DI 映射
    float scale;
    // 映射定义在 standard_meter.yaml (DI: 02010100 -> VOLTAGE_PHASE_A)
    edge_tag_id_t id = edge_lookup_dlt645("02010100", &scale);
    printf("[Driver] DLT645 DI '02010100' -> TagID: %d, Scale: %.2f\n", id, scale);

    // 3. 验证本地契约扩展逻辑
    if (IS_ENABLED(TAG_PROJECT_HUMIDITY_ENABLED)) {
        printf("[Custom] Local Environment: Project Humidity (ID: %d) READY\n",
               TAG_PROJECT_HUMIDITY);
    }

    return 0;
}