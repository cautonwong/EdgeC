#include "ws_proto.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// --- 内置 Base64 编码实现 ---
static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void base64_encode(const uint8_t *src, size_t len, char *dst) {
    size_t i, j;
    for (i = 0, j = 0; i < len; i += 3) {
        uint32_t v = (uint32_t)src[i] << 16;
        if (i + 1 < len) v |= (uint32_t)src[i + 1] << 8;
        if (i + 2 < len) v |= (uint32_t)src[i + 2];

        dst[j++] = b64_table[(v >> 18) & 0x3f];
        dst[j++] = b64_table[(v >> 12) & 0x3f];
        dst[j++] = (i + 1 < len) ? b64_table[(v >> 6) & 0x3f] : '=';
        dst[j++] = (i + 2 < len) ? b64_table[v & 0x3f] : '=';
    }
    dst[j] = '\0';
}

// --- 内置 SHA1 算法实现 (精简版) ---
typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t buffer[64];
} sha1_ctx_t;

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static void sha1_transform(uint32_t state[5], const uint8_t buffer[64]) {
    uint32_t a, b, c, d, e, i, w[80];

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)buffer[i * 4] << 24) | ((uint32_t)buffer[i * 4 + 1] << 16) |

               ((uint32_t)buffer[i * 4 + 2] << 8) | (uint32_t)buffer[i * 4 + 3];
    }

    for (; i < 80; i++) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    for (i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t temp = rol(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rol(b, 30);
        b = a;
        a = temp;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void sha1_init(sha1_ctx_t *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = ctx->count[1] = 0;
}

static void sha1_update(sha1_ctx_t *ctx, const uint8_t *data, uint32_t len) {
    uint32_t i, j;
    j = (ctx->count[0] >> 3) & 63;
    if ((ctx->count[0] += len << 3) < (len << 3)) ctx->count[1]++;
    ctx->count[1] += (len >> 29);
    if ((j + len) > 63) {
        memcpy(&ctx->buffer[j], data, (i = 64 - j));
        sha1_transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64) sha1_transform(ctx->state, &data[i]);
        j = 0;
    } else
        i = 0;
    memcpy(&ctx->buffer[j], &data[i], len - i);
}

static void sha1_final(sha1_ctx_t *ctx, uint8_t digest[20]) {
    uint8_t finalcount[8];
    for (int i = 0; i < 8; i++)
        finalcount[i] = (uint8_t)((ctx->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);
    sha1_update(ctx, (const uint8_t *)"\x80", 1);
    while ((ctx->count[0] & 504) != 448) sha1_update(ctx, (const uint8_t *)"\0", 1);
    sha1_update(ctx, finalcount, 8);
    for (int i = 0; i < 20; i++)
        digest[i] = (uint8_t)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
}

// --- WebSocket API ---

int ws_gen_accept_key(const char *client_key, char *out_accept) {
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", client_key, WS_GUID);

    sha1_ctx_t ctx;
    uint8_t hash[20];
    sha1_init(&ctx);
    sha1_update(&ctx, (const uint8_t *)combined, (uint32_t)strlen(combined));
    sha1_final(&ctx, hash);

    base64_encode(hash, 20, out_accept);
    return 0;
}

int ws_build_frame(ws_opcode_t opcode, const uint8_t *payload, size_t len, uint8_t *out_frame,
                   size_t *out_len) {
    int head_len = 0;
    out_frame[0] = (uint8_t)(0x80 | opcode);  // FIN = 1

    if (len <= 125) {
        out_frame[1] = (uint8_t)len;
        head_len = 2;
    } else if (len <= 65535) {
        out_frame[1] = 126;
        out_frame[2] = (uint8_t)((len >> 8) & 0xFF);
        out_frame[3] = (uint8_t)(len & 0xFF);
        head_len = 4;
    } else {
        out_frame[1] = 127;
        for (int i = 0; i < 8; i++) {
            out_frame[2 + i] = (uint8_t)((((uint64_t)len) >> (56 - i * 8)) & 0xFF);
        }
        head_len = 10;
    }

    if (payload && len > 0) {
        memcpy(out_frame + head_len, payload, len);
    }
    *out_len = (size_t)head_len + len;
    return 0;
}

int ws_serialize_double_array(uint32_t tag_id, const double *arr, uint32_t count, uint8_t *out_buf, size_t *out_len) {
    if (!arr || !out_buf || !out_len) return -1;
    
    ws_bin_header_t hdr;
    hdr.tag_id = tag_id;
    hdr.element_count = count;
    hdr.data_type = 0x01; // Double
    
    size_t payload_size = count * sizeof(double);
    memcpy(out_buf, &hdr, sizeof(ws_bin_header_t));
    memcpy(out_buf + sizeof(ws_bin_header_t), arr, payload_size);
    
    *out_len = sizeof(ws_bin_header_t) + payload_size;
    return 0;
}
