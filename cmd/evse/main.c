#include <stdio.h>
#include <stdlib.h>

#include "contracts.h"  // 引用 EVSE 特定的 contracts.h
#include "edge_service.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("EdgeOS EVSE Charging Station App Starting...\n");

#if IS_ENABLED(TAG_EV_CHARGE_POWER_ENABLED)
    printf("[Contract] EV Charging Logic: ACTIVE (TagID: %d)\n", TAG_EV_CHARGE_POWER);
#endif

    // 演示多协议映射
    float scale;
    // 模拟从 DLMS 驱动解析到的 OBIS 映射 (映射定义在 generic_ess.yaml)
    edge_tag_id_t id = edge_lookup_dlms("1.0.66.7.0.255", &scale);
    printf("[Driver] DLMS Lookup for Power: TagID=%d, Scale=%.3f\n", id, scale);

    return 0;
}