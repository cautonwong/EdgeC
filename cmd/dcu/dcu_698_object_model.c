#include <stdint.h>

#include "dcu_698_object_internal.h"
#include "dcu_698_var.h"
#include "edge_slave_plane.h"

#define BIT(id) ((uint8_t)(1U << ((id)&7)))
#define BYTE(id) ((id) >> 3)

static dcu_698_object_t g_objects[] = {
    {.oi = OI_METER_ARCHIVE,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method = dcu_698_acq_6000_execute,
     .method_mask = {[BYTE(127)] = BIT(127),
                     [BYTE(128)] = BIT(128) | BIT(129) | BIT(130) | BIT(131) | BIT(132) | BIT(133) |
                                   BIT(134)}},

    {.oi = OI_ARCHIVE_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SINGLETON,
     .attr_mask = {[BYTE(2)] = BIT(2)}},

    {.oi = OI_TASK_SCHEDULER,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)},
     .method = dcu_698_acq_6012_execute,
     .method_mask = {[BYTE(1)] = BIT(1),
                     [BYTE(127)] = BIT(127),
                     [BYTE(128)] = BIT(128) | BIT(129) | BIT(130)}},

    /* 6013 实例由 6012 A2 的 task_id 寻址；A2 由 resolve_axdr 编码，无标量读数。 */
    {.oi = OI_TASK_UNIT,
     .class_id = IC_PARAM,
     .scope = DCU_OBJ_SHARED,
     .attr_mask = {[BYTE(2)] = BIT(2)}},

    {.oi = (uint16_t)((OAD_ACTIVE_ENERGY >> 16) & 0xFFFF),
     .class_id = IC_ENERGY,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(5)}},
    {.oi = (uint16_t)((OAD_REACTIVE_ENERGY >> 16) & 0xFFFF),
     .class_id = IC_ENERGY,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(5)}},
    {.oi = (uint16_t)((OAD_VOLTAGE >> 16) & 0xFFFF),
     .class_id = IC_VARIABLE,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},
    {.oi = (uint16_t)((OAD_CURRENT >> 16) & 0xFFFF),
     .class_id = IC_VARIABLE,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},
    {.oi = (uint16_t)((OAD_ACTIVE_POWER_TOTAL >> 16) & 0xFFFF),
     .class_id = IC_POWER,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},
    {.oi = (uint16_t)((OAD_POWER_FACTOR_TOTAL >> 16) & 0xFFFF),
     .class_id = IC_POWER,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},
    {.oi = (uint16_t)((OAD_GRID_FREQUENCY >> 16) & 0xFFFF),
     .class_id = IC_DATA_VARIABLE,
     .scope = DCU_OBJ_PER_METER,
     .read = dcu_698_var_read,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3)}},

    /* 5004 只走 GetRecord（D1-07）：A2 是冻结数据数组而不是条数，把条数当属性值
     * 就是假语义。属性与方法一律不暴露，类内定义交给 class_defines_* 决定 DAR。 */
    {.oi = (uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF),
     .class_id = IC_RECORD,
     .scope = DCU_OBJ_SHARED},

    {.oi = OI_REMOTE_CONTROL,
     .class_id = IC_CONTROL,
     .scope = DCU_OBJ_SINGLETON,
     .method = dcu_698_control_execute,
     .method_mask = {[BYTE(DCU_ACTION_METHOD_FIRST)] = BIT(DCU_ACTION_METHOD_FIRST),
                     [BYTE(DCU_ACTION_METHOD_FIRST + 1)] =
                         BIT(DCU_ACTION_METHOD_FIRST + 1) | BIT(DCU_ACTION_METHOD_FIRST + 2) |
                         BIT(DCU_ACTION_METHOD_FIRST + 3) | BIT(DCU_ACTION_METHOD_FIRST + 4)}},

    {.oi = OI_ESAM_SECURITY,
     .class_id = IC_ESAM,
     .scope = DCU_OBJ_SINGLETON,
     .method = dcu_698_security_f100_execute,
     .method_mask = {[BYTE(1)] = BIT(1)}},

    {.oi = OI_CHANNEL_CONFIG, .class_id = IC_PERIPHERAL, .scope = DCU_OBJ_SINGLETON},

    {.oi = OI_METER_DISCOVERY,
     .class_id = IC_ACQUISITION,
     .scope = DCU_OBJ_SINGLETON,
     .read = dcu_698_acq_6002_read,
     .method = dcu_698_acq_6002_execute,
     .attr_mask = {[BYTE(3)] = BIT(3)},
     .method_mask = {[BYTE(127)] = BIT(127)}},
    {.oi = OI_DISCOVERY_RESULT, .class_id = IC_ACQUISITION, .scope = DCU_OBJ_SINGLETON},

    {.oi = OI_VOLTAGE_DROP,
     .class_id = IC_SUB_EVENT,
     .scope = DCU_OBJ_SHARED,
     .read = dcu_698_event_read,
     .write = dcu_698_event_write,
     .method = dcu_698_event_execute,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(6),
                   [BYTE(11)] = BIT(11) | BIT(12) | BIT(14)},
     .method_mask = {[BYTE(1)] = BIT(1)}},
    {.oi = OI_OVER_CURRENT,
     .class_id = IC_SUB_EVENT,
     .scope = DCU_OBJ_SHARED,
     .read = dcu_698_event_read,
     .write = dcu_698_event_write,
     .method = dcu_698_event_execute,
     .attr_mask = {[BYTE(2)] = BIT(2) | BIT(3) | BIT(4) | BIT(6),
                   [BYTE(11)] = BIT(11) | BIT(12) | BIT(14)},
     .method_mask = {[BYTE(1)] = BIT(1)}},
};

