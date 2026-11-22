/**
 * @file test_driver_dlt698.c
 * @brief DLT698 驱动的主站路径：GET 与 SET（ADR-002 §S5）
 *
 * 只测"结构体 ↔ 字节"：不起总线、不建会话、不查点位表。
 *
 * 帧格式（codec 的约定）：
 *   68 | L(2, LE) | 43 | SA(len+1) | CA | HCS(2) | APDU | FCS(2) | 16
 * 所以 APDU 起点 = 4 + (1 + sa_len) + 1 + 2 = 8 + sa_len。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dlt698_codec.h"
#include "dcu_698_appconn.h"
#include "dcu_698_axdr.h"
#include "dcu_698_param.h"
#include "driver_dlt698.h"
#include "edge_driver.h"
#include "net_buf.h"

extern const edge_driver_t dlt698_driver;

#define DLT698_SA_LEN 6
#define APDU_OFF (8 + DLT698_SA_LEN)

static int prepare_into(uint8_t* priv, const void* desc, size_t desc_len, uint32_t oad,
                        bool is_write, double value) {
    memset(priv, 0, REQ_DRV_PRIV_MAX);
    return dlt698_driver.req_prepare(priv, 1, oad, NULL, 0, desc, desc_len, is_write, value);
}

/* 只在 prepare 成功时使用；失败时返回 NULL 且调用方应先断言过 rc */
static net_buf_t* encode_once(net_buf_pool_t* pool, const void* desc, size_t desc_len,
                              uint32_t oad, bool is_write, double value) {
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    if (prepare_into(priv, desc, desc_len, oad, is_write, value) != 0) return NULL;
    net_buf_t* out = net_buf_alloc(pool, 0);
    if (!out) return NULL;
    if (dlt698_driver.req_encode(priv, NULL, out) != 0) {
        net_buf_unref(out);
        return NULL;
    }
    return out;
}

/* FCS 覆盖 [1, len-3)，末 3 字节是 FCS(2) + 0x16 */
static void assert_frame_sealed(const net_buf_t* f) {
    assert_true(f->b.len > 6);
    assert_int_equal(f->b.data[0], DLT698_HEAD);
    assert_int_equal(f->b.data[f->b.len - 1], DLT698_END);

    /* L 字段 = 帧长 - 2（标准 5.1.2："除起始字符和结束字符之外的帧字节数"）。
     * 这里曾经写成 -1，把 codec 的 off-by-one 当成约定锁进了测试 —— 用标准
     * 附录 H.1 的登录帧就能验：32 字节的帧，L 是 30，不是 31。 */
    uint16_t l = (uint16_t)((f->b.data[1] | (f->b.data[2] << 8)) & 0x3FFF);
    assert_int_equal(l, (uint16_t)(f->b.len - 2));

    uint16_t want = dlt698_calculate_fcs(f->b.data + 1, (size_t)(f->b.len - 4));
    uint16_t got = (uint16_t)(f->b.data[f->b.len - 3] | (f->b.data[f->b.len - 2] << 8));
    assert_int_equal(got, want);
}

/* GET：05 | 01 | PIID | OAD(4) | 00（无选择访问） */
static void test_get_request_layout(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    net_buf_t* f = encode_once(&pool, NULL, 0, 0x20000200u, false, 0.0);
    assert_non_null(f);

    const uint8_t* apdu = f->b.data + APDU_OFF;
    assert_int_equal(apdu[0], DLT698_SVC_GET_REQ);
    assert_int_equal(apdu[1], 0x01);
    assert_int_equal((apdu[3] << 24) | (apdu[4] << 16) | (apdu[5] << 8) | apdu[6], 0x20000200);
    assert_int_equal(apdu[7], 0x00);
    assert_frame_sealed(f);

    net_buf_unref(f);
    net_buf_pool_destroy(&pool);
}

