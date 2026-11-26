#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ipc_binary_flow_codec.h"
#include "net_buf.h"

#define TEST_BUF 512

enum {
    IPC_BF_TEST_HDR_DEV_LEN = 4,
    IPC_BF_TEST_HDR_DEV_OFF = 5,
    IPC_BF_TEST_HDR_CMD_LEN = 5,
    IPC_BF_TEST_HDR_PAYLOAD_LEN_LO = 7,
};

static const uint32_t k_neg_zero_bits = 0x80000000u;
static const uint32_t k_qnan_payload_bits = 0x7FC00001u;

static void _buf_init(net_buf_t *nb, uint8_t *mem, size_t cap) {
    memset(nb, 0, sizeof(*nb));
    net_buf_simple_init_with_data(&nb->b, mem, cap);
}

static size_t _hex_to_bytes(const char *hex, uint8_t *out, size_t cap) {
    size_t n = 0;
    while (hex[0] != '\0' && hex[1] != '\0') {
        unsigned int v = 0;
        assert_int_equal(sscanf(hex, "%2x", &v), 1);
        assert_true(n < cap);
        out[n++] = (uint8_t)v;
        hex += 2;
    }
    return n;
}

typedef int (*encode_fn)(const ipc_bf_header_t *, const ipc_bf_block_t *, uint8_t, net_buf_t *);

static void _assert_encodes(encode_fn encode, const uint8_t *expect, size_t expect_len,
                            const ipc_bf_header_t *h, const ipc_bf_block_t *blocks, uint8_t n) {
    uint8_t mem[TEST_BUF];
    net_buf_t nb;
    _buf_init(&nb, mem, sizeof(mem));
    assert_int_equal(encode(h, blocks, n, &nb), 0);
    assert_int_equal(nb.b.len, expect_len);
    assert_memory_equal(nb.b.data, expect, expect_len);
}

static void test_d1_get_request(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes("0000406A0000000B01000080210001004A0000", frame, sizeof(frame));

    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(h.trans_id, 0x406A);
    assert_int_equal(h.dev_len, 0);
    assert_null(h.dev);
    assert_int_equal(h.cmd_len, 0);
    assert_null(h.cmd);
    assert_int_equal(h.payload_len, 11);
    assert_int_equal(h.block_num, 1);

    assert_int_equal(ipc_bf_block_next(&cur, &b), 0);
    assert_int_equal(b.block_id, 0x8021);
    assert_int_equal(b.start_sub_id, 1);
    assert_int_equal(b.sub_num, 74);
    assert_int_equal(b.data_len, 0);
    assert_non_null(b.data);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 1);

    _assert_encodes(ipc_bf_encode_request, frame, len, &h, &b, 1);
}

static void test_d2_get_resp(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes(
        "0000406A00000029010000800100010008001E"
        "00030000303900005BA0FFFFFC1800000000000013880000000000000309",
        frame, sizeof(frame));

    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(h.trans_id, 0x406A);
    assert_int_equal(h.payload_len, 41);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 0);
    assert_int_equal(b.block_id, 0x8001);
    assert_int_equal(b.start_sub_id, 1);
    assert_int_equal(b.sub_num, 8);
    assert_int_equal(b.data_len, 30);

    uint16_t u16 = 0;
    int32_t s32 = 0;
    assert_int_equal(ipc_bf_decode_point(IPC_BF_U16, b.data, 2, &u16), 2);
    assert_int_equal(u16, 3);
    const int32_t expected[] = {12345, 23456, -1000, 0, 5000, 0, 777};
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        assert_int_equal(ipc_bf_decode_point(IPC_BF_S32, b.data + 2 + 4 * i, 4, &s32), 4);
        assert_int_equal(s32, expected[i]);
    }

    uint8_t reencoded[30];
    size_t at = 0;
    assert_int_equal(ipc_bf_encode_point(IPC_BF_U16, &u16, reencoded + at, sizeof(reencoded) - at),
                     2);
    at += 2;
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        assert_int_equal(
            ipc_bf_encode_point(IPC_BF_S32, &expected[i], reencoded + at, sizeof(reencoded) - at),
            4);
        at += 4;
    }
    assert_int_equal(at, b.data_len);
    assert_memory_equal(reencoded, b.data, b.data_len);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 1);

    _assert_encodes(ipc_bf_encode_get_resp, frame, len, &h, &b, 1);
}

