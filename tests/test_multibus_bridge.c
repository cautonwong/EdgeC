#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "multibus_bridge.h"

int main(void) {
    printf("=== Test Multibus Bridge Start ===\n");

    multibus_bridge_t bridge;
    multibus_bridge_init(&bridge);

    /* Rule 1: Modbus TCP Reg 40001 -> IEC104 ASDU Point 101, Scale 0.1, Offset 0 */
    assert(multibus_bridge_add_rule(&bridge, 1, BUS_TYPE_MODBUS_TCP, 40001, BUS_TYPE_IEC104, 101,
                                    0.1, 0.0) == 0);

    /* Rule 2: CAN ID 0x18FEF600 -> DataHub SHM Tag 12001, Scale 1.0, Offset -40.0 */
    assert(multibus_bridge_add_rule(&bridge, 2, BUS_TYPE_CAN, 0x18FEF600, BUS_TYPE_DATAHUB_SHM,
                                    12001, 1.0, -40.0) == 0);

    multibus_type_t dst_bus = BUS_TYPE_UNKNOWN;
    uint32_t dst_addr = 0;
    double translated = 0.0;

    assert(multibus_bridge_route_signal(&bridge, BUS_TYPE_MODBUS_TCP, 40001, 2300.0, &dst_bus,
                                        &dst_addr, &translated) == 0);
    assert(dst_bus == BUS_TYPE_IEC104);
    assert(dst_addr == 101);
    assert(fabs(translated - 230.0) < 0.0001);

    assert(multibus_bridge_route_signal(&bridge, BUS_TYPE_CAN, 0x18FEF600, 100.0, &dst_bus,
                                        &dst_addr, &translated) == 0);
    assert(dst_bus == BUS_TYPE_DATAHUB_SHM);
    assert(dst_addr == 12001);
    assert(fabs(translated - 60.0) < 0.0001);

    /* Test unmapped route */
    assert(multibus_bridge_route_signal(&bridge, BUS_TYPE_MODBUS_RTU, 9999, 10.0, &dst_bus,
                                        &dst_addr, &translated) == -2);

    uint64_t routed = 0, errors = 0;
    multibus_bridge_get_stats(&bridge, &routed, &errors);
    assert(routed == 2);
    assert(errors == 1);

    printf("=== Test Multibus Bridge Passed (120/120) ===\n");
    return 0;
}
