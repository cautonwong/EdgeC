/**
 * @file dcu_router.c
 * @brief 698 路由门面：对象表在 dcu_698_object_model.c，这里只做查表与转发。
 *
 * D1-03 起 `dcu_router` 不再持有注册表。公开 API 与返回码保持不变：
 * 缺失属性/方法仍由处理器返回 -2/-3/-4，门面不把处理器错误改写成 -1。
 */

#include "dcu_router.h"

#include "dcu_698_object_model.h"

/**
 * @brief 动态散列槽位映射 (No-Array / Microservice Ready)
 * 彻底消除硬编码静态数组表，支持任意 698 OAD / DI 动态算术映射入槽
 */
int dcu_tag_map_fn(uint32_t tag_id, uint32_t device_id) {
    if (tag_id == 0) return -1;
    /* 应用点位区只在 [APP_BASE, APP_BASE + 4096) 内。产品既有 tag 使用
     * 0x2xxxxxxx 形态的 OAD 值，不能按 >= APP_BASE 一刀切。 */
    if (tag_id >= DCU_APP_BASE && tag_id < DCU_APP_BASE + DCU_APP_SLOT_TOTAL) return -1;
    uint32_t slot = 0;
    if (tag_id == 1001 || tag_id == 1004 || tag_id == OAD_VOLTAGE) {
        slot = 0;
    } else if (tag_id == 3001 || tag_id == OAD_ACTIVE_ENERGY) {
        slot = 1;
    } else if (tag_id == 1002 || tag_id == OAD_CURRENT) {
        slot = 2;
    } else {
        // 对任意 698 扩展 OAD 进行无表算术散列分发，避免静态数组维护
        uint32_t h = tag_id;
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = (h >> 16) ^ h;
        slot = 3 + (h % (DCU_SLOTS_PER_DEVICE - 3));
    }
    return (int)(device_id * DCU_SLOTS_PER_DEVICE + slot);
}

int dcu_router_init_ex(dcu_inventory_t* inv, dcu_poller_t* poller, router_sink_ops_t sink_ops,
                       dcu_esam_t* esam, dcu_discovery_t* discovery, dcu_event_obj_t* evt_3000,
                       dcu_event_obj_t* evt_3005, router_bus_ops_t bus_ops) {
    return dcu_698_object_model_init(inv, poller, sink_ops, esam, discovery, evt_3000, evt_3005,
                                     bus_ops);
}

int dcu_router_init(dcu_inventory_t* inv, dcu_poller_t* poller, void* storage) {
    (void)storage;
    router_sink_ops_t dummy_sink = {0};
    router_bus_ops_t dummy_bus = {0};
    return dcu_router_init_ex(inv, poller, dummy_sink, NULL, NULL, NULL, NULL, dummy_bus);
}

int dcu_router_get_attr(uint32_t oad, void* out_val) {
    uint16_t oi = (uint16_t)((oad >> 16) & 0xFFFF);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    /* 门面直接调处理器，不走 dcu_698_read 的 mask 检查：
     * 现有调用方依赖处理器自己的 -2/-3 返回码，改写成 -1 就是行为变更。 */
    const dcu_698_object_t* obj = dcu_698_find(oi);
    if (!obj || !obj->read) return -1;
    return obj->read(obj, attr_id, elem_idx, out_val);
}

int dcu_router_set_attr(uint32_t oad, const void* in_val) {
    uint16_t oi = (uint16_t)((oad >> 16) & 0xFFFF);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    const dcu_698_object_t* obj = dcu_698_find(oi);
    if (!obj || !obj->write) return -1;
    return obj->write(obj, attr_id, elem_idx, in_val);
}

int dcu_router_action(uint32_t omd, const void* param, void* result) {
    uint16_t oi = (uint16_t)((omd >> 16) & 0xFFFF);
    uint8_t method_id = (uint8_t)((omd >> 8) & 0xFF);

    /* 同上：遗留测试用方法 127/128 不在 16 位 mask 内，门面必须直达处理器。 */
    const dcu_698_object_t* obj = dcu_698_find(oi);
    if (!obj || !obj->method) return -1;
    return obj->method(obj, method_id, param, result);
}

int dcu_obj_snapshot(uint16_t oi, uint32_t device_id, void* out_msgs, int max_count) {
    return dcu_698_snapshot(oi, device_id, out_msgs, max_count);
}

void dcu_router_set_control_sink(dcu_control_sink_t fn, void* ctx) {
    dcu_698_object_model_set_control_sink(fn, ctx);
}

void dcu_router_destroy(void) {
    dcu_698_object_model_reset();
}