static void test_d3_set_request(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes("0000406A00000017010000802100010006000C000100010000000100010001",
                               frame, sizeof(frame));

    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(h.payload_len, 23);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 0);
    assert_int_equal(b.block_id, 0x8021);
    assert_int_equal(b.start_sub_id, 1);
    assert_int_equal(b.sub_num, 6);
    assert_int_equal(b.data_len, 12);

    const uint16_t expected[] = {1, 1, 0, 1, 1, 1};
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        uint16_t v = 0;
        assert_int_equal(ipc_bf_decode_point(IPC_BF_U16, b.data + 2 * i, 2, &v), 2);
        assert_int_equal(v, expected[i]);
    }
    assert_int_equal(ipc_bf_block_next(&cur, &b), 1);

    _assert_encodes(ipc_bf_encode_request, frame, len, &h, &b, 1);
}

static void test_d4_set_resp(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes("0000406A00000015010000802100010006000A00000000000000000000", frame,
                               sizeof(frame));

    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(h.payload_len, 21);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 0);
    assert_int_equal(b.block_id, 0x8021);
    assert_int_equal(b.start_sub_id, 1);
    assert_int_equal(b.sub_num, 6);
    assert_int_equal(b.data_len, 10);
    const uint8_t content[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    assert_memory_equal(b.data, content, sizeof(content));
    assert_int_equal(ipc_bf_block_next(&cur, &b), 1);

    b.error_code = 0;
    _assert_encodes(ipc_bf_encode_set_resp, frame, len, &h, &b, 1);
}

static void test_d6_get_resp_with_dev(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes(
        "000000030650435334353600002001000080130001000A0015000000000000000000000000"
        "000000000000000000",
        frame, sizeof(frame));

    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 14);
    assert_int_equal(h.trans_id, 3);
    assert_int_equal(h.dev_len, 6);
    assert_memory_equal(h.dev, "PCS456", 6);
    assert_int_equal(h.cmd_len, 0);
    assert_null(h.cmd);
    assert_int_equal(h.payload_len, 32);

    assert_int_equal(ipc_bf_block_next(&cur, &b), 0);
    assert_int_equal(b.block_id, 0x8013);
    assert_int_equal(b.start_sub_id, 1);
    assert_int_equal(b.sub_num, 10);
    assert_int_equal(b.data_len, 21);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 1);

    _assert_encodes(ipc_bf_encode_get_resp, frame, len, &h, &b, 1);
}

static void test_d7_set_request_mixed_types(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes(
        "0000406A0000002701000080210001000C001C0001000200030004000500060007000800"
        "09000A000004D2FFFFE9D2",
        frame, sizeof(frame));

    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(h.payload_len, 39);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 0);
    assert_int_equal(b.block_id, 0x8021);
    assert_int_equal(b.start_sub_id, 1);
    assert_int_equal(b.sub_num, 12);
    assert_int_equal(b.data_len, 28);

    for (uint16_t i = 0; i < 10; i++) {
        uint16_t v = 0;
        assert_int_equal(ipc_bf_decode_point(IPC_BF_U16, b.data + 2 * i, 2, &v), 2);
        assert_int_equal(v, i + 1);
    }
    int32_t s32 = 0;
    assert_int_equal(ipc_bf_decode_point(IPC_BF_S32, b.data + 20, 4, &s32), 4);
    assert_int_equal(s32, 1234);
    assert_int_equal(ipc_bf_decode_point(IPC_BF_S32, b.data + 24, 4, &s32), 4);
    assert_int_equal(s32, -5678);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 1);

    _assert_encodes(ipc_bf_encode_request, frame, len, &h, &b, 1);
}

