#include "dcu_698_object_internal.h"

#include <stdint.h>

#include "dcu_inventory.h"
#include "edge_log.h"
#include "edge_slave_plane.h"

int dcu_698_control_legacy_action(uint32_t slot, uint8_t action) {
    const production_meter_t *m = dcu_inventory_by_slot(dcu_698_internal_inventory(), slot);
    if (!m) {
        LOG_WARN(LOG_CAT_CORE, "DCU control: slot %u 不在档案里，拒绝执行", slot);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }

    dcu_control_sink_t sink = NULL;
    void *sink_ctx = NULL;
    dcu_698_internal_control_sink(&sink, &sink_ctx);
    if (!sink) {
        LOG_ERROR(LOG_CAT_CORE, "DCU control: 控制下沉端口未注入，拒绝执行 dev=%s", m->address);
        return EDGE_SLAVE_SRC_DENIED;
    }

    int rc = sink(sink_ctx, m->address, (uint16_t)OI_REMOTE_CONTROL, action != 0);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_CORE, "DCU control: 聚合根拒绝/下发失败 dev=%s action=%u rc=%d",
                 m->address, action, rc);
    }
    return rc;
}

int dcu_698_control_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                            void *result) {
    (void)obj;
    (void)result;

    if (method_id >= DCU_ACTION_METHOD_FIRST && method_id <= DCU_ACTION_METHOD_LAST) {
        if (method_id <= DCU_ACTION_METHOD_FIRST + 1) {
            LOG_WARN(LOG_CAT_CORE, "OI 8000 method %u: 产品没有告警输出，拒绝执行", method_id);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        if (!param) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        const dcu_action_param_t *p = (const dcu_action_param_t *)param;
        LOG_WARN(LOG_CAT_CORE,
                 "OI 8000 method %u: 参数已解码（%u 项，继电器 OAD=0x%08X），但产品没有"
                 "继电器 OAD→电表地址映射，拒绝执行",
                 method_id, p->count, p->relay_oad);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }

    return EDGE_SLAVE_SRC_CLASS_MISMATCH;
}