/* SET long-unsigned：06 | 01 | PIID | OAD(4) | 12 | hi | lo */
static void test_set_long_unsigned(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    dlt698_write_desc_t desc = {.type = 0x12};
    net_buf_t* f = encode_once(&pool, &desc, sizeof(desc), 0x20000200u, true, 2200.0);
    assert_non_null(f);

    const uint8_t* apdu = f->b.data + APDU_OFF;
    assert_int_equal(apdu[0], DLT698_SVC_SET_REQ); /* 0x06 */
    assert_int_equal(apdu[1], 0x01);
    assert_int_equal(apdu[7], 0x12); /* long-unsigned 标签 */
    assert_int_equal(apdu[8], (2200 >> 8) & 0xFF);
    assert_int_equal(apdu[9], 2200 & 0xFF);
    /* SET 的 APDU 末尾是时间标签（无 = 00），见标准附录 H.4.1。
     * 数据段 10 字节 + 时间标签 1 字节，再加帧尾 FCS(2)+16(1)。 */
    assert_int_equal(apdu[10], DLT698_NO_TIME_TAG);
    assert_int_equal(f->b.len, (size_t)APDU_OFF + 11 + 3);
    assert_frame_sealed(f);

    net_buf_unref(f);
    net_buf_pool_destroy(&pool);
}

/* SET double-long：06 | 01 | PIID | OAD(4) | 05 | 4 字节大端 */
static void test_set_double_long(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    dlt698_write_desc_t desc = {.type = 0x05};
    net_buf_t* f = encode_once(&pool, &desc, sizeof(desc), 0x20000200u, true, 123456.0);
    assert_non_null(f);

    const uint8_t* apdu = f->b.data + APDU_OFF;
    assert_int_equal(apdu[7], 0x05);
    assert_int_equal(apdu[8], 0x00);
    assert_int_equal(apdu[9], 0x01);
    assert_int_equal(apdu[10], 0xE2);
    assert_int_equal(apdu[11], 0x40);
    assert_frame_sealed(f);

    net_buf_unref(f);
    net_buf_pool_destroy(&pool);
}

/* 写请求必须带类型描述，且类型/值域合法——宁可失败也不发"看起来对"的报文 */
static void test_set_rejects_bad_input(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];

    /* 1. 缺描述 */
    assert_int_not_equal(prepare_into(priv, NULL, 0, 0x20000200u, true, 1.0), 0);

    /* 2. 类型不支持（0x11 unsigned 没有编码器） */
    dlt698_write_desc_t bad_type = {.type = 0x11};
    assert_int_not_equal(prepare_into(priv, &bad_type, sizeof(bad_type), 0x20000200u, true, 1.0), 0);

    /* 3. long-unsigned 越界（>65535 / <0）：截断写入比失败更危险 */
    dlt698_write_desc_t lu = {.type = 0x12};
    assert_int_not_equal(prepare_into(priv, &lu, sizeof(lu), 0x20000200u, true, 70000.0), 0);
    assert_int_not_equal(prepare_into(priv, &lu, sizeof(lu), 0x20000200u, true, -1.0), 0);

    /* 3b. 合法值必须通过，证明拒绝不是"一律拒绝" */
    assert_int_equal(prepare_into(priv, &lu, sizeof(lu), 0x20000200u, true, 65535.0), 0);

    /* 4. 读请求不受影响 */
    net_buf_t* f = encode_once(&pool, NULL, 0, 0x20000200u, false, 0.0);
    assert_non_null(f);
    net_buf_unref(f);

    net_buf_pool_destroy(&pool);
}

static size_t build_slave_frame(uint8_t* out, const uint8_t* apdu, size_t apdu_len) {
    size_t n = 0;
    out[n++] = DLT698_HEAD;
    size_t len_off = n;
    n += 2;
    out[n++] = 0x43;
    out[n++] = 0x00; /* SA: type 0, len 1 */
    out[n++] = 0x01;
    out[n++] = 0x10; /* CA */

    uint16_t l = (uint16_t)(9 + apdu_len + 3 - 2);
    out[len_off] = (uint8_t)(l & 0xFF);
    out[len_off + 1] = (uint8_t)(l >> 8);
    uint16_t hcs = dlt698_calculate_fcs(out + 1, n - 1);
    out[n++] = (uint8_t)(hcs & 0xFF);
    out[n++] = (uint8_t)(hcs >> 8);

    memcpy(out + n, apdu, apdu_len);
    n += apdu_len;

    uint16_t fcs = dlt698_calculate_fcs(out + 1, n - 1);
    out[n++] = (uint8_t)(fcs & 0xFF);
    out[n++] = (uint8_t)(fcs >> 8);
    out[n++] = DLT698_END;
    return n;
}