/* Frames copied verbatim from v3 src/ems/mqtt_north.c comment samples. Each
 * decodes to the block the comment documents, then re-encodes byte-for-byte. */
static void _assert_v3_frame(const char *hex, encode_fn encode, uint32_t block_id,
                             uint16_t start_sub_id, uint16_t sub_num, uint16_t data_len) {
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes(hex, frame, sizeof(frame));
    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 0);
    assert_int_equal(b.block_id, block_id);
    assert_int_equal(b.start_sub_id, start_sub_id);
    assert_int_equal(b.sub_num, sub_num);
    assert_int_equal(b.data_len, data_len);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 1);
    _assert_encodes(encode, frame, len, &h, &b, 1);
}

static void test_v3_documented_frames(void **state) {
    (void)state;
    /* :687 Get, :673 Set, :198-214 Set, :689-701 GetResp, :584-596 BMS GetResp,
     * :648-658 actual SetResp. */
    _assert_v3_frame("000003370000000b01000080310001000A0000", ipc_bf_encode_request, 0x8031, 1, 10,
                     0);
    _assert_v3_frame("0000406a0000000d01000080310001000100020005", ipc_bf_encode_request, 0x8031, 1,
                     1, 2);
    _assert_v3_frame("0022D31C0000001B010000803100010006001000030000000000000000000000000001",
                     ipc_bf_encode_request, 0x8031, 1, 6, 16);
    _assert_v3_frame(
        "000003370000002301000080310001000A0018000300000000000000002774000000000000000000"
        "000000",
        ipc_bf_encode_get_resp, 0x8031, 1, 10, 24);
    _assert_v3_frame(
        "0013D4E80000002901000000021001001e001e02C00619049807A80620054E060903F2055D05FD0356076"
        "1035A06450555",
        ipc_bf_encode_get_resp, 2, 0x1001, 30, 30);
    _assert_v3_frame("0033547c00000015010000803100010006000a00000000000000000000",
                     ipc_bf_encode_set_resp, 0x8031, 1, 6, 10);
}

static void test_set_resp_unknown_block_keeps_layout(void **state) {
    (void)state;
    ipc_bf_header_t h;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&b, 0, sizeof(b));
    h.trans_id = 0x406A;
    h.block_num = 1;
    b.block_id = 0x9999;
    b.start_sub_id = 1;
    b.sub_num = 6;
    b.error_code = 1;

    uint8_t mem[TEST_BUF];
    net_buf_t nb;
    _buf_init(&nb, mem, sizeof(mem));
    assert_int_equal(ipc_bf_encode_set_resp(&h, &b, 1, &nb), 0);
    assert_int_equal(nb.b.len, 29);

    const uint8_t expect[] = {
        0x00, 0x00, 0x40, 0x6A, 0x00, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x99, 0x99, 0x00, 0x01,
        0x00, 0x06, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    };
    assert_memory_equal(nb.b.data, expect, sizeof(expect));
    assert_int_equal(nb.b.data[8], 1);
    assert_int_equal((size_t)(nb.b.data[6] << 8 | nb.b.data[7]), nb.b.len - 8);

    ipc_bf_header_t dh;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t db;
    memset(&dh, 0, sizeof(dh));
    memset(&cur, 0, sizeof(cur));
    memset(&db, 0, sizeof(db));
    assert_int_equal(ipc_bf_decode_header(nb.b.data, nb.b.len, &dh, &cur), 8);
    assert_int_equal(ipc_bf_block_next(&cur, &db), 0);
    assert_int_equal(db.block_id, 0x9999);
    assert_int_equal(db.data_len, 10);
    assert_int_equal(db.data[0], 0);
    assert_int_equal(db.data[1], 0);
    assert_int_equal(db.data[5], 1);
    assert_int_equal(db.data[9], 0);
}

