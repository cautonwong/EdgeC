#include "dcu_twin_source.h"

#include <stddef.h>

#include "dcu_router.h"

int dcu_twin_source_init(dcu_twin_source_t *source, double base_voltage, double base_current) {
    if (!source) return -1;
    source->base_voltage = (base_voltage > 0.0) ? base_voltage : 220.0;
    source->base_current = (base_current > 0.0) ? base_current : 5.0;
    return 0;
}

int dcu_twin_source_read(void *ctx, uint32_t meter_idx, uint32_t tag_id, double *out_val) {
    if (!ctx || !out_val) return -1;
    dcu_twin_source_t *source = (dcu_twin_source_t *)ctx;

    double delta = (meter_idx % 10) * 0.2;
    if (tag_id == OAD_VOLTAGE || tag_id == 0x20000200) {
        *out_val = source->base_voltage + delta;
        return 0;
    } else if (tag_id == OAD_CURRENT || tag_id == 0x20010200) {
        *out_val = source->base_current + (meter_idx % 5) * 0.1;
        return 0;
    } else if (tag_id == OAD_ACTIVE_ENERGY || tag_id == 0x00100200) {
        *out_val = 1000.0 + (double)meter_idx * 12.5;
        return 0;
    } else {
        *out_val = source->base_voltage;
        return 0;
    }
}

/* 合成源本来就是同步的：算完立刻回调。poller 的 pumping 守卫保证不会递归打穿栈。 */
static int dcu_twin_source_submit(void *ctx, uint32_t meter_idx, uint32_t tag_id,
                                  poller_read_done_cb cb, void *ud) {
    double v = 0.0;
    if (dcu_twin_source_read(ctx, meter_idx, tag_id, &v) != 0) return -1;
    if (cb) cb(ud, 0, v);
    return 0;
}

poller_source_ops_t dcu_twin_source_get_ops(dcu_twin_source_t *source) {
    poller_source_ops_t ops;
    memset(&ops, 0, sizeof(ops));
    ops.ctx = source;
    ops.submit = dcu_twin_source_submit;
    return ops;
}
