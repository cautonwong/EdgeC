#include "dcu_698_object_internal.h"

#include <string.h>
#include <uv.h>

#include "dcu_discovery.h"
#include "dcu_poller.h"

int dcu_698_acq_6012_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                             void *result) {
    dcu_poller_t *poller = (dcu_poller_t *)obj->instance;
    if (!poller) return -1;

    if (method_id == 1) {
        uv_mutex_lock(&poller->lock);
        poller->scheme_count = 0;
        memset(poller->schemes, 0, sizeof(poller->schemes));
        uv_mutex_unlock(&poller->lock);
        if (result) *(int *)result = 0;
        return 0;
    }
    if (method_id == 127) {
        const dcu_scheme_t *s = (const dcu_scheme_t *)param;
        if (!s || s->type != SCHEME_TYPE_NORMAL) return -1;
        int ret = dcu_poller_add_scheme(poller, s->scheme_id, s->data.normal.capture_mode,
                                        s->data.normal.csd_list, s->data.normal.csd_count);
        if (result) *(int *)result = ret;
        return ret;
    }
    return -2;
}

int dcu_698_acq_6002_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                          void *out_val) {
    (void)elem_idx;
    dcu_discovery_t *disc = (dcu_discovery_t *)obj->instance;
    if (!disc || !out_val) return -1;

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
        return ret;
    }
    return -2;
}