typedef struct {
    size_t total;
    int begin_rc;
    bool row_fails;
    uint8_t acl[1024];
    size_t acl_len;
    size_t fail_index;
    int row_rc;
    int axdr_rc;
    int begin_calls;
    int end_calls;
    size_t seen_sel_len;
} fake_record_t;

static size_t build_acl(uint8_t* buf, const uint16_t* ois, size_t n) {
    size_t o = 0;
    buf[o++] = 0x01;
    buf[o++] = (uint8_t)n;
    for (size_t i = 0; i < n; i++) {
        buf[o++] = 0x02;
        buf[o++] = 0x02;
        buf[o++] = 0x50;
        buf[o++] = (uint8_t)(ois[i] >> 8);
        buf[o++] = (uint8_t)ois[i];
        buf[o++] = 0x02;
        buf[o++] = 0x02;
        buf[o++] = 0x01;
        buf[o++] = 1;
        buf[o++] = 0x02;
        buf[o++] = 0x02;
        buf[o++] = 0x11;
        buf[o++] = 2;
        buf[o++] = 0x16;
        buf[o++] = 1; /* 只读 */
        buf[o++] = 0x01;
        buf[o++] = 0;
    }
    return o;
}

static int fake_object_list(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    fake_record_t* f = (fake_record_t*)ctx;
    if (oad != 0x44000200u) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (f->acl_len > cap) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    memcpy(buf, f->acl, f->acl_len);
    *len = f->acl_len;
    return 0;
}

static int fake_begin(void* ctx, uint32_t oad, const uint8_t* selection, size_t selection_len,
                      size_t* row_count) {
    fake_record_t* f = (fake_record_t*)ctx;
    (void)oad;
    (void)selection;
    f->begin_calls++;
    f->seen_sel_len = selection_len;
    if (f->begin_rc != 0) return f->begin_rc;
    *row_count = f->total;
    return 0;
}

static int fake_row(void* ctx, uint32_t oad, size_t index, uint8_t* row, size_t cap,
                    size_t* row_len) {
    fake_record_t* f = (fake_record_t*)ctx;
    (void)oad;
    if (f->row_fails && index == f->fail_index) return f->row_rc;
    static const uint8_t row_a[] = {0x1C, 0x07, 0xE0, 0x01, 0x14, 0x00, 0x00, 0x00};
    static const uint8_t row_b[] = {0x1C, 0x07, 0xE0, 0x01, 0x15, 0x00, 0x00, 0x00};
    const uint8_t* src = (index == 0) ? row_a : row_b;
    if (cap < sizeof(row_a)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    memcpy(row, src, sizeof(row_a));
    *row_len = sizeof(row_a);
    return 0;
}

static int fake_end(void* ctx, uint32_t oad) {
    fake_record_t* f = (fake_record_t*)ctx;
    (void)oad;
    f->end_calls++;
    return 0;
}

static int fake_resolve_axdr(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    (void)oad;
    (void)buf;
    (void)cap;
    (void)len;
    return ((fake_record_t*)ctx)->axdr_rc;
}

static void assert_slave_response(const net_buf_t* out, const uint8_t* want, size_t want_len) {
    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, out->b.data, out->b.len);
    nb.len = (uint16_t)out->b.len;
    dlt698_addr_t sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t* payload = NULL;
    size_t payload_len = 0;
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload,
                                            &payload_len),
                     0);
    size_t apdu_len = payload_len + 3;
    assert_int_equal(apdu_len, want_len);
    assert_memory_equal(payload - 3, want, want_len);
}

static int run_record_case(const uint8_t* apdu, size_t apdu_len, fake_record_t* f, bool with_port,
                           net_buf_t* out) {
    uint8_t req_apdu[256];
    req_apdu[0] = DLT698_SVC_GET_REQ;
    req_apdu[1] = DLT698_CHOICE_GET_RECORD;
    req_apdu[2] = 0x05; /* PIID */
    memcpy(req_apdu + 3, apdu, apdu_len);
    uint8_t frame[320];
    size_t flen = build_slave_frame(frame, req_apdu, apdu_len + 3);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = fake_object_list;
    if (with_port) {
        src.record.begin = fake_begin;
        src.record.row = fake_row;
        src.record.end = fake_end;
    }
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    return dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, f, out, &consumed);
}

