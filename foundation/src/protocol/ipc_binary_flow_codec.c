#include "ipc_binary_flow_codec.h"

#include <string.h>

static uint16_t _be16(const uint8_t *p) {
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t _be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void _put_be16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)v;
}

static void _put_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

int ipc_bf_decode_header(const uint8_t *buf, size_t len, ipc_bf_header_t *out,
                         ipc_bf_cursor_t *cur) {
    if (!buf || !out || !cur) return -1;
    if (len < 8) return -1;

    uint8_t dev_len = buf[4];
    if (dev_len > IPC_BF_MAX_DEV_LEN) return -1;
    size_t dev_off = 5;
    size_t cmd_len_off = dev_off + (size_t)dev_len;
    if (len < cmd_len_off + 1) return -1;

    uint8_t cmd_len = buf[cmd_len_off];
    if (cmd_len > IPC_BF_MAX_CMD_LEN) return -1;
    size_t cmd_off = cmd_len_off + 1;
    size_t payload_off = cmd_off + (size_t)cmd_len + 2u;
    if (len < payload_off) return -1;

    uint16_t payload_len = _be16(buf + cmd_off + cmd_len);
    if ((size_t)payload_len != len - payload_off) return -1;
    if (payload_len < 1) return -1;

    uint8_t block_num = buf[payload_off];
    if (block_num == 0 || block_num > IPC_BF_MAX_BLOCKS) return -1;

    out->trans_id = _be32(buf);
    out->dev = dev_len ? buf + dev_off : NULL;
    out->dev_len = dev_len;
    out->cmd = cmd_len ? buf + cmd_off : NULL;
    out->cmd_len = cmd_len;
    out->payload_len = payload_len;
    out->block_num = block_num;

    cur->payload = buf + payload_off;
    cur->payload_len = payload_len;
    cur->offset = 1;
    cur->block_num = block_num;
    cur->index = 0;

    return (int)payload_off;
}

int ipc_bf_block_next(ipc_bf_cursor_t *cur, ipc_bf_block_t *out) {
    if (!cur || !out) return -1;
    if (cur->index >= cur->block_num) return 1;
    if (!cur->payload) return -1;
    if (cur->offset > cur->payload_len) return -1;
    if (cur->payload_len - cur->offset < IPC_BF_BLOCK_HEADER_LEN) return -1;

    const uint8_t *p = cur->payload + cur->offset;
    uint16_t start_sub_id = _be16(p + 4);
    if (start_sub_id == 0) return -1;
    uint16_t data_len = _be16(p + 8);

    size_t data_off = cur->offset + IPC_BF_BLOCK_HEADER_LEN;
    if ((size_t)data_len > cur->payload_len - data_off) return -1;

    out->block_id = _be32(p);
    out->start_sub_id = start_sub_id;
    out->sub_num = _be16(p + 6);
    out->data_len = data_len;
    out->data = cur->payload + data_off;
    out->error_code = 0;

    cur->offset = data_off + (size_t)data_len;
    cur->index++;
    return 0;
}

static int _encode_frame(const ipc_bf_header_t *h, const ipc_bf_block_t *blocks, uint8_t n,
                         net_buf_t *out, int set_resp) {
    if (!h || !out) return -1;
    if (n == 0 || n > IPC_BF_MAX_BLOCKS || h->block_num != n) return -1;
    if (!blocks) return -1;
    if (h->dev_len > IPC_BF_MAX_DEV_LEN || h->cmd_len > IPC_BF_MAX_CMD_LEN) return -1;
    if ((h->dev_len && !h->dev) || (h->cmd_len && !h->cmd)) return -1;

    size_t payload_len = 1;
    for (uint8_t i = 0; i < n; i++) {
        const ipc_bf_block_t *b = &blocks[i];
        if (b->start_sub_id == 0) return -1;
        size_t data_len = set_resp ? 10u : (size_t)b->data_len;
        if (!set_resp && b->data_len && !b->data) return -1;
        payload_len += IPC_BF_BLOCK_HEADER_LEN + data_len;
    }
    if (payload_len > (size_t)UINT16_MAX) return -1;

    size_t frame_len = 8u + (size_t)h->dev_len + (size_t)h->cmd_len + payload_len;
    if (net_buf_simple_tailroom(&out->b) < frame_len) return -1;

    uint8_t *p = net_buf_simple_add(&out->b, frame_len);
    size_t idx = 0;
    _put_be32(p + idx, h->trans_id);
    idx += 4;
    p[idx++] = h->dev_len;
    if (h->dev_len) {
        memcpy(p + idx, h->dev, h->dev_len);
        idx += h->dev_len;
    }
    p[idx++] = h->cmd_len;
    if (h->cmd_len) {
        memcpy(p + idx, h->cmd, h->cmd_len);
        idx += h->cmd_len;
    }
    _put_be16(p + idx, (uint16_t)payload_len);
    idx += 2;
    p[idx++] = n;

    for (uint8_t i = 0; i < n; i++) {
        const ipc_bf_block_t *b = &blocks[i];
        _put_be32(p + idx, b->block_id);
        idx += 4;
        _put_be16(p + idx, b->start_sub_id);
        idx += 2;
        _put_be16(p + idx, b->sub_num);
        idx += 2;
        if (set_resp) {
            _put_be16(p + idx, 10);
            idx += 2;
            _put_be16(p + idx, 0);
            idx += 2;
            _put_be32(p + idx, b->error_code);
            idx += 4;
            _put_be32(p + idx, 0);
            idx += 4;
        } else {
            _put_be16(p + idx, b->data_len);
            idx += 2;
            if (b->data_len) {
                memcpy(p + idx, b->data, b->data_len);
                idx += b->data_len;
            }
        }
    }
    return 0;
}

