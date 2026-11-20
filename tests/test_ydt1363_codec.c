/**
 * @file test_ydt1363_codec.c
 * @brief YD/T 1363.3 纯编解码内核的字节级向量测试
 *
 * 只断言线上字节与返回值：没有驱动、点表、模板映射。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "net_buf.h"
#include "ydt1363_codec.h"

#define BUF_CAP 4200

/* 请求帧，INFO 为空：
 * LENID=0 → LCHKSUM=0，LENGTH="0000"；
 * CHKSUM("220146420000") = ~0x255 + 1 = 0xFDAB */
static const uint8_t kFrameEmpty[] = "~220146420000FDAB\r";
/* 应答帧（CID2/RTN=0），INFO={0x01,0x23}：
 * LENID=4 → LCHKSUM=C，LENGTH="C004"；CHKSUM = ~0x32C + 1 = 0xFCD4 */
static const uint8_t kFrameInfo[] = "~22014600C0040123FCD4\r";

static uint8_t g_out[BUF_CAP];
static uint8_t g_info[BUF_CAP];
static uint8_t g_dec[BUF_CAP];

static void mk_buf(net_buf_t *nb, uint8_t *mem, size_t cap) {
    memset(nb, 0, sizeof(*nb));
    net_buf_simple_init_with_data(&nb->b, mem, cap);
}

static void test_encode_empty_info_bytes(void **state) {
    (void)state;
    net_buf_t nb;
    mk_buf(&nb, g_out, sizeof(g_out));
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x42, NULL, 0, &nb), YDT1363_OK);

    assert_int_equal(nb.b.len, 18);
    assert_memory_equal(nb.b.data, kFrameEmpty, 18);
}

static void test_encode_info_bytes(void **state) {
    (void)state;
    const uint8_t info[] = {0x01, 0x23};
    net_buf_t nb;
    mk_buf(&nb, g_out, sizeof(g_out));
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x00, info, sizeof(info), &nb), YDT1363_OK);

    assert_int_equal(nb.b.len, 22);
    assert_memory_equal(nb.b.data, kFrameInfo, 22);
}

static void test_encode_rejects_oversize(void **state) {
    (void)state;
    net_buf_t nb;
    mk_buf(&nb, g_out, sizeof(g_out));

    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x00, g_info, 2048, &nb), YDT1363_ERR_SIZE);
    assert_int_equal(nb.b.len, 0);
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x00, g_info, 2049, &nb), YDT1363_ERR_SIZE);
    assert_int_equal(nb.b.len, 0);
}

static void test_encode_rejects_no_tailroom(void **state) {
    (void)state;
    net_buf_t nb;

    mk_buf(&nb, g_out, 17);
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x42, NULL, 0, &nb), YDT1363_ERR_SIZE);
    assert_int_equal(nb.b.len, 0);

    mk_buf(&nb, g_out, 18);
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x42, NULL, 0, &nb), YDT1363_OK);
    assert_int_equal(nb.b.len, 18);
}

static void test_encode_arg_errors(void **state) {
    (void)state;
    net_buf_t nb;
    mk_buf(&nb, g_out, sizeof(g_out));

    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x00, NULL, 1, &nb), YDT1363_ERR_ARG);
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x00, NULL, 0, NULL), YDT1363_ERR_ARG);
}

/* 2047 是 LENID 12 位能装下的最大 INFO；2048 会被静默截断，必须拒绝 */
static void test_max_info_roundtrip(void **state) {
    (void)state;
    for (size_t i = 0; i < YDT1363_MAX_INFO; i++) {
        g_info[i] = (uint8_t)(i * 7 + 1);
    }

    net_buf_t nb;
    mk_buf(&nb, g_out, sizeof(g_out));
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x00, g_info, YDT1363_MAX_INFO, &nb),
                     YDT1363_OK);
    assert_int_equal(nb.b.len, 18 + 2 * YDT1363_MAX_INFO);
    assert_int_equal(ydt1363_frame_len(nb.b.data, nb.b.len), 18 + 2 * YDT1363_MAX_INFO);

    size_t n = 0;
    uint8_t rtn = 0xFF;
    assert_int_equal(ydt1363_decode(nb.b.data, nb.b.len, g_dec, YDT1363_MAX_INFO, &n, &rtn),
                     YDT1363_OK);
    assert_int_equal(n, YDT1363_MAX_INFO);
    assert_int_equal(rtn, YDT1363_RTN_NORMAL);
    assert_memory_equal(g_dec, g_info, YDT1363_MAX_INFO);
}

static void test_decode_ok(void **state) {
    (void)state;
    size_t n = 0xAA;
    uint8_t rtn = 0xAA;
    assert_int_equal(
        ydt1363_decode(kFrameInfo, sizeof(kFrameInfo) - 1, g_info, sizeof(g_info), &n, &rtn),
        YDT1363_OK);
    assert_int_equal(n, 2);
    assert_int_equal(rtn, YDT1363_RTN_NORMAL);
    assert_int_equal(g_info[0], 0x01);
    assert_int_equal(g_info[1], 0x23);
}