static void test_slave_get_record_page(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x01, 0x00,
                                  0x20, 0x21, 0x02, 0x00, 0x00};
    fake_record_t f = {.total = 2};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, true, out), 0);
    assert_int_equal(f.begin_calls, 1);
    assert_int_equal(f.end_calls, 1);
    assert_int_equal(f.seen_sel_len, 7); /* RSD(1) + RCSD 计数(1) + CSD(5)，不含时间标签 */

    static const uint8_t want[] = {
        0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00, 0x20, 0x21, 0x02, 0x00,
        0x01, 0x02, 0x1C, 0x07, 0xE0, 0x01, 0x14, 0x00, 0x00, 0x00, 0x1C, 0x07, 0xE0,
        0x01, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    assert_slave_response(out, want, sizeof(want));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_slave_get_record_unsupported_selector(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x03};
    fake_record_t f = {.total = 0};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, true, out), 0);
    assert_int_equal(f.begin_calls, 0);

    static const uint8_t want[] = {0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x00,
                                   0x00, 0x02, 0x00, 0x00};
    assert_slave_response(out, want, sizeof(want));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_slave_get_record_begin_failure(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x01, 0x00,
                                  0x20, 0x21, 0x02, 0x00, 0x00};
    fake_record_t f = {.total = 2, .begin_rc = EDGE_SLAVE_SRC_NOT_FOUND};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, true, out), 0);
    assert_int_equal(f.begin_calls, 1);
    assert_int_equal(f.end_calls, 0);

    static const uint8_t want[] = {
        0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00, 0x20, 0x21, 0x02, 0x00,
        0x00, 0x06, 0x00, 0x00,
    };
    assert_slave_response(out, want, sizeof(want));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_slave_get_record_row_failure_aborts_page(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x01, 0x00,
                                  0x20, 0x21, 0x02, 0x00, 0x00};
    fake_record_t f = {.total = 2, .row_fails = true, .fail_index = 1, .row_rc = EDGE_SLAVE_SRC_TEMPORARY};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, true, out), 0);
    assert_int_equal(f.end_calls, 1);

    static const uint8_t want[] = {
        0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00, 0x20, 0x21, 0x02, 0x00,
        0x00, 0x02, 0x00, 0x00,
    };
    assert_slave_response(out, want, sizeof(want));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_slave_get_record_port_absent(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x01, 0x00,
                                  0x20, 0x21, 0x02, 0x00, 0x00};
    fake_record_t f = {.total = 1};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, false, out), 0);

    static const uint8_t want[] = {
        0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00, 0x20, 0x21, 0x02, 0x00,
        0x00, 0x02, 0x00, 0x00,
    };
    assert_slave_response(out, want, sizeof(want));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 截断的 GetRecord 不应答：坏报文不能伪装成 DAR 2 的应答。 */
static void test_slave_get_record_malformed_is_dropped(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x02, 0x20};
    fake_record_t f = {.total = 1};
    assert_int_equal(run_record_case(req, sizeof(req), &f, true, out), -1);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

typedef struct {
    int calls;
    uint8_t body[32];
    size_t body_len;
} fake_report_t;

static int fake_report_next(void* ctx, uint8_t* body, size_t cap, size_t* len) {
    fake_report_t* r = (fake_report_t*)ctx;
    r->calls++;
    if (r->body_len > cap) return -1;
    memcpy(body, r->body, r->body_len);
    *len = r->body_len;
    return 0;
}

/* CONNECT-Request（表59）：公共连接，版本 0x0010、window>0、apdu_max>0。 */
static size_t build_connect_apdu(uint8_t* apdu, uint8_t piid) {
    size_t o = 0;
    apdu[o++] = 0x02;
    apdu[o++] = piid;
    apdu[o++] = 0x00;
    apdu[o++] = 0x10; /* version */
    for (int i = 0; i < 8; i++) apdu[o++] = 0;
    for (int i = 0; i < 16; i++) apdu[o++] = 0;
    apdu[o++] = 0x00;
    apdu[o++] = 0x00; /* send max */
    apdu[o++] = 0x00;
    apdu[o++] = 0x00; /* recv max */
    apdu[o++] = 0x01; /* window */
    apdu[o++] = 0x00;
    apdu[o++] = 0x05; /* apdu max */
    for (int i = 0; i < 4; i++) apdu[o++] = 0;
    apdu[o++] = 0x00; /* mechanism */
    return o;
}

