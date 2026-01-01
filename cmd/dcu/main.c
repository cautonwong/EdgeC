#include <stdio.h>
#include <stdlib.h>

#include "contracts.h"
#include "edge_service.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("--- EdgeOS DCU Application [契约化验证] ---\n");

    // 1. 验证特性门控
    if (IS_ENABLED(TAG_GRID_FREQUENCY_ENABLED)) {
        printf("[Contract] Frequency Monitoring: ACTIVE (ID: %d)\n", TAG_GRID_FREQUENCY);
    }

    // 2. 验证多协议查表：DLT698 OAID
    float scale;
    // 模拟从 DLT698 驱动获取频率数据 (OAID 定义在 dcu_interface.yaml)
    edge_tag_id_t id = edge_lookup_dlt698("20010200", &scale);
    printf("[Driver] DLT698 OAID '20010200' -> TagID: %d, Scale: %.2f\n", id, scale);

    // 3. 验证本地扩展测点感知
    if (IS_ENABLED(TAG_PROJECT_HUMIDITY_ENABLED)) {
        printf("[Custom] Local Sensor Detected: Project Humidity (ID: %d)\n", TAG_PROJECT_HUMIDITY);
    }

    return 0;
}
