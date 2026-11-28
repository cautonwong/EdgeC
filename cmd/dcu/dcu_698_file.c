/**
 * @file dcu_698_file.c
 * @brief Class 18 (0xF000/0xF001/0xF002) 文件传输软件领域引擎实现
 */

#include "dcu_698_file.h"

#include <string.h>

#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define DCU_FILE_TAG_BIT_STRING 0x04u

static dcu_file_session_t s_file_session = {
    .active = false,
    .verified = false,
    .cmd_result = DCU_FILE_RES_RESET,
};

static bool _bitmap_get(const uint8_t *bitmap, uint16_t idx) {
    return (bitmap[idx / 8u] & (uint8_t)(1u << (idx % 8u))) != 0u;
}

static void _bitmap_set(uint8_t *bitmap, uint16_t idx) {
    bitmap[idx / 8u] |= (uint8_t)(1u << (idx % 8u));
}

static uint16_t _bitmap_count(const uint8_t *bitmap, uint16_t total_blocks) {
    uint16_t count = 0;
    for (uint16_t i = 0; i < total_blocks; i++) {
        if (_bitmap_get(bitmap, i)) count++;
    }
    return count;
}

static size_t _expected_block_len(const dcu_file_session_t *s, uint16_t block_idx) {
    size_t offset = (size_t)block_idx * (size_t)s->block_size;
    size_t remaining = (size_t)s->total_size - offset;
    return (remaining < (size_t)s->block_size) ? remaining : (size_t)s->block_size;
}

static bool _is_file_oi(uint16_t oi) {
    return oi == 0xF000u || oi == 0xF001u || oi == 0xF002u;
}

