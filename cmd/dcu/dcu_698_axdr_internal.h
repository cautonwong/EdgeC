#ifndef DCU_698_AXDR_INTERNAL_H
#define DCU_698_AXDR_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dlt698_codec.h"
#include "edge_slave_plane.h"

typedef struct {
    const uint8_t *p;
    size_t len;
    size_t off;
} axdr_cursor_t;

/* ACTION-Request 的 TimeTag 可选：线格式要么没有这个字节，要么就是一个 0x00。
 * 真实 TimeTag 要做传输时效判定（6.1.1.5），本实现不做，按 DAR 7 拒绝，不静默忽略。 */
static inline int _time_tag_absent(const uint8_t *buf, size_t off, size_t len) {
    return off == len || (off + 1 == len && buf[off] == DLT698_NO_TIME_TAG);
}

static inline bool _take(axdr_cursor_t *c, size_t n, const uint8_t **out) {
    if (c->off + n < c->off || c->off + n > c->len) return false;
    if (out) *out = c->p + c->off;
    c->off += n;
    return true;
}

static inline int _take_u8(axdr_cursor_t *c, uint8_t tag, uint8_t *out) {
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != tag) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (out) *out = p[1];
    return 0;
}

static inline int _take_u16(axdr_cursor_t *c, uint8_t tag, uint16_t *out) {
    const uint8_t *p = NULL;
    if (!_take(c, 3, &p) || p[0] != tag) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (out) *out = (uint16_t)((p[1] << 8) | p[2]);
    return 0;
}

static inline int _take_head(axdr_cursor_t *c, uint8_t tag, uint8_t count) {
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != tag || p[1] != count) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    return 0;
}

static inline int _take_array(axdr_cursor_t *c, uint8_t *count) {
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != DLT698_TAG_ARRAY) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *count = p[1];
    return 0;
}

static inline int _put_tagged_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint8_t v) {
    if (*off + 2 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = v;
    return 0;
}

static inline int _put_tagged_u16(uint8_t *buf, size_t cap, size_t *off, uint8_t tag,
                                  uint16_t v) {
    if (*off + 3 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = (uint8_t)(v >> 8);
    buf[(*off)++] = (uint8_t)v;
    return 0;
}

/* 表 24：TI 没有内层标签。 */
static inline int _take_ti(axdr_cursor_t *c, uint8_t *unit, uint16_t *value) {
    const uint8_t *p = NULL;
    if (!_take(c, 4, &p) || p[0] != DLT698_TAG_TI) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (unit) *unit = p[1];
    if (value) *value = (uint16_t)((p[2] << 8) | p[3]);
    return 0;
}

static inline int _put_raw_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t v) {
    if (*off + 1 > cap) return -1;
    buf[(*off)++] = v;
    return 0;
}

int dcu_698_axdr_take_ms(axdr_cursor_t *c, uint8_t *ms_choice);

#endif /* DCU_698_AXDR_INTERNAL_H */
