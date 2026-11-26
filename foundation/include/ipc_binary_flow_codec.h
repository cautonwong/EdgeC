#ifndef EDGE_IPC_BINARY_FLOW_CODEC_H
#define EDGE_IPC_BINARY_FLOW_CODEC_H

#include <stddef.h>
#include <stdint.h>

#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* v3 stores blocks in analsys_dev_info_t.block_info[32]; larger values are rejected
 * instead of overflowing that array on the response path. */
#define IPC_BF_MAX_BLOCKS 32
/* v3 copies DevMes/CMD into 128-byte arrays without a length check. */
#define IPC_BF_MAX_DEV_LEN 128
#define IPC_BF_MAX_CMD_LEN 128
#define IPC_BF_BLOCK_HEADER_LEN 10

typedef enum {
    IPC_BF_U16 = 0,
    IPC_BF_S16,
    IPC_BF_U32,
    IPC_BF_S32,
    IPC_BF_FLOAT,
    IPC_BF_BI16 /* present in v3 config, no wire encoding */
} ipc_bf_type_t;

typedef struct {
    uint32_t trans_id;
    const uint8_t *dev; /* NULL when dev_len == 0 */
    uint8_t dev_len;
    const uint8_t *cmd; /* NULL when cmd_len == 0 */
    uint8_t cmd_len;
    uint16_t payload_len; /* decode output; ignored by encoders */
    uint8_t block_num;
} ipc_bf_header_t;

typedef struct {
    uint32_t block_id;
    uint16_t start_sub_id; /* 1-based */
    uint16_t sub_num;
    uint16_t data_len;   /* request/GetResp: Bytes; SetResp decode: ContentLen */
    const uint8_t *data; /* points into the caller's frame buffer */
    uint32_t error_code; /* SetResp encode only; block_next leaves 0 */
} ipc_bf_block_t;

typedef struct {
    const uint8_t *payload;
    size_t payload_len;
    size_t offset;
    uint8_t block_num;
    uint8_t index;
} ipc_bf_cursor_t;

/* Returns the header length in bytes, or -1 when the frame is malformed.
 * Requires PayloadLen == len - header length. */
int ipc_bf_decode_header(const uint8_t *buf, size_t len, ipc_bf_header_t *out,
                         ipc_bf_cursor_t *cur);

/* Returns 0 and fills one block, 1 when all blocks were read, -1 on malformed input.
 * A SetResp block decodes as data_len == ContentLen and data over CMD/error/reserved. */
int ipc_bf_block_next(ipc_bf_cursor_t *cur, ipc_bf_block_t *out);

/* Return 0 on success, -1 on malformed input or insufficient tailroom. */
int ipc_bf_encode_request(const ipc_bf_header_t *h, const ipc_bf_block_t *blocks, uint8_t n,
                          net_buf_t *out);
int ipc_bf_encode_get_resp(const ipc_bf_header_t *h, const ipc_bf_block_t *blocks, uint8_t n,
                           net_buf_t *out);
int ipc_bf_encode_set_resp(const ipc_bf_header_t *h, const ipc_bf_block_t *blocks, uint8_t n,
                           net_buf_t *out);

/* Wire width of a point; 0 for types without a wire encoding (BI16, unknown). */
size_t ipc_bf_point_size(ipc_bf_type_t t);
/* Return bytes written/read, or -1 for an unsupported type or short buffer. */
int ipc_bf_encode_point(ipc_bf_type_t t, const void *value, uint8_t *out, size_t cap);
int ipc_bf_decode_point(ipc_bf_type_t t, const uint8_t *in, size_t len, void *out);

#ifdef __cplusplus
}
#endif

#endif