static void test_decode_empty_info(void **state) {
    (void)state;
    net_buf_t nb;
    mk_buf(&nb, g_out, sizeof(g_out));
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x00, NULL, 0, &nb), YDT1363_OK);

    size_t n = 0xAA;
    uint8_t rtn = 0xAA;
    assert_int_equal(ydt1363_decode(nb.b.data, nb.b.len, NULL, 0, &n, &rtn), YDT1363_OK);
    assert_int_equal(n, 0);
    assert_int_equal(rtn, YDT1363_RTN_NORMAL);
}

/* RTN 非 0：帧本身完好，返回错误但必须给出 RTN，INFO 不复制 */
static void test_decode_rtn_error(void **state) {
    (void)state;
    net_buf_t nb;
    mk_buf(&nb, g_out, sizeof(g_out));
    assert_int_equal(ydt1363_encode(0x22, 0x01, 0x46, 0x02, NULL, 0, &nb), YDT1363_OK);

    size_t n = 0xAA;
    uint8_t rtn = 0;
    assert_int_equal(ydt1363_decode(nb.b.data, nb.b.len, g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_RTN);
    assert_int_equal(rtn, 0x02);
    assert_int_equal(n, 0);
}

static void test_decode_bad_lchksum(void **state) {
    (void)state;
    uint8_t frame[18];
    memcpy(frame, kFrameEmpty, sizeof(frame));
    frame[9] = '1'; /* LENGTH="1000"：LCHKSUM=1，与 LENID=0 不符 */

    size_t n = 0;
    uint8_t rtn = 0;
    assert_int_equal(ydt1363_decode(frame, sizeof(frame), g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_CHECKSUM);
}

static void test_decode_bad_chksum(void **state) {
    (void)state;
    uint8_t frame[18];
    memcpy(frame, kFrameEmpty, sizeof(frame));
    frame[13] = '0'; /* CHKSUM="0DAB" */

    size_t n = 0;
    uint8_t rtn = 0;
    assert_int_equal(ydt1363_decode(frame, sizeof(frame), g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_CHECKSUM);
}

static void test_decode_missing_eoi(void **state) {
    (void)state;
    uint8_t frame[18];
    memcpy(frame, kFrameEmpty, sizeof(frame));
    frame[17] = 0x00;

    size_t n = 0;
    uint8_t rtn = 0;
    assert_int_equal(ydt1363_decode(frame, sizeof(frame), g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_FORMAT);
}

/* LENID 必须是偶数（INFO 按整字节成对编码）：LENID=3，LCHKSUM=D 正确 */
static void test_decode_odd_lenid(void **state) {
    (void)state;
    static const uint8_t frame[] = "~22014644D0030120000\r";

    size_t n = 0;
    uint8_t rtn = 0;
    assert_int_equal(ydt1363_decode(frame, sizeof(frame) - 1, g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_FORMAT);
}

static void test_decode_non_hex(void **state) {
    (void)state;
    uint8_t frame[22];
    size_t n = 0;
    uint8_t rtn = 0;

    memcpy(frame, kFrameInfo, sizeof(frame));
    frame[14] = 'G'; /* INFO 首字节的 ASCII */
    assert_int_equal(ydt1363_decode(frame, sizeof(frame), g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_FORMAT);

    memcpy(frame, kFrameInfo, sizeof(frame));
    frame[2] = 'g'; /* VER 的 ASCII */
    assert_int_equal(ydt1363_decode(frame, sizeof(frame), g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_FORMAT);
}

/* LENID=0 却带 4 个 INFO 字符：声明长度与实际缓冲不符 */
static void test_decode_len_mismatch(void **state) {
    (void)state;
    static const uint8_t frame[] = "~22014642000001230000\r";

    size_t n = 0;
    uint8_t rtn = 0;
    assert_int_equal(ydt1363_decode(frame, sizeof(frame) - 1, g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_FORMAT);
}

static void test_decode_info_over_cap(void **state) {
    (void)state;
    memset(g_info, 0xAA, sizeof(g_info));

    size_t n = 0xAA;
    uint8_t rtn = 0xAA;
    assert_int_equal(ydt1363_decode(kFrameInfo, sizeof(kFrameInfo) - 1, g_info, 1, &n, &rtn),
                     YDT1363_ERR_SIZE);
    assert_int_equal(n, 0);
    assert_int_equal(g_info[0], 0xAA); /* 不截断、不写半个 INFO */
}

/* 拒绝路径也要把输出写成确定值，调用方不会读到未初始化数据 */
static void test_decode_error_outputs_defined(void **state) {
    (void)state;
    uint8_t frame[18];
    memcpy(frame, kFrameEmpty, sizeof(frame));
    frame[13] = '0';

    memset(g_info, 0xAA, sizeof(g_info));
    size_t n = 0xAA;
    uint8_t rtn = 0xAA;
    assert_int_equal(ydt1363_decode(frame, sizeof(frame), g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_CHECKSUM);
    assert_int_equal(n, 0);
    assert_int_equal(rtn, 0);
    assert_int_equal(g_info[0], 0xAA);
}

static void test_decode_arg_errors(void **state) {
    (void)state;
    size_t n = 0;
    uint8_t rtn = 0;

    assert_int_equal(ydt1363_decode(NULL, 18, g_info, sizeof(g_info), &n, &rtn), YDT1363_ERR_ARG);
    assert_int_equal(ydt1363_decode(kFrameEmpty, 18, g_info, sizeof(g_info), NULL, &rtn),
                     YDT1363_ERR_ARG);
    assert_int_equal(ydt1363_decode(kFrameEmpty, 18, g_info, sizeof(g_info), &n, NULL),
                     YDT1363_ERR_ARG);
}

static void test_frame_len(void **state) {
    (void)state;
    uint8_t header[13];
    size_t i;

    assert_int_equal(ydt1363_frame_len(kFrameEmpty, 0), 0);
    assert_int_equal(ydt1363_frame_len(kFrameEmpty, 12), 0);
    assert_int_equal(ydt1363_frame_len(kFrameEmpty, sizeof(kFrameEmpty) - 1), 18);
    assert_int_equal(ydt1363_frame_len(kFrameInfo, sizeof(kFrameInfo) - 1), 22);
    /* 头部够 13 字节就能给出期望总长，是否收满由调用方判断 */
    assert_int_equal(ydt1363_frame_len(kFrameInfo, 13), 22);

    memcpy(header, kFrameEmpty, sizeof(header));
    header[9] = '1';
    assert_true(ydt1363_frame_len(header, sizeof(header)) < 0);

    for (i = 0; i < sizeof(header); i++) {
        header[i] = 0x00;
    }
    assert_true(ydt1363_frame_len(header, sizeof(header)) < 0); /* 非 SOI */
    assert_int_equal(ydt1363_frame_len(NULL, 13), YDT1363_ERR_ARG);
}

/* 前导垃圾：逐字节扫到 SOI 后按帧长解 */
static void test_resync_leading_garbage(void **state) {
    (void)state;
    static uint8_t stream[32];
    static const uint8_t junk[] = {0x00, 0xAA, 0x55};

    memcpy(stream, junk, sizeof(junk));
    memcpy(stream + sizeof(junk), kFrameInfo, sizeof(kFrameInfo) - 1);
    size_t total = sizeof(junk) + sizeof(kFrameInfo) - 1;

    size_t off = 0;
    while (off < total && stream[off] != YDT1363_SOI) {
        off++;
    }
    assert_int_equal(off, sizeof(junk));
    assert_int_equal(ydt1363_frame_len(stream + off, total - off), 22);

    size_t n = 0;
    uint8_t rtn = 0xAA;
    assert_int_equal(ydt1363_decode(stream + off, total - off, g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_OK);
    assert_int_equal(n, 2);
    assert_int_equal(g_info[0], 0x01);
}

/* 坏帧只丢 1 字节再扫 SOI：下一帧仍能被完整找回 */
static void test_resync_after_bad_frame(void **state) {
    (void)state;
    static uint8_t stream[64];
    uint8_t bad[18];

    memcpy(bad, kFrameEmpty, sizeof(bad));
    bad[13] = '0';
    memcpy(stream, bad, sizeof(bad));
    memcpy(stream + sizeof(bad), kFrameInfo, sizeof(kFrameInfo) - 1);
    size_t total = sizeof(bad) + sizeof(kFrameInfo) - 1;

    size_t n = 0;
    uint8_t rtn = 0;
    assert_int_equal(ydt1363_decode(stream, sizeof(bad), g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_ERR_CHECKSUM);

    size_t off = 1;
    while (off < total && stream[off] != YDT1363_SOI) {
        off++;
    }
    assert_int_equal(off, sizeof(bad));
    assert_int_equal(ydt1363_frame_len(stream + off, total - off), 22);
    assert_int_equal(ydt1363_decode(stream + off, total - off, g_info, sizeof(g_info), &n, &rtn),
                     YDT1363_OK);
    assert_int_equal(n, 2);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_encode_empty_info_bytes),
        cmocka_unit_test(test_encode_info_bytes),
        cmocka_unit_test(test_encode_rejects_oversize),
        cmocka_unit_test(test_encode_rejects_no_tailroom),
        cmocka_unit_test(test_encode_arg_errors),
        cmocka_unit_test(test_max_info_roundtrip),
        cmocka_unit_test(test_decode_ok),
        cmocka_unit_test(test_decode_empty_info),
        cmocka_unit_test(test_decode_rtn_error),
        cmocka_unit_test(test_decode_bad_lchksum),
        cmocka_unit_test(test_decode_bad_chksum),
        cmocka_unit_test(test_decode_missing_eoi),
        cmocka_unit_test(test_decode_odd_lenid),
        cmocka_unit_test(test_decode_non_hex),
        cmocka_unit_test(test_decode_len_mismatch),
        cmocka_unit_test(test_decode_info_over_cap),
        cmocka_unit_test(test_decode_error_outputs_defined),
        cmocka_unit_test(test_decode_arg_errors),
        cmocka_unit_test(test_frame_len),
        cmocka_unit_test(test_resync_leading_garbage),
        cmocka_unit_test(test_resync_after_bad_frame),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