#define OBJECT_COUNT (sizeof(g_objects) / sizeof(g_objects[0]))

static bool g_initialized = false;
static router_bus_ops_t g_router_bus_ops;
static router_sink_ops_t g_router_sink_ops;
static dcu_inventory_t *s_router_inv = NULL;
static dcu_control_sink_t s_control_sink;
static void *s_control_ctx;

static dcu_698_object_t *_find_mut(uint16_t oi) {
    for (size_t i = 0; i < OBJECT_COUNT; i++) {
        if (g_objects[i].oi == oi) return &g_objects[i];
    }
    return NULL;
}

static void _set_instance(uint16_t oi, const void *instance) {
    dcu_698_object_t *obj = _find_mut(oi);
    if (obj) obj->instance = instance;
}

int dcu_698_object_model_init(dcu_inventory_t *inv, dcu_poller_t *poller,
                              router_sink_ops_t sink_ops, dcu_esam_t *esam,
                              dcu_discovery_t *discovery, dcu_event_obj_t *evt_3000,
                              dcu_event_obj_t *evt_3005, router_bus_ops_t bus_ops) {
    g_router_sink_ops = sink_ops;
    g_router_bus_ops = bus_ops;
    s_router_inv = inv;

    for (size_t i = 0; i < OBJECT_COUNT; i++) g_objects[i].instance = NULL;
    g_initialized = true;

    _set_instance(OI_METER_ARCHIVE, inv);
    _set_instance(OI_ARCHIVE_UNIT, inv);
    _set_instance(OI_TASK_SCHEDULER, poller);
    _set_instance(OI_ESAM_SECURITY, esam);
    _set_instance(OI_METER_DISCOVERY, discovery);
    _set_instance(OI_DISCOVERY_RESULT, discovery);
    _set_instance(OI_VOLTAGE_DROP, evt_3000);
    _set_instance(OI_OVER_CURRENT, evt_3005);
    return 0;
}

void dcu_698_object_model_reset(void) {
    for (size_t i = 0; i < OBJECT_COUNT; i++) g_objects[i].instance = NULL;
    g_initialized = false;
    g_router_bus_ops = (router_bus_ops_t){0};
    g_router_sink_ops = (router_sink_ops_t){0};
    s_router_inv = NULL;
}

void dcu_698_object_model_set_control_sink(dcu_control_sink_t fn, void *ctx) {
    s_control_sink = fn;
    s_control_ctx = ctx;
}

const dcu_698_object_t *dcu_698_find(uint16_t oi) {
    if (!g_initialized) return NULL;
    return _find_mut(oi);
}

size_t dcu_698_object_count(void) {
    return OBJECT_COUNT;
}

const dcu_698_object_t *dcu_698_object_at(size_t index) {
    return index < OBJECT_COUNT ? &g_objects[index] : NULL;
}

/* 附录 A 的类定义上限。 */
static const struct {
    uint8_t class_id;
    uint8_t max_attr;
} k_class_max_attr[] = {
    {1, 5},   {2, 3},  {3, 3},   {4, 3},  {5, 6},   {6, 3},   {7, 10},  {8, 2},  {9, 3},
    {10, 3},  {11, 4}, {12, 19}, {13, 5}, {14, 3},  {15, 3},  {16, 3},  {17, 4}, {18, 3},
    {19, 10}, {20, 5}, {21, 15}, {22, 4}, {23, 18}, {24, 14}, {25, 11}, {26, 5},
};

bool dcu_698_class_defines_attr(uint8_t class_id, uint8_t attr_id) {
    if (attr_id == 0) return false;
    for (size_t i = 0; i < sizeof(k_class_max_attr) / sizeof(k_class_max_attr[0]); i++) {
        if (k_class_max_attr[i].class_id == class_id)
            return attr_id <= k_class_max_attr[i].max_attr;
    }
    return false;
}