static void test_point_vectors(void **state) {
    (void)state;
    uint8_t out[4];

    uint16_t u16 = 3;
    const uint8_t u16_wire[] = {0x00, 0x03};
    assert_int_equal(ipc_bf_point_size(IPC_BF_U16), 2);
    assert_int_equal(ipc_bf_encode_point(IPC_BF_U16, &u16, out, sizeof(out)), 2);
    assert_memory_equal(out, u16_wire, sizeof(u16_wire));
    uint16_t u16_back = 0;
    assert_int_equal(ipc_bf_decode_point(IPC_BF_U16, u16_wire, 2, &u16_back), 2);
    assert_int_equal(u16_back, 3);

    int16_t s16 = -1;
    const uint8_t s16_wire[] = {0xFF, 0xFF};
    assert_int_equal(ipc_bf_point_size(IPC_BF_S16), 2);
    assert_int_equal(ipc_bf_encode_point(IPC_BF_S16, &s16, out, sizeof(out)), 2);
    assert_memory_equal(out, s16_wire, sizeof(s16_wire));
    int16_t s16_back = 0;
    assert_int_equal(ipc_bf_decode_point(IPC_BF_S16, s16_wire, 2, &s16_back), 2);
    assert_int_equal(s16_back, -1);

    uint32_t u32 = 0x89ABCDEFu;
    const uint8_t u32_wire[] = {0x89, 0xAB, 0xCD, 0xEF};
    assert_int_equal(ipc_bf_point_size(IPC_BF_U32), 4);
    assert_int_equal(ipc_bf_encode_point(IPC_BF_U32, &u32, out, sizeof(out)), 4);
    assert_memory_equal(out, u32_wire, sizeof(u32_wire));
    uint32_t u32_back = 0;
    assert_int_equal(ipc_bf_decode_point(IPC_BF_U32, u32_wire, 4, &u32_back), 4);
    assert_int_equal(u32_back, 0x89ABCDEFu);

    int32_t s32 = -2;
    const uint8_t s32_wire[] = {0xFF, 0xFF, 0xFF, 0xFE};
    assert_int_equal(ipc_bf_point_size(IPC_BF_S32), 4);
    assert_int_equal(ipc_bf_encode_point(IPC_BF_S32, &s32, out, sizeof(out)), 4);
    assert_memory_equal(out, s32_wire, sizeof(s32_wire));
    int32_t s32_back = 0;
    assert_int_equal(ipc_bf_decode_point(IPC_BF_S32, s32_wire, 4, &s32_back), 4);
    assert_int_equal(s32_back, -2);

    float f = 1.5f;
    const uint8_t f_wire[] = {0x3F, 0xC0, 0x00, 0x00};
    assert_int_equal(ipc_bf_point_size(IPC_BF_FLOAT), 4);
    assert_int_equal(ipc_bf_encode_point(IPC_BF_FLOAT, &f, out, sizeof(out)), 4);
    assert_memory_equal(out, f_wire, sizeof(f_wire));
    float f_back = 0.0f;
    assert_int_equal(ipc_bf_decode_point(IPC_BF_FLOAT, f_wire, 4, &f_back), 4);
    assert_true(f_back == 1.5f);

    float f2 = -2.25f;
    const uint8_t f2_wire[] = {0xC0, 0x10, 0x00, 0x00};
    assert_int_equal(ipc_bf_encode_point(IPC_BF_FLOAT, &f2, out, sizeof(out)), 4);
    assert_memory_equal(out, f2_wire, sizeof(f2_wire));
    assert_int_equal(ipc_bf_decode_point(IPC_BF_FLOAT, f2_wire, 4, &f_back), 4);
    assert_true(f_back == -2.25f);
}

