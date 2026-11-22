#include "dcu_698_object_internal.h"

#include <stdio.h>

#include "dcu_security.h"

int dcu_698_security_f100_execute(const dcu_698_object_t *obj, uint8_t method_id,
                                  const void *param, void *result) {
    dcu_esam_t *esam = (dcu_esam_t *)obj->instance;
    if (!esam || !param || !result) return -1;

    if (method_id == 1) {
        return dcu_security_authenticate(esam, (const uint8_t *)param, (uint8_t *)result);
    }
    return -2;
}

static dcu_channel_cfg_t g_f200_channel_cfg = {.baudrate = 9600, .parity = 0};

int dcu_698_security_f200_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                               void *out_val) {
    (void)obj;
    (void)elem_idx;
    if (!out_val) return -1;

    if (attr_id == 2) {
        *(uint32_t *)out_val = g_f200_channel_cfg.baudrate;
        return 0;
    }
    if (attr_id == 3) {
        *(uint8_t *)out_val = g_f200_channel_cfg.parity;
        return 0;
    }
    return -2;
}

int dcu_698_security_f200_write(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                                const void *in_val) {
    (void)obj;
    (void)elem_idx;
    if (!in_val) return -1;

    if (attr_id == 2) {
        g_f200_channel_cfg.baudrate = *(const uint32_t *)in_val;
        printf("🔧 [CHANNEL] 动态配置外设串口波特率调整为: %u\n", g_f200_channel_cfg.baudrate);
        return 0;
    }
    if (attr_id == 3) {
        g_f200_channel_cfg.parity = *(const uint8_t *)in_val;
        return 0;
    }
    return -2;
}
