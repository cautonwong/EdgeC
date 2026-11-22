#include "dcu_698_object_internal.h"

#include <string.h>
#include <uv.h>

#include "dcu_discovery.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"

int dcu_698_acq_6000_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val) {
    (void)elem_idx;
    dcu_inventory_t *inv = (dcu_inventory_t *)obj->instance;
    if (!inv || !out_val) return -1;

    if (attr_id == 2) {
        *(uint16_t *)out_val = inv->count;
        return 0;
    }
    return -2;
}

int dcu_698_acq_6000_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    dcu_inventory_t *inv = (dcu_inventory_t *)obj->instance;
    if (!inv) return -1;

    if (method_id == 1) {
        const char *addr = (const char *)param;
        int ret = dcu_inventory_add(inv, addr, "DLT698", 9600, 1);
        if (result) *(int *)result = ret;
        return ret;
    }
    if (method_id == 2) {
        return -4;
    }
    if (method_id == 3) {
        return -4;
    }
    if (method_id == 4) {
        const char *addr = (const char *)param;
        int ret = dcu_inventory_remove(inv, addr);
        if (result) *(int *)result = ret;
        return 0;
    }
    if (method_id == 5) {
        memset(inv->meters, 0, sizeof(inv->meters));
        inv->count = 0;
        inv->revision++;
        if (result) *(int *)result = 0;
        return 0;
    }

    // [Legacy/Testing mapping] 保留原来的 127/128 映射以免中断外部测试用例
    if (method_id == 127) {
        int ret = dcu_inventory_add(inv, (const char *)param, "DLT698", 9600, 1);
        if (result) *(int *)result = ret;
        return ret;
    }
    if (method_id == 128) {
        int ret = dcu_inventory_remove(inv, (const char *)param);
        if (result) *(int *)result = ret;
        return ret;
    }

    return -2;
}

int dcu_698_acq_6012_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val) {
    (void)elem_idx;
    dcu_poller_t *poller = (dcu_poller_t *)obj->instance;
    if (!poller || !out_val) return -1;

    if (attr_id == 2) {
        uv_mutex_lock(&poller->lock);
        *(uint16_t *)out_val = poller->scheme_count;
        uv_mutex_unlock(&poller->lock);
        return 0;
    }
    return -2;
}

int dcu_698_acq_6012_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    dcu_poller_t *poller = (dcu_poller_t *)obj->instance;
    if (!poller) return -1;

    if (method_id == 1) {
        const dcu_scheme_t *s = (const dcu_scheme_t *)param;
        if (s->type != SCHEME_TYPE_NORMAL)
            return -1;
        int ret = dcu_poller_add_scheme(poller, s->scheme_id, s->data.normal.capture_mode,
                                        s->data.normal.csd_list, s->data.normal.csd_count);
        if (result) *(int *)result = ret;
        return ret;
    }
    return -2;
}

int dcu_698_acq_5004_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val) {
    (void)obj;
    (void)elem_idx;
    if (!out_val) return -1;

    if (attr_id == 2) {
        const router_sink_ops_t *sink = dcu_698_internal_sink_ops();
        if (sink->count_freezes) {
            return sink->count_freezes(sink->ctx, (uint16_t *)out_val);
        } else {
            *(uint16_t *)out_val = 0;
            return 0;
        }
    }
    return -2;
}

/* Class 9 方法 3/4/5/7/8 已登记进 mask，但实现随 D1-15 的 GetRecord/RSD 落地。
 * 现在明确回 -4（存在但拒绝执行），不得返回 0 让主站误以为冻结已执行。 */
int dcu_698_acq_5004_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    (void)obj;
    (void)param;
    (void)result;
    if (method_id == 3 || method_id == 4 || method_id == 5 || method_id == 7 || method_id == 8) {
        return -4;
    }
    return -2;
}

int dcu_698_acq_6002_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val) {
    (void)elem_idx;
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc || !out_val) return -1;

    if (attr_id == 2) {
        uv_mutex_lock(&disc->lock);
        *(uint8_t *)out_val = disc->status;
        uv_mutex_unlock(&disc->lock);
        return 0;
    }
    if (attr_id == 3) {
        uv_mutex_lock(&disc->lock);
        *(uint16_t *)out_val = disc->count;
        uv_mutex_unlock(&disc->lock);
        return 0;
    }
    return -2;
}

int dcu_698_acq_6002_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc) return -1;

    if (method_id == 127) {
        uint32_t duration = param ? *(const uint32_t *)param : 15;
        int ret = dcu_discovery_start(disc, duration);
        if (result) *(int *)result = ret;
        return 0;
    }
    return -2;
}

int dcu_698_acq_6003_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val) {
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc || !out_val) return -1;

    if (attr_id == 2) {
        uv_mutex_lock(&disc->lock);
        if (elem_idx < disc->count) {
            strncpy((char *)out_val, disc->discovered_tsas[elem_idx], DISCOVERY_ADDR_LEN - 1);
            ((char *)out_val)[DISCOVERY_ADDR_LEN - 1] = '\0';
            uv_mutex_unlock(&disc->lock);
            return 0;
        }
        uv_mutex_unlock(&disc->lock);
        return -3;
    }
    return -2;
}