static void test_float_bits_preserved(void **state) {
    (void)state;
    uint8_t out[4];
    uint8_t back[4];
    uint32_t bits = k_neg_zero_bits;
    float f;
    memcpy(&f, &bits, 4);
    assert_int_equal(ipc_bf_encode_point(IPC_BF_FLOAT, &f, out, sizeof(out)), 4);
    const uint8_t neg_zero_wire[] = {0x80, 0x00, 0x00, 0x00};
    assert_memory_equal(out, neg_zero_wire, sizeof(neg_zero_wire));
    assert_int_equal(ipc_bf_decode_point(IPC_BF_FLOAT, neg_zero_wire, 4, &f), 4);
    memcpy(&bits, &f, 4);
    assert_int_equal(bits, k_neg_zero_bits);

    bits = k_qnan_payload_bits;
    memcpy(&f, &bits, 4);
    assert_int_equal(ipc_bf_encode_point(IPC_BF_FLOAT, &f, out, sizeof(out)), 4);
    const uint8_t nan_wire[] = {0x7F, 0xC0, 0x00, 0x01};
    assert_memory_equal(out, nan_wire, sizeof(nan_wire));
    assert_int_equal(ipc_bf_decode_point(IPC_BF_FLOAT, nan_wire, 4, back), 4);
    memcpy(&bits, back, 4);
    assert_int_equal(bits, k_qnan_payload_bits);
}

static void test_bi16_and_unknown_type_rejected(void **state) {
    (void)state;
    uint16_t value = 1;
    uint16_t got = 0;
    uint8_t out[4];
    assert_int_equal(ipc_bf_point_size(IPC_BF_BI16), 0);
    assert_int_equal(ipc_bf_encode_point(IPC_BF_BI16, &value, out, sizeof(out)), -1);
    assert_int_equal(ipc_bf_decode_point(IPC_BF_BI16, out, 2, &got), -1);

    ipc_bf_type_t unknown = (ipc_bf_type_t)99;
    assert_int_equal(ipc_bf_point_size(unknown), 0);
    assert_int_equal(ipc_bf_encode_point(unknown, &value, out, sizeof(out)), -1);
    assert_int_equal(ipc_bf_decode_point(unknown, out, 2, &got), -1);
}

static void test_payload_len_mismatch(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes("0000406A0000000101", frame, sizeof(frame));
    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));

    const uint8_t payload_len = frame[IPC_BF_TEST_HDR_PAYLOAD_LEN_LO];
    frame[IPC_BF_TEST_HDR_PAYLOAD_LEN_LO] = (uint8_t)(payload_len + 1);
    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), -1);

    frame[IPC_BF_TEST_HDR_PAYLOAD_LEN_LO] = (uint8_t)(payload_len - 1);
    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), -1);

    frame[IPC_BF_TEST_HDR_PAYLOAD_LEN_LO] = payload_len;
    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
}

static void test_truncated_block_header(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes("0000406A00000006011122334455", frame, sizeof(frame));
    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(ipc_bf_block_next(&cur, &b), -1);
}

static void test_data_len_overrun(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes("0000406A0000000D0100008021000100010004AABB", frame, sizeof(frame));
    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(ipc_bf_block_next(&cur, &b), -1);
}

static void test_block_num_invalid(void **state) {
    (void)state;
    uint8_t frame[TEST_BUF];
    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));
    memset(&b, 0, sizeof(b));

    size_t len = _hex_to_bytes("0000406A0000000100", frame, sizeof(frame));
    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), -1);

    len = _hex_to_bytes("0000406A0000000121", frame, sizeof(frame));
    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), -1);

    len = _hex_to_bytes("0000406A0000000B0200008021000100010000", frame, sizeof(frame));
    assert_int_equal(ipc_bf_decode_header(frame, len, &h, &cur), 8);
    assert_int_equal(ipc_bf_block_next(&cur, &b), 0);
    assert_int_equal(ipc_bf_block_next(&cur, &b), -1);
}

