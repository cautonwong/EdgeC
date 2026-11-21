#include "dcu_698_axdr.h"

#include <string.h>
#include <uv.h>

#include "dcu_698_var.h"
#include "dcu_router.h"
#include "edge_log.h"
#include "edge_slave_plane.h"

/**
 * @brief 把 `dcu_value_t` 编成完整 A-XDR 值（含类型标签）。
 *
 * 缓冲由调用方给；任何类型放不进 `cap` 都返回 -1，不截断。
 */
static int _encode_axdr_value(const dcu_value_t* v, uint8_t* buf, size_t cap, size_t* len) {
    if (!v || !buf || !len) return -1;

    switch (v->type_tag) {
        case DCU_698_TAG_DOUBLE_LONG_UNSIGNED: {
            if (cap < 5) return -1;
            uint32_t x = (uint32_t)v->raw;
            buf[0] = DCU_698_TAG_DOUBLE_LONG_UNSIGNED;
            buf[1] = (uint8_t)(x >> 24);
            buf[2] = (uint8_t)(x >> 16);
            buf[3] = (uint8_t)(x >> 8);
            buf[4] = (uint8_t)x;
            *len = 5;
            return 0;
        }
        case DCU_698_TAG_LONG64_UNSIGNED: {
            if (cap < 9) return -1;
            uint64_t x = (uint64_t)v->raw;
            buf[0] = DCU_698_TAG_LONG64_UNSIGNED;
            for (int i = 1; i < 9; i++) buf[i] = (uint8_t)(x >> (8 * (8 - i)));
            *len = 9;
            return 0;
        }
        case DCU_698_TAG_LONG: {
            if (cap < 3) return -1;
            uint16_t x = (uint16_t)(int16_t)v->raw;
            buf[0] = DCU_698_TAG_LONG;
            buf[1] = (uint8_t)(x >> 8);
            buf[2] = (uint8_t)x;
            *len = 3;
            return 0;
        }
        case DCU_698_TAG_LONG_UNSIGNED: {
            if (cap < 3) return -1;
            uint16_t x = (uint16_t)v->raw;
            buf[0] = DCU_698_TAG_LONG_UNSIGNED;
            buf[1] = (uint8_t)(x >> 8);
            buf[2] = (uint8_t)x;
            *len = 3;
            return 0;
        }
        case DCU_698_TAG_FLOAT32: {
            if (cap < 5) return -1;
            float f = (float)v->value;
            uint32_t bits = 0;
            memcpy(&bits, &f, sizeof(bits));
            buf[0] = DCU_698_TAG_FLOAT32;
            buf[1] = (uint8_t)(bits >> 24);
            buf[2] = (uint8_t)(bits >> 16);
            buf[3] = (uint8_t)(bits >> 8);
            buf[4] = (uint8_t)bits;
            *len = 5;
            return 0;
        }
        default:
            return -1;
    }
}

int dcu_north_resolve_axdr(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    (void)ctx;
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    dcu_value_t v;
    if (dcu_698_var_read_value(oad, &v) == 0) {
        return (_encode_axdr_value(&v, buf, cap, len) == 0) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    if (oi == OI_METER_ARCHIVE && attr_id == 2) {
        uint16_t count = 0;
        if (dcu_698_read(oad, &count) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
        dcu_value_t cv = {.type_tag = DCU_698_TAG_LONG_UNSIGNED,
                          .raw = count,
                          .value = (double)count,
                          .ts_ms = (uint64_t)(uv_hrtime() / 1000000ULL),
                          .quality = 0};
        return (_encode_axdr_value(&cv, buf, cap, len) == 0) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    return EDGE_SLAVE_SRC_NOT_FOUND;
}

int dcu_north_accept_axdr(void* ctx, uint32_t oad, const uint8_t* axdr, size_t len) {
    (void)ctx;
    (void)oad;
    (void)axdr;
    (void)len;
    LOG_WARN(LOG_CAT_NB, "DCU north SET A-XDR: 698 SET 分支待 D1-14");
    return EDGE_SLAVE_SRC_TEMPORARY;
}