/* 附录 A 各接口类自身方法号的上限（OCI 专属的 127+ 方法另算）。 */
static const struct {
    uint8_t class_id;
    uint8_t max_method;
} k_class_max_method[] = {
    {1, 2},  {2, 2},  {3, 2},   {4, 2},  {5, 2},  {6, 2},  {7, 5},  {8, 2},  {9, 8},
    {10, 3}, {11, 2}, {12, 4},  {13, 7}, {14, 4}, {15, 4}, {16, 2}, {17, 6}, {18, 8},
    {19, 2}, {20, 2}, {21, 12}, {22, 2}, {23, 5}, {24, 5}, {25, 2}, {26, 2},
};

static bool _class_defines_method(uint8_t class_id, uint16_t oi, uint8_t method_id) {
    if (method_id == 0) return false;
    if (class_id == 9 && method_id == 6) return false; /* 表148 跳过方法 6 */
    if (oi == OI_REMOTE_CONTROL && method_id >= DCU_ACTION_METHOD_FIRST &&
        method_id <= DCU_ACTION_METHOD_LAST) {
        return true;
    }
    if (oi == OI_METER_ARCHIVE && method_id >= 127 && method_id <= 134) return true;
    if (oi == OI_TASK_SCHEDULER && method_id >= 127 && method_id <= 130) return true;
    for (size_t i = 0; i < sizeof(k_class_max_method) / sizeof(k_class_max_method[0]); i++) {
        if (k_class_max_method[i].class_id == class_id) {
            return method_id <= k_class_max_method[i].max_method;
        }
    }
    return false;
}

const router_sink_ops_t *dcu_698_internal_sink_ops(void) {
    return &g_router_sink_ops;
}

dcu_inventory_t *dcu_698_internal_inventory(void) {
    return s_router_inv;
}

void dcu_698_internal_control_sink(dcu_control_sink_t *out_fn, void **out_ctx) {
    if (out_fn) *out_fn = s_control_sink;
    if (out_ctx) *out_ctx = s_control_ctx;
}

const router_bus_ops_t *dcu_698_object_model_bus_ops(void) {
    return &g_router_bus_ops;
}

int dcu_698_read(uint32_t oad, void *out_val) {
    uint16_t oi = (uint16_t)((oad >> 16) & 0xFFFF);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj || !obj->read) return -1;
    if (!dcu_mask_test(obj->attr_mask, attr_id)) return -1;
    return obj->read(obj, attr_id, elem_idx, out_val);
}

int dcu_698_write(uint32_t oad, const void *in_val) {
    uint16_t oi = (uint16_t)((oad >> 16) & 0xFFFF);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj || !obj->write) return -1;
    if (!dcu_mask_test(obj->attr_mask, attr_id)) return -1;
    return obj->write(obj, attr_id, elem_idx, in_val);
}

int dcu_698_check_call(uint32_t omd, const dcu_698_object_t **obj_out) {
    if (obj_out) *obj_out = NULL;
    uint16_t oi = (uint16_t)((omd >> 16) & 0xFFFF);
    uint8_t method_id = (uint8_t)((omd >> 8) & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (!obj->method || !dcu_mask_test(obj->method_mask, method_id)) {
        return _class_defines_method(obj->class_id, obj->oi, method_id)
                   ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                   : EDGE_SLAVE_SRC_UNDEFINED;
    }
    if (obj_out) *obj_out = obj;
    return 0;
}

int dcu_698_call(uint32_t omd, const void *param, void *result) {
    uint8_t method_id = (uint8_t)((omd >> 8) & 0xFF);
    const dcu_698_object_t *obj = NULL;
    int rc = dcu_698_check_call(omd, &obj);
    if (rc != 0) return rc;
    return obj->method(obj, method_id, param, result);
}

int dcu_698_snapshot(uint16_t oi, uint32_t device_id, void *out_msgs, int max_count) {
    uint8_t *p_out = (uint8_t *)out_msgs;
    if (!g_router_bus_ops.read_shadow || !p_out || max_count <= 0) return 0;

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj) return 0;

    int captured = 0;

    /* class_id 是标准接口类（3xxx 为 24）；快照按处理器族分派。 */
    if (obj->read == dcu_698_event_read) {
        dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
        if (!evt) return 0;
        for (int i = 0; i < evt->snapshot_count && captured < max_count; i++) {
            uint32_t oad = evt->snapshot_oads[i];
            void *dest = p_out + (size_t)captured * g_router_bus_ops.msg_size;
            if (g_router_bus_ops.read_shadow(g_router_bus_ops.ctx, oad, device_id, dest) == 0) {
                captured++;
            }
        }
    }

    return captured;
}
