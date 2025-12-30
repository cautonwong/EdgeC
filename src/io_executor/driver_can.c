#include <stdio.h>
#include <string.h>

#include "contract_data_types.h"
#include "edge_driver.h"

// 引用生成的 CAN 映射表
extern const can_map_t g_can_map[];
extern const int g_can_map_len;

static int can_process_frame(uint32_t can_id, const uint8_t *data, size_t len) {
    (void)len;

    // 工业 CAN 逻辑：单帧多测点提取
    for (int i = 0; i < g_can_map_len; i++) {
        if (g_can_map[i].can_id == can_id) {
            // 提取逻辑：从 data[start] 长度 len 中根据 factor/offset 换算
            // 这里演示 TagID 识别
            printf("[CAN] Identified Tag %u in Frame 0x%X\n", g_can_map[i].id, can_id);
        }
    }
    return 0;
}

edge_driver_t can_bus_driver = {
    .name = "CAN_Raw_ContractAware",
    // CAN 驱动通常直接由 io_executor 调用 process_frame
};