/* 走一次 CONNECT，让会话 connected 并记下 SA（build_slave_frame 固定 SA=1 字节 0x01）。 */
static void connect_session(uint8_t* session, fake_record_t* f) {
    uint16_t ois[1] = {0x3000};
    f->acl_len = build_acl(f->acl, ois, 1);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = fake_object_list;

    uint8_t apdu[64];
    size_t alen = build_connect_apdu(apdu, 0x01);
    uint8_t frame[96];
    size_t flen = build_slave_frame(frame, apdu, alen);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    assert_int_equal(dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, f, out,
                                                      &consumed),
                     0);
    assert_true(out->b.len > 0);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static int tick_report(uint8_t* session, fake_report_t* r, net_buf_pool_t* pool, net_buf_t** tx) {
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.report.next = fake_report_next;
    src.report.ctx = r;
    return dlt698_driver.req_tick(session, NULL, NULL, &src, NULL, pool, tx);
}

/* 主站回一条 REPORT-Response（PIID 与上报一致），确认不应产生应答帧。 */
static void feed_report_response(uint8_t* session, uint8_t piid) {
    uint8_t apdu[4] = {DLT698_SVC_REPORT_RES, DLT698_REPORT_LIST, piid, 0x00};
    uint8_t frame[32];
    size_t flen = build_slave_frame(frame, apdu, sizeof(apdu));
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 128, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    size_t consumed = 0;
    assert_int_equal(dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, NULL, out,
                                                      &consumed),
                     0);
    assert_int_equal(out->b.len, 0);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void fill_report_body(fake_report_t* r) {
    static const uint8_t body[] = {
        0x01, 0x07, 0x01,       /* CHOICE[1] | PIID 7 | 1 项 */
        0x30, 0x00, 0x04, 0x00, /* OAD 30000400 */
        0x01, 0x12, 0x00, 0x01, /* Get-Result[1] Data：long-unsigned 1 */
    };
    memcpy(r->body, body, sizeof(body));
    r->body_len = sizeof(body);
}

static void test_report_frame_layout(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    fake_record_t f;
    memset(&f, 0, sizeof(f));
    connect_session(session, &f);

    fake_report_t r;
    memset(&r, 0, sizeof(r));
    fill_report_body(&r);

    net_buf_t* tx = NULL;
    assert_int_equal(tick_report(session, &r, &pool, &tx), 0);
    assert_int_equal(r.calls, 1);
    assert_non_null(tx);
    assert_frame_sealed(tx);

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, tx->b.data, tx->b.len);
    nb.len = tx->b.len;
    dlt698_addr_t sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t* payload = NULL;
    size_t payload_len = 0;
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload,
                                            &payload_len),
                     0);
    static const uint8_t want[] = {0x08, 0x01, 0x07, 0x01, 0x30, 0x00, 0x04,
                                   0x00, 0x01, 0x12, 0x00, 0x01, 0x00};
    assert_int_equal(svc, DLT698_SVC_REPORT_NOTIFICATION);
    assert_int_equal(payload_len + 3, sizeof(want));
    assert_memory_equal(payload - 3, want, sizeof(want));

    net_buf_unref(tx);
    net_buf_pool_destroy(&pool);
}

static void test_report_no_session_no_send(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    static uint8_t session[1024];
    memset(session, 0, sizeof(session));

    fake_report_t r;
    memset(&r, 0, sizeof(r));
    fill_report_body(&r);

    net_buf_t* tx = NULL;
    assert_int_equal(tick_report(session, &r, &pool, &tx), 0);
    assert_null(tx);
    assert_int_equal(r.calls, 0);

    net_buf_pool_destroy(&pool);
}