static void test_header_field_limits(void **state) {
    (void)state;
    uint8_t frame[300];
    ipc_bf_header_t h;
    ipc_bf_cursor_t cur;
    memset(&h, 0, sizeof(h));
    memset(&cur, 0, sizeof(cur));

    enum { FIELD_CAP_PROBE = 255 };
    const size_t dev_len_payload_off = IPC_BF_TEST_HDR_DEV_OFF + FIELD_CAP_PROBE + 1 + 2;
    const size_t cmd_len_payload_off = IPC_BF_TEST_HDR_DEV_OFF + 1 + FIELD_CAP_PROBE + 2;

    memset(frame, 0, sizeof(frame));
    frame[3] = 1;
    frame[IPC_BF_TEST_HDR_DEV_LEN] = FIELD_CAP_PROBE;
    frame[IPC_BF_TEST_HDR_DEV_OFF + FIELD_CAP_PROBE] = 0;
    frame[dev_len_payload_off - 2] = 0;
    frame[dev_len_payload_off - 1] = (uint8_t)(sizeof(frame) - dev_len_payload_off);
    frame[dev_len_payload_off] = 1;
    assert_int_equal(ipc_bf_decode_header(frame, sizeof(frame), &h, &cur), -1);

    memset(frame, 0, sizeof(frame));
    frame[3] = 1;
    frame[IPC_BF_TEST_HDR_CMD_LEN] = FIELD_CAP_PROBE;
    frame[cmd_len_payload_off - 2] = 0;
    frame[cmd_len_payload_off - 1] = (uint8_t)(sizeof(frame) - cmd_len_payload_off);
    frame[cmd_len_payload_off] = 1;
    assert_int_equal(ipc_bf_decode_header(frame, sizeof(frame), &h, &cur), -1);
}

static void test_start_sub_id_zero_rejected(void **state) {
    (void)state;
    ipc_bf_header_t h;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&b, 0, sizeof(b));
    h.trans_id = 0x406A;
    h.block_num = 1;
    b.block_id = 0x8021;
    b.start_sub_id = 0;
    b.sub_num = 1;

    uint8_t mem[TEST_BUF];
    net_buf_t nb;
    _buf_init(&nb, mem, sizeof(mem));
    assert_int_equal(ipc_bf_encode_request(&h, &b, 1, &nb), -1);
    assert_int_equal(nb.b.len, 0);

    uint8_t frame[TEST_BUF];
    size_t len = _hex_to_bytes("0000406A0000000B01000080210001004A0000", frame, sizeof(frame));
    frame[13] = 0;
    frame[14] = 0;
    ipc_bf_header_t dh;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t db;
    memset(&dh, 0, sizeof(dh));
    memset(&cur, 0, sizeof(cur));
    memset(&db, 0, sizeof(db));
    assert_int_equal(ipc_bf_decode_header(frame, len, &dh, &cur), 8);
    assert_int_equal(ipc_bf_block_next(&cur, &db), -1);
}