uint32_t dcu_698_file_crc32(const uint8_t *data, size_t len) {
    if (!data && len > 0u) return 0u;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            uint32_t mask = (uint32_t)(-(int32_t)(crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

void dcu_698_file_reset(void) {
    memset(&s_file_session, 0, sizeof(s_file_session));
    s_file_session.cmd_result = DCU_FILE_RES_RESET;
}

const dcu_file_session_t *dcu_698_file_get_session(void) {
    return &s_file_session;
}

int dcu_698_file_start(const char *filename, uint32_t total_size, uint16_t block_size,
                       uint32_t expected_crc32) {
    if (!filename || filename[0] == '\0') return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    size_t name_len = strnlen(filename, DCU_FILE_NAME_MAX);
    if (name_len >= DCU_FILE_NAME_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    if (total_size == 0u || total_size > DCU_FILE_MAX_SIZE) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (block_size == 0u || block_size > DCU_FILE_BLOCK_SIZE) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    uint32_t total_blocks = (total_size + block_size - 1u) / block_size;
    if (total_blocks == 0u || total_blocks > DCU_FILE_MAX_BLOCKS) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    memset(&s_file_session, 0, sizeof(s_file_session));
    memcpy(s_file_session.filename, filename, name_len);
    s_file_session.filename[name_len] = '\0';
    s_file_session.active = true;
    s_file_session.verified = false;
    s_file_session.total_size = total_size;
    s_file_session.block_size = block_size;
    s_file_session.total_blocks = (uint16_t)total_blocks;
    s_file_session.expected_crc32 = expected_crc32;
    s_file_session.cmd_result = 0u;
    return 0;
}

int dcu_698_file_write_block(uint16_t block_idx, const uint8_t *data, size_t len) {
    if (!s_file_session.active) return EDGE_SLAVE_SRC_TEMPORARY;
    if (!data || len == 0u) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (block_idx >= s_file_session.total_blocks) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    size_t offset = (size_t)block_idx * (size_t)s_file_session.block_size;
    size_t expected_len = _expected_block_len(&s_file_session, block_idx);
    if (offset + len > (size_t)s_file_session.total_size || len != expected_len) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    memcpy(s_file_session.buffer + offset, data, len);
    _bitmap_set(s_file_session.bitmap, block_idx);
    s_file_session.verified = false;

    uint16_t rx_blocks = _bitmap_count(s_file_session.bitmap, s_file_session.total_blocks);
    s_file_session.cmd_result = (uint8_t)((rx_blocks * 99u) / s_file_session.total_blocks);
    return 0;
}

int dcu_698_file_read_block(uint16_t block_idx, uint8_t *out_buf, size_t cap, size_t *out_len) {
    if (out_len) *out_len = 0u;
    if (!out_buf || !out_len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (!s_file_session.active) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (block_idx >= s_file_session.total_blocks) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (!_bitmap_get(s_file_session.bitmap, block_idx)) return EDGE_SLAVE_SRC_TEMPORARY;

    size_t offset = (size_t)block_idx * (size_t)s_file_session.block_size;
    size_t blk_len = _expected_block_len(&s_file_session, block_idx);
    if (cap < blk_len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    memcpy(out_buf, s_file_session.buffer + offset, blk_len);
    *out_len = blk_len;
    return 0;
}

int dcu_698_file_verify(void) {
    if (!s_file_session.active) return EDGE_SLAVE_SRC_NOT_FOUND;

    for (uint16_t i = 0; i < s_file_session.total_blocks; i++) {
        if (!_bitmap_get(s_file_session.bitmap, i)) {
            s_file_session.verified = false;
            s_file_session.cmd_result = DCU_FILE_RES_ABORTED;
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
    }

    uint32_t actual_crc = dcu_698_file_crc32(s_file_session.buffer, s_file_session.total_size);
    if (actual_crc != s_file_session.expected_crc32) {
        s_file_session.verified = false;
        s_file_session.cmd_result = DCU_FILE_RES_VERIFY_FAIL;
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    s_file_session.verified = true;
    s_file_session.cmd_result = DCU_FILE_RES_OK;
    return 0;
}

static int _put_visible_string(uint8_t *buf, size_t cap, size_t *off, const char *s,
                               size_t max_len) {
    size_t slen = s ? strnlen(s, max_len) : 0u;
    if (slen > 127u || *off + 2u + slen > cap) return -1;
    buf[(*off)++] = (uint8_t)DLT698_TAG_VISIBLE_STRING;
    buf[(*off)++] = (uint8_t)slen;
    if (slen > 0u) {
        memcpy(buf + *off, s, slen);
        *off += slen;
    }
    return 0;
}

static int _put_u32(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint32_t v) {
    if (*off + 5u > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = (uint8_t)(v >> 24);
    buf[(*off)++] = (uint8_t)(v >> 16);
    buf[(*off)++] = (uint8_t)(v >> 8);
    buf[(*off)++] = (uint8_t)v;
    return 0;
}

static int _put_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint8_t v) {
    if (*off + 2u > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = v;
    return 0;
}

static int _encode_file_info_a2(const dcu_file_session_t *s, uint8_t *buf, size_t cap,
                                size_t *off) {
    const char *name = s->active ? s->filename : "";
    uint32_t size = s->active ? s->total_size : 0u;
    const char *ver = s->active ? "V1.0" : "";

    if (_put_u8(buf, cap, off, (uint8_t)DLT698_TAG_STRUCTURE, 6u) != 0) return -1;
    if (_put_visible_string(buf, cap, off, name, DCU_FILE_NAME_MAX - 1u) != 0) return -1;
    if (_put_visible_string(buf, cap, off, name, DCU_FILE_NAME_MAX - 1u) != 0) return -1;
    if (_put_u32(buf, cap, off, (uint8_t)DLT698_TAG_DOUBLE_LONG_UNSIGNED, size) != 0) return -1;

    /* 文件属性 bit-string(SIZE(3)): read | write | exec = 0xE0 (MSB-first) */
    if (*off + 3u > cap) return -1;
    buf[(*off)++] = (uint8_t)DCU_FILE_TAG_BIT_STRING;
    buf[(*off)++] = 3u;
    buf[(*off)++] = s->active ? 0xC0u : 0x00u;

    if (_put_visible_string(buf, cap, off, ver, 4u) != 0) return -1;
    if (_put_u8(buf, cap, off, (uint8_t)DLT698_TAG_ENUM, 0u) != 0) return -1;
    return 0;
}

static int _encode_bitmap(const dcu_file_session_t *s, uint8_t *buf, size_t cap, size_t *off) {
    uint16_t nbits = s->active ? s->total_blocks : 0u;
    size_t nbytes = (size_t)((nbits + 7u) / 8u);
    if (*off + 2u + nbytes > cap) return -1;
    buf[(*off)++] = (uint8_t)DCU_FILE_TAG_BIT_STRING;
    buf[(*off)++] = (uint8_t)nbits;
    if (nbytes > 0u) {
        memcpy(buf + *off, s->bitmap, nbytes);
        *off += nbytes;
    }
    return 0;
}

int dcu_698_file_encode_attr(uint16_t oi, uint8_t attr_id, uint8_t *buf, size_t cap,
                             size_t *out_len) {
    if (out_len) *out_len = 0u;
    if (!_is_file_oi(oi)) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (!buf || !out_len || cap == 0u) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    size_t off = 0u;
    int rc = 0;

    switch (attr_id) {
        case 2:
            rc = _encode_file_info_a2(&s_file_session, buf, cap, &off);
            break;
        case 3:
            if (oi == 0xF001u) {
                rc = _encode_bitmap(&s_file_session, buf, cap, &off);
            } else {
                rc = _put_u8(buf, cap, &off, (uint8_t)DLT698_TAG_ENUM, s_file_session.cmd_result);
            }
            break;
        case 4:
            if (oi == 0xF001u) {
                rc = _encode_bitmap(&s_file_session, buf, cap, &off);
            } else {
                return EDGE_SLAVE_SRC_CLASS_MISMATCH;
            }
            break;
        default:
            return EDGE_SLAVE_SRC_UNDEFINED;
    }

    if (rc != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *out_len = off;
    return 0;
}

int dcu_698_file_activate(const dcu_file_dfu_ops_t *ops, void *ctx) {
    /* 未注入真实 DFU 激活端口时诚实返回 DAR 5，绝不伪造成功。 */
    if (!ops || !ops->activate) return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    if (!s_file_session.active || !s_file_session.verified) return EDGE_SLAVE_SRC_TEMPORARY;
    return ops->activate(ctx);
}

int dcu_698_file_rollback(const dcu_file_dfu_ops_t *ops, void *ctx) {
    /* 未注入真实 DFU 回滚端口时诚实返回 DAR 5，绝不伪造成功。 */
    if (!ops || !ops->rollback) return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    return ops->rollback(ctx);
}