static void test_report_confirm_clears_pending(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    fake_record_t f;
    memset(&f, 0, sizeof(f));
    connect_session(session, &f);

    fake_report_t r;
    memset(&r, 0, sizeof(r));
    fill_report_body(&r);

    net_buf_t* tx1 = NULL;
    assert_int_equal(tick_report(session, &r, &pool, &tx1), 0);
    assert_non_null(tx1);

    net_buf_t* tx2 = NULL;
    assert_int_equal(tick_report(session, &r, &pool, &tx2), 0);
    assert_null(tx2);
    assert_int_equal(r.calls, 1);

    feed_report_response(session, 0x07);
    net_buf_t* tx3 = NULL;
    assert_int_equal(tick_report(session, &r, &pool, &tx3), 0);
    assert_non_null(tx3);
    assert_int_equal(r.calls, 2);

    net_buf_unref(tx1);
    net_buf_unref(tx3);
    net_buf_pool_destroy(&pool);
}

static void test_report_timeout_unblocks(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    fake_record_t f;
    memset(&f, 0, sizeof(f));
    connect_session(session, &f);

    fake_report_t r;
    memset(&r, 0, sizeof(r));
    fill_report_body(&r);

    net_buf_t* tx1 = NULL;
    assert_int_equal(tick_report(session, &r, &pool, &tx1), 0);
    assert_non_null(tx1);

    net_buf_t* tx2 = NULL;
    assert_int_equal(tick_report(session, &r, &pool, &tx2), 0);
    assert_null(tx2);

    /* 驱动超时取自 uv_hrtime，没有注入点，只能真等过 5s（同 IEC104 t1 慢测）。 */
    uv_sleep(5100);

    net_buf_t* tx3 = NULL;
    assert_int_equal(tick_report(session, &r, &pool, &tx3), 0);
    assert_non_null(tx3);
    assert_int_equal(r.calls, 2);

    net_buf_unref(tx1);
    net_buf_unref(tx3);
    net_buf_pool_destroy(&pool);
}

/* GET 5004 A2：端口回 CLASS_MISMATCH / UNDEFINED 时驱动必须映射成 DAR 5 / 4。 */
static void test_get_maps_source_rc_to_dar(void** state) {
    (void)state;
    struct {
        int rc;
        uint8_t dar;
    } cases[] = {
        {EDGE_SLAVE_SRC_CLASS_MISMATCH, DLT698_DAR_OBJECT_CLASS_MISMATCH},
        {EDGE_SLAVE_SRC_UNDEFINED, DLT698_DAR_OBJECT_UNDEFINED},
    };
    static uint8_t session[1024];

    for (size_t c = 0; c < sizeof(cases) / sizeof(cases[0]); c++) {
        net_buf_pool_t pool;
        assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
        memset(session, 0, sizeof(session));

        fake_record_t f;
        memset(&f, 0, sizeof(f));
        uint16_t ois[1] = {0x5004};
        f.acl_len = build_acl(f.acl, ois, 1);
        f.axdr_rc = cases[c].rc;

        uint8_t apdu[8] = {DLT698_SVC_GET_REQ, 0x01, 0x05, 0x50, 0x04, 0x02, 0x00, 0x00};
        uint8_t frame[64];
        size_t flen = build_slave_frame(frame, apdu, sizeof(apdu));

        edge_slave_source_ops_t src;
        memset(&src, 0, sizeof(src));
        src.object_list = fake_object_list;
        src.resolve_axdr = fake_resolve_axdr;

        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
        memset(priv, 0, sizeof(priv));
        size_t consumed = 0;
        assert_int_equal(dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, &f, out,
                                                          &consumed),
                         0);

        struct net_buf_simple nb;
        net_buf_simple_init_with_data(&nb, out->b.data, out->b.len);
        nb.len = (uint16_t)out->b.len;
        dlt698_addr_t sa;
        uint8_t svc = 0, choice = 0, piid = 0;
        uint8_t* payload = NULL;
        size_t payload_len = 0;
        assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload,
                                                &payload_len),
                         0);
        assert_int_equal(svc, DLT698_SVC_GET_RES);
        assert_int_equal(payload[4], 0x00);         /* CHOICE [0] DAR */
        assert_int_equal(payload[5], cases[c].dar);

        net_buf_unref(out);
        net_buf_pool_destroy(&pool);
    }
}

static int run_slave_apdu(const uint8_t* apdu, size_t apdu_len,
                          const edge_slave_source_ops_t* src, void* src_ctx, net_buf_t* out) {
    uint8_t frame[512];
    size_t flen = build_slave_frame(frame, apdu, apdu_len);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    size_t consumed = 0;
    return dlt698_driver.req_slave_exchange(priv, session, frame, flen, src, src_ctx, out,
                                            &consumed);
}

