#include "dcu_discovery.h"

#include <stddef.h>

void dcu_discovery_init(dcu_discovery_t* disc) {
    if (!disc) return;

    disc->status = DISCOVERY_STATUS_IDLE;
    disc->count = 0;
    uv_mutex_init(&disc->lock);
}

void dcu_discovery_clear(dcu_discovery_t* disc) {
    if (!disc) return;

    uv_mutex_lock(&disc->lock);
    disc->count = 0;
    uv_mutex_unlock(&disc->lock);
}

void dcu_discovery_snapshot(dcu_discovery_t* disc, uint8_t* out_status, uint16_t* out_count) {
    if (!disc) return;

    uv_mutex_lock(&disc->lock);
    if (out_status) *out_status = (uint8_t)disc->status;
    if (out_count) *out_count = disc->count;
    uv_mutex_unlock(&disc->lock);
}

void dcu_discovery_destroy(dcu_discovery_t* disc) {
    if (!disc) return;

    uv_mutex_destroy(&disc->lock);
}
