#include <stdio.h>
#include <stdlib.h>

#include "contracts.h"
#include "edge_service.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("--- EdgeOS EVSE App [契约化验证] ---\n");

    // 1. 验证 DLMS OBIS 映射
    float scale;
    // 映射定义在 generic_ess.yaml (OBIS: 1.0.66.7.0.255 -> EV_CHARGE_POWER)
    edge_tag_id_t id = edge_lookup_dlms("1.0.66.7.0.255", &scale);
    printf("[Driver] DLMS OBIS '1.0.66.7.0.255' -> TagID: %d, Scale: %.2f\n", id, scale);

    // 2. 验证本地私有报警
    if (IS_ENABLED(TAG_PLUG_OVERTEMP_ENABLED)) {
        printf("[Custom] Local Alarm Logic: Plug Overtemp (ID: %d) ACTIVE\n", TAG_PLUG_OVERTEMP);
    }

    return 0;
}
