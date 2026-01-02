#include <stdio.h>

#include "contracts.h"
#include "edge_log.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    edge_log_init(NULL, false);
    edge_contract_self_test();
    printf("--- EdgeOS SG Gateway [契约化验证] ---\n");

    // 1. 验证 Modbus 动态检索 (安科瑞点表)
    float scale;
    // 地址 0x0000 对应 VOLTAGE_PHASE_A
    edge_tag_id_t id = edge_lookup_modbus(0x0000, &scale);
    printf("[Driver] Modbus Addr 0x0000 -> TagID: %d, Scale: %.2f\n", id, scale);

    // 2. 验证网关内部测点
    if (IS_ENABLED(TAG_GATEWAY_INTERNAL_TEMP_ENABLED)) {
        printf("[Self-Mon] Internal Temp (ID: %d) MONITORING\n", TAG_GATEWAY_INTERNAL_TEMP);
    }

    return 0;
}