int ipc_bf_encode_request(const ipc_bf_header_t *h, const ipc_bf_block_t *blocks, uint8_t n,
                          net_buf_t *out) {
    return _encode_frame(h, blocks, n, out, 0);
}

int ipc_bf_encode_get_resp(const ipc_bf_header_t *h, const ipc_bf_block_t *blocks, uint8_t n,
                           net_buf_t *out) {
    return _encode_frame(h, blocks, n, out, 0);
}

int ipc_bf_encode_set_resp(const ipc_bf_header_t *h, const ipc_bf_block_t *blocks, uint8_t n,
                           net_buf_t *out) {
    return _encode_frame(h, blocks, n, out, 1);
}

size_t ipc_bf_point_size(ipc_bf_type_t t) {
    switch (t) {
        case IPC_BF_U16:
        case IPC_BF_S16:
            return 2;
        case IPC_BF_U32:
        case IPC_BF_S32:
        case IPC_BF_FLOAT:
            return 4;
        case IPC_BF_BI16:
            return 0;
    }
    return 0;
}

int ipc_bf_encode_point(ipc_bf_type_t t, const void *value, uint8_t *out, size_t cap) {
    if (!value || !out) return -1;
    size_t size = ipc_bf_point_size(t);
    if (size == 0 || cap < size) return -1;

    switch (t) {
        case IPC_BF_U16: {
            uint16_t v;
            memcpy(&v, value, 2);
            _put_be16(out, v);
            break;
        }
        case IPC_BF_S16: {
            int16_t v;
            memcpy(&v, value, 2);
            _put_be16(out, (uint16_t)v);
            break;
        }
        case IPC_BF_U32: {
            uint32_t v;
            memcpy(&v, value, 4);
            _put_be32(out, v);
            break;
        }
        case IPC_BF_S32: {
            int32_t v;
            memcpy(&v, value, 4);
            _put_be32(out, (uint32_t)v);
            break;
        }
        case IPC_BF_FLOAT: {
            uint32_t bits;
            memcpy(&bits, value, 4);
            _put_be32(out, bits);
            break;
        }
        default:
            return -1;
    }
    return (int)size;
}

int ipc_bf_decode_point(ipc_bf_type_t t, const uint8_t *in, size_t len, void *out) {
    if (!in || !out) return -1;
    size_t size = ipc_bf_point_size(t);
    if (size == 0 || len < size) return -1;

    switch (t) {
        case IPC_BF_U16: {
            uint16_t v = _be16(in);
            memcpy(out, &v, 2);
            break;
        }
        case IPC_BF_S16: {
            int16_t v = (int16_t)_be16(in);
            memcpy(out, &v, 2);
            break;
        }
        case IPC_BF_U32: {
            uint32_t v = _be32(in);
            memcpy(out, &v, 4);
            break;
        }
        case IPC_BF_S32: {
            int32_t v = (int32_t)_be32(in);
            memcpy(out, &v, 4);
            break;
        }
        case IPC_BF_FLOAT: {
            uint32_t bits = _be32(in);
            float f;
            memcpy(&f, &bits, 4);
            memcpy(out, &f, 4);
            break;
        }
        default:
            return -1;
    }
    return (int)size;
}