static void test_sub_num_zero_returns_empty_block(void **state) {
    (void)state;
    ipc_bf_header_t h;
    ipc_bf_block_t b;
    memset(&h, 0, sizeof(h));
    memset(&b, 0, sizeof(b));
    h.trans_id = 0x406A;
    h.block_num = 1;
    b.block_id = 0x8021;
    b.start_sub_id = 1;
    b.sub_num = 0;
    b.data_len = 0;

    uint8_t mem[TEST_BUF];
    net_buf_t nb;
    _buf_init(&nb, mem, sizeof(mem));
    assert_int_equal(ipc_bf_encode_request(&h, &b, 1, &nb), 0);
    const uint8_t expect[] = {0x00, 0x00, 0x40, 0x6A, 0x00, 0x00, 0x00, 0x0B, 0x01, 0x00,
                              0x00, 0x80, 0x21, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    assert_int_equal(nb.b.len, sizeof(expect));
    assert_memory_equal(nb.b.data, expect, sizeof(expect));

    ipc_bf_header_t dh;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t db;
    memset(&dh, 0, sizeof(dh));
    memset(&cur, 0, sizeof(cur));
    memset(&db, 0, sizeof(db));
    assert_int_equal(ipc_bf_decode_header(nb.b.data, nb.b.len, &dh, &cur), 8);
    assert_int_equal(ipc_bf_block_next(&cur, &db), 0);
    assert_int_equal(db.block_id, 0x8021);
    assert_int_equal(db.start_sub_id, 1);
    assert_int_equal(db.sub_num, 0);
    assert_int_equal(db.data_len, 0);
    assert_non_null(db.data);
}

static void test_encode_rejects_invalid_arguments(void **state) {
    (void)state;
    ipc_bf_header_t h;
    ipc_bf_block_t b;
    uint8_t mem[TEST_BUF];
    net_buf_t nb;
    memset(&h, 0, sizeof(h));
    memset(&b, 0, sizeof(b));
    h.trans_id = 0x406A;
    h.block_num = 1;
    b.block_id = 0x8021;
    b.start_sub_id = 1;
    b.sub_num = 1;
    b.data_len = 0;

    _buf_init(&nb, mem, sizeof(mem));
    assert_int_equal(ipc_bf_encode_request(NULL, &b, 1, &nb), -1);
    assert_int_equal(ipc_bf_encode_request(&h, NULL, 1, &nb), -1);
    assert_int_equal(ipc_bf_encode_request(&h, &b, 0, &nb), -1);
    h.block_num = 2;
    assert_int_equal(ipc_bf_encode_request(&h, &b, 1, &nb), -1);

    h.block_num = 1;
    b.data_len = 2;
    b.data = NULL;
    assert_int_equal(ipc_bf_encode_request(&h, &b, 1, &nb), -1);
    b.data_len = 0;
    b.data = NULL;

    const uint8_t dev[130] = {0};
    h.dev = dev;
    h.dev_len = 130;
    assert_int_equal(ipc_bf_encode_request(&h, &b, 1, &nb), -1);
    h.dev = NULL;
    h.dev_len = 0;

    uint8_t small[8];
    net_buf_t tiny;
    _buf_init(&tiny, small, sizeof(small));
    assert_int_equal(ipc_bf_encode_request(&h, &b, 1, &tiny), -1);
    assert_int_equal(tiny.b.len, 0);

    const ipc_bf_block_t many[33] = {{0}};
    assert_int_equal(ipc_bf_encode_request(&h, many, 33, &nb), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_d1_get_request),
        cmocka_unit_test(test_d2_get_resp),
        cmocka_unit_test(test_d3_set_request),
        cmocka_unit_test(test_d4_set_resp),
        cmocka_unit_test(test_d6_get_resp_with_dev),
        cmocka_unit_test(test_d7_set_request_mixed_types),
        cmocka_unit_test(test_v3_documented_frames),
        cmocka_unit_test(test_set_resp_unknown_block_keeps_layout),
        cmocka_unit_test(test_point_vectors),
        cmocka_unit_test(test_float_bits_preserved),
        cmocka_unit_test(test_bi16_and_unknown_type_rejected),
        cmocka_unit_test(test_payload_len_mismatch),
        cmocka_unit_test(test_truncated_block_header),
        cmocka_unit_test(test_data_len_overrun),
        cmocka_unit_test(test_block_num_invalid),
        cmocka_unit_test(test_header_field_limits),
        cmocka_unit_test(test_start_sub_id_zero_rejected),
        cmocka_unit_test(test_sub_num_zero_returns_empty_block),
        cmocka_unit_test(test_encode_rejects_invalid_arguments),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