static int product_object_list(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    (void)ctx;
    if (oad != 0x44000200u) return EDGE_SLAVE_SRC_NOT_FOUND;
    return dcu_698_appconn_object_list(buf, cap, len) == 0 ? 0
                                                           : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static void decode_payload(const net_buf_t* out, uint8_t** payload, uint8_t* svc) {
    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, out->b.data, out->b.len);
    nb.len = (uint16_t)out->b.len;
    dlt698_addr_t sa;
    uint8_t choice = 0, piid = 0;
    size_t payload_len = 0;
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, svc, &choice, &piid, payload,
                                            &payload_len),
                     0);
}

/* 28 个对象的真实产品列表必须能被紧凑 ACL 解析；4000 A2 走参数模块。 */
static void test_slave_accepts_product_object_list_28(void** state) {
    (void)state;
    uint8_t list[2048];
    size_t list_len = 0;
    assert_int_equal(dcu_698_appconn_object_list(list, sizeof(list), &list_len), 0);
    assert_int_equal(list[1], 28);

    dcu_698_param_reset();
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t get_4000[] = {0x05, 0x01, 0x05, 0x40, 0x00, 0x02, 0x00, 0x00};
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    assert_int_equal(run_slave_apdu(get_4000, sizeof(get_4000), &src, NULL, out), 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_memory_equal(payload, get_4000 + 3, 4);
    assert_int_equal(payload[4], 0x01); /* A-Result Data */
    assert_int_equal(payload[5], DLT698_TAG_DATE_TIME_S);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 列表内但权限=1（只读）的对象，SET 必须回 DAR 3。 */
static void test_slave_set_read_only_is_denied(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    fake_record_t f;
    memset(&f, 0, sizeof(f));
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);

    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = fake_object_list;
    static const uint8_t set_5004[] = {0x06, 0x01, 0x05, 0x50, 0x04, 0x02,
                                       0x00, 0x09, 0x00, 0x00};
    assert_int_equal(run_slave_apdu(set_5004, sizeof(set_5004), &src, &f, out), 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 超容量（29 对象）的列表整体拒绝，任何 OI 都按未授权 DAR 3 处理。 */
static void test_slave_rejects_29_object_list(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    uint16_t ois[29];
    for (size_t i = 0; i < 29; i++) ois[i] = (uint16_t)(0x2000 + i);
    fake_record_t f;
    memset(&f, 0, sizeof(f));
    f.acl_len = build_acl(f.acl, ois, 29);

    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = fake_object_list;
    src.resolve_axdr = fake_resolve_axdr;
    static const uint8_t get_2000[] = {0x05, 0x01, 0x05, 0x20, 0x00, 0x02, 0x00, 0x00};
    assert_int_equal(run_slave_apdu(get_2000, sizeof(get_2000), &src, &f, out), 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(payload[4], 0x00); /* [0] DAR */
    assert_int_equal(payload[5], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_get_request_layout),
        cmocka_unit_test(test_set_long_unsigned),
        cmocka_unit_test(test_set_double_long),
        cmocka_unit_test(test_set_rejects_bad_input),
        cmocka_unit_test(test_slave_get_record_page),
        cmocka_unit_test(test_slave_get_record_unsupported_selector),
        cmocka_unit_test(test_slave_get_record_begin_failure),
        cmocka_unit_test(test_slave_get_record_row_failure_aborts_page),
        cmocka_unit_test(test_slave_get_record_port_absent),
        cmocka_unit_test(test_slave_get_record_malformed_is_dropped),
        cmocka_unit_test(test_report_frame_layout),
        cmocka_unit_test(test_report_no_session_no_send),
        cmocka_unit_test(test_report_confirm_clears_pending),
        cmocka_unit_test(test_report_timeout_unblocks),
        cmocka_unit_test(test_get_maps_source_rc_to_dar),
        cmocka_unit_test(test_slave_accepts_product_object_list_28),
        cmocka_unit_test(test_slave_set_read_only_is_denied),
        cmocka_unit_test(test_slave_rejects_29_object_list),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
