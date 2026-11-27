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
#include <time.h>

#include "bus_orchestrator.h"
#include "dcu_comm_info.h"
#include "dcu_698_appconn.h"
#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_param.h"
#include "dcu_inventory.h"
#include "dcu_north_adapter.h"
#include "dcu_poller.h"
#include "dlt698_codec.h"
#include "driver_dlt698.h"
#include "edge_driver.h"
#include "net_buf.h"

extern const edge_driver_t dlt698_driver;

static int slave_clock_now(void* ctx, struct tm* out) {
    (void)ctx;
    *out = (struct tm){
        .tm_year = 124, .tm_mon = 4, .tm_mday = 6, .tm_hour = 7, .tm_min = 8, .tm_sec = 9};
    return 0;
}

static int slave_clock_set(void* ctx, const struct tm* in) {
    (void)ctx;
    (void)in;
    return 0;
}

#define DLT698_SA_LEN 6
#define APDU_OFF (8 + DLT698_SA_LEN)

static int prepare_into(uint8_t* priv, const void* desc, size_t desc_len, uint32_t oad,
                        bool is_write, double value) {
    memset(priv, 0, REQ_DRV_PRIV_MAX);
    return dlt698_driver.req_prepare(priv, 1, oad, NULL, 0, desc, desc_len, is_write, value);
}

/* 只在 prepare 成功时使用；失败时返回 NULL 且调用方应先断言过 rc */
static net_buf_t* encode_once(net_buf_pool_t* pool, const void* desc, size_t desc_len, uint32_t oad,
                              bool is_write, double value) {
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
    assert_int_not_equal(prepare_into(priv, &bad_type, sizeof(bad_type), 0x20000200u, true, 1.0),
                         0);

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
    uint8_t acl[4096];
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
    if (n < 0x80) {
        buf[o++] = (uint8_t)n;
    } else {
        buf[o++] = 0x81;
        buf[o++] = (uint8_t)n; /* 224 < 256 */
    }
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
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload, &payload_len),
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
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    return dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, f, out, &consumed);
}

static void test_slave_get_record_page(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x01,
                                  0x00, 0x20, 0x21, 0x02, 0x00, 0x00};
    fake_record_t f = {.total = 2};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, true, out), 0);
    assert_int_equal(f.begin_calls, 1);
    assert_int_equal(f.end_calls, 1);
    assert_int_equal(f.seen_sel_len, 7); /* RSD(1) + RCSD 计数(1) + CSD(5)，不含时间标签 */

    static const uint8_t want[] = {
        0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00, 0x20, 0x21,
        0x02, 0x00, 0x01, 0x02, 0x1C, 0x07, 0xE0, 0x01, 0x14, 0x00, 0x00,
        0x00, 0x1C, 0x07, 0xE0, 0x01, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
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

    static const uint8_t want[] = {0x85, 0x03, 0x05, 0x50, 0x04, 0x02,
                                   0x00, 0x00, 0x00, 0x02, 0x00, 0x00};
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

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x01,
                                  0x00, 0x20, 0x21, 0x02, 0x00, 0x00};
    fake_record_t f = {.total = 2, .begin_rc = EDGE_SLAVE_SRC_NOT_FOUND};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, true, out), 0);
    assert_int_equal(f.begin_calls, 1);
    assert_int_equal(f.end_calls, 0);

    static const uint8_t want[] = {
        0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00,
        0x20, 0x21, 0x02, 0x00, 0x00, 0x06, 0x00, 0x00,
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

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x01,
                                  0x00, 0x20, 0x21, 0x02, 0x00, 0x00};
    fake_record_t f = {
        .total = 2, .row_fails = true, .fail_index = 1, .row_rc = EDGE_SLAVE_SRC_TEMPORARY};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, true, out), 0);
    assert_int_equal(f.end_calls, 1);

    static const uint8_t want[] = {
        0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00,
        0x20, 0x21, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00,
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

    static const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x01,
                                  0x00, 0x20, 0x21, 0x02, 0x00, 0x00};
    fake_record_t f = {.total = 1};
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);
    assert_int_equal(run_record_case(req, sizeof(req), &f, false, out), 0);

    static const uint8_t want[] = {
        0x85, 0x03, 0x05, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00,
        0x20, 0x21, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00,
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
    assert_int_equal(
        dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, f, out, &consumed), 0);
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
    assert_int_equal(
        dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, NULL, out, &consumed),
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
    static uint8_t session[4096];
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
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload, &payload_len),
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
    static uint8_t session[4096];
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
    static uint8_t session[4096];
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
    static uint8_t session[4096];
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

/* N=224 的对象列表必须用 A-XDR 长格式长度解析；最后一个对象 0x0002
 * 带 attr 2 只读，GET 应进入 resolve（返回 DAR 6）而不是被 ACL 拒绝。 */
static void test_slave_parses_long_form_acl_224(void** state) {
    (void)state;
    static uint8_t session[4096];
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    memset(session, 0, sizeof(session));

    fake_record_t f;
    memset(&f, 0, sizeof(f));
    static uint16_t ois[224];
    for (size_t i = 0; i < 223; i++) ois[i] = 0x0001;
    ois[223] = 0x0002;
    f.acl_len = build_acl(f.acl, ois, 224);
    f.axdr_rc = EDGE_SLAVE_SRC_NOT_FOUND;

    static const uint8_t get_allowed[] = {DLT698_SVC_GET_REQ, 0x01, 0x05, 0x00, 0x02, 0x02, 0x00,
                                          0x00};
    static const uint8_t get_denied[] = {DLT698_SVC_GET_REQ, 0x01, 0x05, 0x00, 0x03, 0x02, 0x00,
                                         0x00};
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = fake_object_list;
    src.resolve_axdr = fake_resolve_axdr;

    const uint8_t* reqs[] = {get_allowed, get_denied};
    const uint8_t want_dar[] = {DLT698_DAR_OBJECT_NOT_FOUND, DLT698_DAR_DENIED};
    for (size_t c = 0; c < 2; c++) {
        uint8_t frame[64];
        size_t flen = build_slave_frame(frame, reqs[c], sizeof(get_allowed));
        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
        memset(priv, 0, sizeof(priv));
        size_t consumed = 0;
        assert_int_equal(
            dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, &f, out, &consumed),
            0);
        struct net_buf_simple nb;
        net_buf_simple_init_with_data(&nb, out->b.data, out->b.len);
        nb.len = (uint16_t)out->b.len;
        dlt698_addr_t sa;
        uint8_t svc = 0, choice = 0, piid = 0;
        uint8_t* payload = NULL;
        size_t payload_len = 0;
        assert_int_equal(
            dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload, &payload_len), 0);
        assert_int_equal(svc, DLT698_SVC_GET_RES);
        assert_int_equal(payload[4], 0x00);
        assert_int_equal(payload[5], want_dar[c]);
        net_buf_unref(out);
    }
    net_buf_pool_destroy(&pool);
}

static void test_get_maps_source_rc_to_dar(void** state) {
    (void)state;
    struct {
        int rc;
        uint8_t dar;
    } cases[] = {
        {EDGE_SLAVE_SRC_CLASS_MISMATCH, DLT698_DAR_OBJECT_CLASS_MISMATCH},
        {EDGE_SLAVE_SRC_UNDEFINED, DLT698_DAR_OBJECT_UNDEFINED},
    };
    static uint8_t session[4096];

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
        assert_int_equal(
            dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, &f, out, &consumed),
            0);

        struct net_buf_simple nb;
        net_buf_simple_init_with_data(&nb, out->b.data, out->b.len);
        nb.len = (uint16_t)out->b.len;
        dlt698_addr_t sa;
        uint8_t svc = 0, choice = 0, piid = 0;
        uint8_t* payload = NULL;
        size_t payload_len = 0;
        assert_int_equal(
            dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload, &payload_len), 0);
        assert_int_equal(svc, DLT698_SVC_GET_RES);
        assert_int_equal(payload[4], 0x00); /* CHOICE [0] DAR */
        assert_int_equal(payload[5], cases[c].dar);

        net_buf_unref(out);
        net_buf_pool_destroy(&pool);
    }
}

static int run_slave_apdu(const uint8_t* apdu, size_t apdu_len, const edge_slave_source_ops_t* src,
                          void* src_ctx, net_buf_t* out) {
    uint8_t frame[512];
    size_t flen = build_slave_frame(frame, apdu, apdu_len);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    size_t consumed = 0;
    return dlt698_driver.req_slave_exchange(priv, session, frame, flen, src, src_ctx, out,
                                            &consumed);
}

static int product_object_list(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    (void)ctx;
    if (oad != 0x44000200u) return EDGE_SLAVE_SRC_NOT_FOUND;
    return dcu_698_appconn_object_list(buf, cap, len) == 0 ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

static void decode_payload(const net_buf_t* out, uint8_t** payload, uint8_t* svc) {
    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, out->b.data, out->b.len);
    nb.len = (uint16_t)out->b.len;
    dlt698_addr_t sa;
    uint8_t choice = 0, piid = 0;
    size_t payload_len = 0;
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, svc, &choice, &piid, payload, &payload_len),
                     0);
}

static void test_slave_accepts_product_object_list_42(void** state) {
    (void)state;
    uint8_t list[EDGE_SLAVE_AXDR_MAX];
    size_t list_len = 0;
    assert_int_equal(dcu_698_appconn_object_list(list, sizeof(list), &list_len), 0);
    assert_int_equal(list[1], 118);

    static const dcu_clock_port_t clock = {.now = slave_clock_now, .set = slave_clock_set};
    dcu_698_param_set_clock(&clock);
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

static void assert_product_get_dar(const uint8_t* apdu, size_t apdu_len, uint8_t want_dar) {
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    assert_int_equal(run_slave_apdu(apdu, apdu_len, &src, NULL, out), 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(payload[4], 0x00); /* A-Result Data 的 DAR 分支 */
    assert_int_equal(payload[5], want_dar);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_slave_class17_display_dar(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    static const uint8_t get_f300_a2[] = {0x05, 0x01, 0x05, 0xF3, 0x00, 0x02, 0x00, 0x00};
    static const uint8_t get_f300_a1[] = {0x05, 0x01, 0x05, 0xF3, 0x00, 0x01, 0x00, 0x00};
    static const uint8_t get_f301_a2[] = {0x05, 0x01, 0x05, 0xF3, 0x01, 0x02, 0x00, 0x00};
    assert_product_get_dar(get_f300_a2, sizeof(get_f300_a2), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_f301_a2, sizeof(get_f301_a2), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_f300_a1, sizeof(get_f300_a1), DLT698_DAR_DENIED);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    static const uint8_t set_f300_a2[] = {0x06, 0x01, 0x05, 0xF3, 0x00, 0x02,
                                          0x00, 0x09, 0x00, 0x00};
    assert_int_equal(run_slave_apdu(set_f300_a2, sizeof(set_f300_a2), &src, NULL, out), 0);
    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
    dcu_698_object_model_reset();
}

static void test_slave_class11_collection_dar(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    static const uint8_t get_7010_a2[] = {0x05, 0x01, 0x05, 0x70, 0x10, 0x02, 0x00, 0x00};
    static const uint8_t get_7010_a1[] = {0x05, 0x01, 0x05, 0x70, 0x10, 0x01, 0x00, 0x00};
    static const uint8_t get_7000_a2[] = {0x05, 0x01, 0x05, 0x70, 0x00, 0x02, 0x00, 0x00};
    assert_product_get_dar(get_7010_a2, sizeof(get_7010_a2), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_7010_a1, sizeof(get_7010_a1), DLT698_DAR_DENIED);
    assert_product_get_dar(get_7000_a2, sizeof(get_7000_a2), DLT698_DAR_DENIED);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_action = dcu_north_accept_action;

    static const uint8_t action_7000_127[] = {0x07, 0x01, 0x05, 0x70, 0x00,
                                              0x7F, 0x00, 0x00, 0x00};
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_7000_127, sizeof(action_7000_127), &src, NULL, out), 0);
    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
    net_buf_unref(out);

    static const uint8_t action_7000_130[] = {0x07, 0x01, 0x05, 0x70, 0x00,
                                              0x82, 0x00, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_7000_130, sizeof(action_7000_130), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    static const uint8_t set_7010_a2[] = {0x06, 0x01, 0x05, 0x70, 0x10,
                                          0x02, 0x00, 0x09, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(set_7010_a2, sizeof(set_7010_a2), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
    dcu_698_object_model_reset();
}

static void test_slave_class12_pulse_dar(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    /* ACL 广告 A1..A15：类定义但无来源 → DAR 5；A16/A20 不在 ACL → 主站先 DAR 3。 */
    static const uint8_t get_2401_a1[] = {0x05, 0x01, 0x05, 0x24, 0x01, 0x01, 0x00, 0x00};
    static const uint8_t get_2401_a15[] = {0x05, 0x01, 0x05, 0x24, 0x01, 0x0F, 0x00, 0x00};
    static const uint8_t get_2401_a16[] = {0x05, 0x01, 0x05, 0x24, 0x01, 0x10, 0x00, 0x00};
    static const uint8_t get_2401_a20[] = {0x05, 0x01, 0x05, 0x24, 0x01, 0x14, 0x00, 0x00};
    static const uint8_t get_2402_a2[] = {0x05, 0x01, 0x05, 0x24, 0x02, 0x02, 0x00, 0x00};
    assert_product_get_dar(get_2401_a1, sizeof(get_2401_a1), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_2401_a15, sizeof(get_2401_a15), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_2401_a16, sizeof(get_2401_a16), DLT698_DAR_DENIED);
    assert_product_get_dar(get_2401_a20, sizeof(get_2401_a20), DLT698_DAR_DENIED);
    assert_product_get_dar(get_2402_a2, sizeof(get_2402_a2), DLT698_DAR_DENIED);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_action = dcu_north_accept_action;

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    for (uint8_t method = 1; method <= 4; method++) {
        uint8_t action_2401[9] = {0x07, 0x01, 0x05, 0x24, 0x01, method, 0x00, 0x00, 0x00};
        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        assert_int_equal(run_slave_apdu(action_2401, sizeof(action_2401), &src, NULL, out), 0);
        decode_payload(out, &payload, &svc);
        assert_int_equal(svc, DLT698_SVC_ACTION_RES);
        assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
        net_buf_unref(out);
    }

    static const uint8_t action_2401_5[] = {0x07, 0x01, 0x05, 0x24, 0x01,
                                            0x05, 0x00, 0x00, 0x00};
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_2401_5, sizeof(action_2401_5), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    static const uint8_t set_2401_a1[] = {0x06, 0x01, 0x05, 0x24, 0x01,
                                          0x01, 0x00, 0x09, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(set_2401_a1, sizeof(set_2401_a1), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
    dcu_698_object_model_reset();
}

static void test_slave_class25_26_comm_dar(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    static const uint8_t k_mac[6] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};
    dcu_comm_info_t info = {.mac_present = true};
    memcpy(info.mac, k_mac, sizeof(k_mac));
    dcu_comm_info_install(&info);

    /* ACL 广告 A1..A11（Class 25）/A1..A5（Class 26）：类定义但无来源 → DAR 5；
     * 未在 ACL 先 DAR 3。4510 A5 是唯一有平台来源的属性。 */
    static const uint8_t get_4500_a1[] = {0x05, 0x01, 0x05, 0x45, 0x00, 0x01, 0x00, 0x00};
    static const uint8_t get_4500_a12[] = {0x05, 0x01, 0x05, 0x45, 0x00, 0x0C, 0x00, 0x00};
    static const uint8_t get_4510_a4[] = {0x05, 0x01, 0x05, 0x45, 0x10, 0x04, 0x00, 0x00};
    static const uint8_t get_4510_a6[] = {0x05, 0x01, 0x05, 0x45, 0x10, 0x06, 0x00, 0x00};
    static const uint8_t get_4510_a5[] = {0x05, 0x01, 0x05, 0x45, 0x10, 0x05, 0x00, 0x00};
    static const uint8_t get_4511_a5[] = {0x05, 0x01, 0x05, 0x45, 0x11, 0x05, 0x00, 0x00};
    static const uint8_t get_4502_a2[] = {0x05, 0x01, 0x05, 0x45, 0x02, 0x02, 0x00, 0x00};
    assert_product_get_dar(get_4500_a1, sizeof(get_4500_a1), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_4500_a12, sizeof(get_4500_a12), DLT698_DAR_DENIED);
    assert_product_get_dar(get_4510_a4, sizeof(get_4510_a4), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_4510_a6, sizeof(get_4510_a6), DLT698_DAR_DENIED);
    assert_product_get_dar(get_4511_a5, sizeof(get_4511_a5), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_4502_a2, sizeof(get_4502_a2), DLT698_DAR_DENIED);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_action = dcu_north_accept_action;

    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(get_4510_a5, sizeof(get_4510_a5), &src, NULL, out), 0);
    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(payload[4], 0x01); /* A-Result Data */
    assert_int_equal(payload[5], DLT698_TAG_OCTET_STRING);
    assert_int_equal(payload[6], 6);
    assert_memory_equal(payload + 7, k_mac, sizeof(k_mac));
    net_buf_unref(out);

    static const uint8_t action_4500_1[] = {0x07, 0x01, 0x05, 0x45, 0x00,
                                            0x01, 0x00, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_4500_1, sizeof(action_4500_1), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
    net_buf_unref(out);

    static const uint8_t action_4500_2[] = {0x07, 0x01, 0x05, 0x45, 0x00,
                                            0x02, 0x00, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_4500_2, sizeof(action_4500_2), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    static const uint8_t set_4510_a5[] = {0x06, 0x01, 0x05, 0x45, 0x10,
                                          0x05, 0x00, 0x09, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(set_4510_a5, sizeof(set_4510_a5), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);

    /* 平台没有以太网接口时 A5 回 DAR 5，不拿全零或编造的 MAC 冒充。 */
    dcu_comm_info_reset();
    assert_product_get_dar(get_4510_a5, sizeof(get_4510_a5), DLT698_DAR_OBJECT_CLASS_MISMATCH);

    dcu_698_object_model_reset();
}

static void test_slave_class19_devmgr_dar(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    /* ACL 广告 A1..A10 与方法 1..6（4307..4309 另加 127..129）：类定义但无来源 →
     * DAR 5；未在 ACL 先 DAR 3。 */
    static const uint8_t get_4300_a1[] = {0x05, 0x01, 0x05, 0x43, 0x00, 0x01, 0x00, 0x00};
    static const uint8_t get_4300_a10[] = {0x05, 0x01, 0x05, 0x43, 0x00, 0x0A, 0x00, 0x00};
    static const uint8_t get_4300_a11[] = {0x05, 0x01, 0x05, 0x43, 0x00, 0x0B, 0x00, 0x00};
    static const uint8_t get_4301_a2[] = {0x05, 0x01, 0x05, 0x43, 0x01, 0x02, 0x00, 0x00};
    assert_product_get_dar(get_4300_a1, sizeof(get_4300_a1), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_4300_a10, sizeof(get_4300_a10), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_4300_a11, sizeof(get_4300_a11), DLT698_DAR_DENIED);
    assert_product_get_dar(get_4301_a2, sizeof(get_4301_a2), DLT698_DAR_DENIED);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_action = dcu_north_accept_action;

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    /* 复位/执行/数据初始化/恢复出厂/事件初始化/需量初始化都广告，ACTION 到对象表
     * 如实回 DAR 5，绝不运行。 */
    for (uint8_t method = 1; method <= 6; method++) {
        uint8_t action_4300[9] = {0x07, 0x01, 0x05, 0x43, 0x00, method, 0x00, 0x00, 0x00};
        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        assert_int_equal(run_slave_apdu(action_4300, sizeof(action_4300), &src, NULL, out), 0);
        decode_payload(out, &payload, &svc);
        assert_int_equal(svc, DLT698_SVC_ACTION_RES);
        assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
        net_buf_unref(out);
    }

    static const uint8_t action_4300_7[] = {0x07, 0x01, 0x05, 0x43, 0x00,
                                            0x07, 0x00, 0x00, 0x00};
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_4300_7, sizeof(action_4300_7), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    /* 4307..4309 的 127..129 已计入 resolver 定义并进 ACL：ACTION 到对象表回
     * DAR 5；4300 没有 OI 专属方法，127 仍先被 ACL 拒（DAR 3）。 */
    static const uint16_t sub_devices[] = {0x4307, 0x4308, 0x4309};
    for (size_t i = 0; i < sizeof(sub_devices) / sizeof(sub_devices[0]); i++) {
        for (uint8_t method = 127; method <= 129; method++) {
            uint8_t action[9] = {0x07, 0x01, 0x05, (uint8_t)(sub_devices[i] >> 8),
                                 (uint8_t)sub_devices[i], method, 0x00, 0x00, 0x00};
            out = net_buf_alloc(&pool, 0);
            assert_non_null(out);
            assert_int_equal(run_slave_apdu(action, sizeof(action), &src, NULL, out), 0);
            decode_payload(out, &payload, &svc);
            assert_int_equal(svc, DLT698_SVC_ACTION_RES);
            assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
            net_buf_unref(out);
        }
    }

    static const uint8_t action_4300_127[] = {0x07, 0x01, 0x05, 0x43, 0x00,
                                              0x7F, 0x00, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_4300_127, sizeof(action_4300_127), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    static const uint8_t set_4300_a2[] = {0x06, 0x01, 0x05, 0x43, 0x00,
                                          0x02, 0x00, 0x09, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(set_4300_a2, sizeof(set_4300_a2), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
    dcu_698_object_model_reset();
}

/* D4-04 回归：118 项对象列表的 GET 响应是 5992 B 数据 + 22 B 帧 = 6014 B。
 * 生产共享池的 4096 B 装不下；旧码 net_buf 的 assert 在 NDEBUG 下被编译掉，
 * 直接越界写坏下一个 net_buf 节点。放不下时必须干净回 DAR 2。 */
static void test_slave_object_list_does_not_overflow_pool_buffer(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 4096, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t get_4400[] = {0x05, 0x01, 0x05, 0x44, 0x00, 0x02, 0x00, 0x00};
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    assert_int_equal(run_slave_apdu(get_4400, sizeof(get_4400), &src, NULL, out), 0);

    assert_true(out->b.len <= out->b.size);
    const uint8_t* end = out->b.__buf + out->b.size;
    for (int i = 0; i < 64; i++) assert_int_equal(end[i], 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(payload[4], 0x00); /* A-Result Data 的 DAR 分支 */
    assert_int_equal(payload[5], DLT698_DAR_TEMPORARY_FAILURE);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 生产从站池尺寸下，106 项对象列表必须完整装入，且缓冲末尾之后不被触碰。 */
static void test_slave_object_list_fits_production_slave_pool(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, BUS_SLAVE_POOL_BUF_SIZE, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static const uint8_t get_4400[] = {0x05, 0x01, 0x05, 0x44, 0x00, 0x02, 0x00, 0x00};
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    assert_int_equal(run_slave_apdu(get_4400, sizeof(get_4400), &src, NULL, out), 0);

    assert_int_equal(out->b.len, 6014);
    assert_true(out->b.len <= out->b.size);
    const uint8_t* end = out->b.__buf + out->b.size;
    for (int i = 0; i < 64; i++) assert_int_equal(end[i], 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(payload[4], 0x01); /* A-Result Data */
    assert_int_equal(payload[5], 0x01); /* array */
    assert_int_equal(payload[6], 118);  /* 对象列表项数 */

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

typedef struct {
    fake_record_t rec;
    size_t lens[3];
} list_buf_case_t;

static int list_case_object_list(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    list_buf_case_t* c = (list_buf_case_t*)ctx;
    return fake_object_list(&c->rec, oad, buf, cap, len);
}

static int list_case_resolve(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    list_buf_case_t* c = (list_buf_case_t*)ctx;
    uint16_t oi = (uint16_t)(oad >> 16);
    size_t idx = oi == 0x3000 ? 0 : (oi == 0x3005 ? 1 : 2);
    size_t n = c->lens[idx];
    if (n > cap) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    memset(buf, 0x5A, n);
    *len = n;
    return 0;
}

/* GET-NORMAL-LIST 请求：前 n 个 OI 的 A2。 */
static int run_list_case(list_buf_case_t* c, size_t n, net_buf_t* out) {
    static const uint16_t ois[3] = {0x3000, 0x3005, 0x3006};
    c->rec.acl_len = build_acl(c->rec.acl, ois, 3);
    uint8_t apdu[32];
    size_t o = 0;
    apdu[o++] = DLT698_SVC_GET_REQ;
    apdu[o++] = DLT698_CHOICE_GET_NORMAL_LIST;
    apdu[o++] = 0x01;
    apdu[o++] = (uint8_t)n;
    for (size_t i = 0; i < n; i++) {
        apdu[o++] = (uint8_t)(ois[i] >> 8);
        apdu[o++] = (uint8_t)ois[i];
        apdu[o++] = 0x02;
        apdu[o++] = 0x00;
    }
    apdu[o++] = 0x00; /* 时间标签 */
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = list_case_object_list;
    src.resolve_axdr = list_case_resolve;
    return run_slave_apdu(apdu, o, &src, c, out);
}

/* 列表项必须给帧尾预留 5 字节（跟随上报|时间标签|FCS|END）。生产池 10496：
 * 头 9 + APDU 4 + item1(5+10240) 后剩 238；item2 数据 229+10=239 > 238，
 * 必须回该项 DAR 2，end 才能封口。旧码写满后 end 失败，留下 L=0 的半帧。 */
static void test_slave_list_item_reserves_frame_trailer(void** state) {
    (void)state;
    list_buf_case_t c;
    memset(&c, 0, sizeof(c));
    c.lens[0] = 10240;
    c.lens[1] = 165;

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, BUS_SLAVE_POOL_BUF_SIZE, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_list_case(&c, 2, out), 0);

    assert_int_equal(out->b.data[out->b.len - 1], DLT698_END);
    uint16_t l = (uint16_t)(out->b.data[1] | ((uint16_t)out->b.data[2] << 8));
    assert_int_equal((size_t)l, out->b.len - 2);
    /* item1 从偏移 13 开始：OAD(4)+Data(1)+10240；item2 紧随其后回 DAR 2。 */
    size_t item2 = 13 + 5 + 10240;
    assert_int_equal(out->b.data[item2 + 4], 0x00);
    assert_int_equal(out->b.data[item2 + 5], DLT698_DAR_TEMPORARY_FAILURE);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 编码失败不得留下半帧：item2 写满后只剩 5 字节，下一项与其 DAR 都放不下，
 * 驱动必须清空 resp 长度；旧码会把半帧交给引擎并随每个 RX 事件重发。 */
static void test_slave_encode_failure_clears_half_frame(void** state) {
    (void)state;
    list_buf_case_t c;
    memset(&c, 0, sizeof(c));
    c.lens[0] = 10240;
    c.lens[1] = 164;
    c.lens[2] = 1;

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, BUS_SLAVE_POOL_BUF_SIZE, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    int rc = run_list_case(&c, 3, out);
    assert_true(rc < 0);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_slave_class21_esam_dar(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    /* ACL 广告 A1..A15 与方法 1..12：类定义但无 ESAM 来源 → DAR 5；未在 ACL 先 DAR 3。 */
    static const uint8_t get_f100_a1[] = {0x05, 0x01, 0x05, 0xF1, 0x00, 0x01, 0x00, 0x00};
    static const uint8_t get_f100_a15[] = {0x05, 0x01, 0x05, 0xF1, 0x00, 0x0F, 0x00, 0x00};
    static const uint8_t get_f100_a16[] = {0x05, 0x01, 0x05, 0xF1, 0x00, 0x10, 0x00, 0x00};
    static const uint8_t get_f101_a2[] = {0x05, 0x01, 0x05, 0xF1, 0x01, 0x02, 0x00, 0x00};
    assert_product_get_dar(get_f100_a1, sizeof(get_f100_a1), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_f100_a15, sizeof(get_f100_a15), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_f100_a16, sizeof(get_f100_a16), DLT698_DAR_DENIED);
    assert_product_get_dar(get_f101_a2, sizeof(get_f101_a2), DLT698_DAR_DENIED);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_action = dcu_north_accept_action;

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    /* 复位/执行/数据读取/更新/协商失效/钱包/密钥/证书：全部无处理器，DAR 5，绝不运行。 */
    for (uint8_t method = 1; method <= 8; method++) {
        uint8_t action_f100[9] = {0x07, 0x01, 0x05, 0xF1, 0x00, method, 0x00, 0x00, 0x00};
        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        assert_int_equal(run_slave_apdu(action_f100, sizeof(action_f100), &src, NULL, out), 0);
        decode_payload(out, &payload, &svc);
        assert_int_equal(svc, DLT698_SVC_ACTION_RES);
        assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
        net_buf_unref(out);
    }

    /* 方法 9..12 已计入 resolver 定义并进 ACL：ACTION 回 DAR 5；13 未定义，ACL 先 DAR 3。 */
    for (uint8_t method = 9; method <= 12; method++) {
        uint8_t action_f100[9] = {0x07, 0x01, 0x05, 0xF1, 0x00, method, 0x00, 0x00, 0x00};
        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        assert_int_equal(run_slave_apdu(action_f100, sizeof(action_f100), &src, NULL, out), 0);
        decode_payload(out, &payload, &svc);
        assert_int_equal(svc, DLT698_SVC_ACTION_RES);
        assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
        net_buf_unref(out);
    }

    static const uint8_t action_f100_13[] = {0x07, 0x01, 0x05, 0xF1, 0x00,
                                             0x0D, 0x00, 0x00, 0x00};
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_f100_13, sizeof(action_f100_13), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    static const uint8_t set_f100_a2[] = {0x06, 0x01, 0x05, 0xF1, 0x00,
                                          0x02, 0x00, 0x09, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(set_f100_a2, sizeof(set_f100_a2), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
    dcu_698_object_model_reset();
}

static void test_slave_class23_group_dar(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    /* ACL 广告 A1..A18 与方法 1..5：类定义但无来源 → DAR 5；A19 类未定义、
     * 未知 OI 都先 DAR 3。 */
    static const uint8_t get_2301_a1[] = {0x05, 0x01, 0x05, 0x23, 0x01, 0x01, 0x00, 0x00};
    static const uint8_t get_2301_a16[] = {0x05, 0x01, 0x05, 0x23, 0x01, 0x10, 0x00, 0x00};
    static const uint8_t get_2301_a17[] = {0x05, 0x01, 0x05, 0x23, 0x01, 0x11, 0x00, 0x00};
    static const uint8_t get_2301_a18[] = {0x05, 0x01, 0x05, 0x23, 0x01, 0x12, 0x00, 0x00};
    static const uint8_t get_2301_a19[] = {0x05, 0x01, 0x05, 0x23, 0x01, 0x13, 0x00, 0x00};
    static const uint8_t get_2309_a2[] = {0x05, 0x01, 0x05, 0x23, 0x09, 0x02, 0x00, 0x00};
    assert_product_get_dar(get_2301_a1, sizeof(get_2301_a1), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_2301_a16, sizeof(get_2301_a16), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_2301_a17, sizeof(get_2301_a17), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_2301_a18, sizeof(get_2301_a18), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_2301_a19, sizeof(get_2301_a19), DLT698_DAR_DENIED);
    assert_product_get_dar(get_2309_a2, sizeof(get_2309_a2), DLT698_DAR_DENIED);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_action = dcu_north_accept_action;

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    /* 清空/执行/添加/批量添加/删除：全部无处理器，DAR 5，绝不执行总加配置。 */
    for (uint8_t method = 1; method <= 5; method++) {
        uint8_t action_2301[9] = {0x07, 0x01, 0x05, 0x23, 0x01, method, 0x00, 0x00, 0x00};
        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        assert_int_equal(run_slave_apdu(action_2301, sizeof(action_2301), &src, NULL, out), 0);
        decode_payload(out, &payload, &svc);
        assert_int_equal(svc, DLT698_SVC_ACTION_RES);
        assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
        net_buf_unref(out);
    }

    static const uint8_t action_2301_6[] = {0x07, 0x01, 0x05, 0x23, 0x01,
                                            0x06, 0x00, 0x00, 0x00};
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_2301_6, sizeof(action_2301_6), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    static const uint8_t set_2301_a2[] = {0x06, 0x01, 0x05, 0x23, 0x01,
                                          0x02, 0x00, 0x09, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(set_2301_a2, sizeof(set_2301_a2), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    /* A17 已广告为只读：SET 仍由 ACL 回 DAR 3，不落到 resolver。 */
    static const uint8_t set_2301_a17[] = {0x06, 0x01, 0x05, 0x23, 0x01,
                                           0x11, 0x00, 0x09, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(set_2301_a17, sizeof(set_2301_a17), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
    dcu_698_object_model_reset();
}

static void test_slave_class18_file_dar(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    /* ACL：F000 广告 A1..A5，F001/F002 广告 A1..A4；方法 1..8。类定义但无来源 →
     * DAR 5；未在 ACL 先 DAR 3。 */
    static const uint8_t get_f000_a1[] = {0x05, 0x01, 0x05, 0xF0, 0x00, 0x01, 0x00, 0x00};
    static const uint8_t get_f000_a5[] = {0x05, 0x01, 0x05, 0xF0, 0x00, 0x05, 0x00, 0x00};
    static const uint8_t get_f000_a6[] = {0x05, 0x01, 0x05, 0xF0, 0x00, 0x06, 0x00, 0x00};
    static const uint8_t get_f001_a4[] = {0x05, 0x01, 0x05, 0xF0, 0x01, 0x04, 0x00, 0x00};
    static const uint8_t get_f001_a5[] = {0x05, 0x01, 0x05, 0xF0, 0x01, 0x05, 0x00, 0x00};
    static const uint8_t get_f002_a4[] = {0x05, 0x01, 0x05, 0xF0, 0x02, 0x04, 0x00, 0x00};
    static const uint8_t get_f003_a2[] = {0x05, 0x01, 0x05, 0xF0, 0x03, 0x02, 0x00, 0x00};
    assert_product_get_dar(get_f000_a1, sizeof(get_f000_a1), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_f000_a5, sizeof(get_f000_a5), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_f000_a6, sizeof(get_f000_a6), DLT698_DAR_DENIED);
    assert_product_get_dar(get_f001_a4, sizeof(get_f001_a4), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_f001_a5, sizeof(get_f001_a5), DLT698_DAR_DENIED);
    assert_product_get_dar(get_f002_a4, sizeof(get_f002_a4), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_product_get_dar(get_f003_a2, sizeof(get_f003_a2), DLT698_DAR_DENIED);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_action = dcu_north_accept_action;

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    /* 复位/执行/删除/校验/代发/代收/上传/下载：全部无处理器，DAR 5，绝不传输。 */
    for (uint8_t method = 1; method <= 8; method++) {
        uint8_t action_f000[9] = {0x07, 0x01, 0x05, 0xF0, 0x00, method, 0x00, 0x00, 0x00};
        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        assert_int_equal(run_slave_apdu(action_f000, sizeof(action_f000), &src, NULL, out), 0);
        decode_payload(out, &payload, &svc);
        assert_int_equal(svc, DLT698_SVC_ACTION_RES);
        assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
        net_buf_unref(out);
    }

    /* F001 的 OI 方法 9/10 已计入 resolver 定义并进 ACL：ACTION 回 DAR 5；
     * F000/F002 没有 9/10，ACL 先回 DAR 3。 */
    static const uint8_t action_f001_9[] = {0x07, 0x01, 0x05, 0xF0, 0x01,
                                            0x09, 0x00, 0x00, 0x00};
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_f001_9, sizeof(action_f001_9), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
    net_buf_unref(out);

    static const uint8_t action_f001_10[] = {0x07, 0x01, 0x05, 0xF0, 0x01,
                                             0x0A, 0x00, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_f001_10, sizeof(action_f001_10), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_OBJECT_CLASS_MISMATCH);
    net_buf_unref(out);

    static const uint8_t action_f000_9[] = {0x07, 0x01, 0x05, 0xF0, 0x00,
                                            0x09, 0x00, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_f000_9, sizeof(action_f000_9), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    static const uint8_t action_f002_9[] = {0x07, 0x01, 0x05, 0xF0, 0x02,
                                            0x09, 0x00, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(action_f002_9, sizeof(action_f002_9), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);
    net_buf_unref(out);

    static const uint8_t set_f000_a2[] = {0x06, 0x01, 0x05, 0xF0, 0x00,
                                          0x02, 0x00, 0x09, 0x00, 0x00};
    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(set_f000_a2, sizeof(set_f000_a2), &src, NULL, out), 0);
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
    dcu_698_object_model_reset();
}

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
    static const uint8_t set_5004[] = {0x06, 0x01, 0x05, 0x50, 0x04, 0x02, 0x00, 0x09, 0x00, 0x00};
    assert_int_equal(run_slave_apdu(set_5004, sizeof(set_5004), &src, &f, out), 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], DLT698_DAR_DENIED);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_slave_rejects_over_cap_object_list(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    uint16_t ois[225];
    for (size_t i = 0; i < 225; i++) ois[i] = (uint16_t)(0x2000 + i);
    fake_record_t f;
    memset(&f, 0, sizeof(f));
    f.acl_len = build_acl(f.acl, ois, 225);

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

typedef struct {
    uint8_t acl[EDGE_SLAVE_AXDR_MAX];
    size_t acl_len;
    int resolve_calls;
} big_acl_t;

static int big_acl_object_list(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    big_acl_t* b = (big_acl_t*)ctx;
    if (oad != 0x44000200u) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (b->acl_len > cap) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    memcpy(buf, b->acl, b->acl_len);
    *len = b->acl_len;
    return 0;
}

static int big_acl_resolve(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    (void)oad;
    (void)buf;
    (void)cap;
    (void)len;
    ((big_acl_t*)ctx)->resolve_calls++;
    return EDGE_SLAVE_SRC_NOT_FOUND;
}

static void test_slave_rejects_acl_method_pool_overflow(void** state) {
    (void)state;
    static big_acl_t b;
    memset(&b, 0, sizeof(b));
    size_t off = 0;
    b.acl[off++] = 0x01;
    b.acl[off++] = 0x81;
    b.acl[off++] = 100;
    for (uint8_t o = 0; o < 100; o++) {
        b.acl[off++] = 0x02;
        b.acl[off++] = 0x02;
        b.acl[off++] = 0x50;
        b.acl[off++] = 0x00;
        b.acl[off++] = 0x01;
        b.acl[off++] = 0x02;
        b.acl[off++] = 0x02;
        b.acl[off++] = 0x01;
        b.acl[off++] = (o == 0) ? 1 : 0;
        if (o == 0) {
            /* 第一只对象广告属性 2：整单被池界拒绝时它也不得可用。 */
            b.acl[off++] = 0x02;
            b.acl[off++] = 0x02;
            b.acl[off++] = 0x11;
            b.acl[off++] = 0x02;
            b.acl[off++] = 0x16;
            b.acl[off++] = 0x01;
        }
        b.acl[off++] = 0x01;
        b.acl[off++] = 12;
        for (uint8_t m = 1; m <= 12; m++) {
            b.acl[off++] = 0x02;
            b.acl[off++] = 0x02;
            b.acl[off++] = 0x11;
            b.acl[off++] = m;
            b.acl[off++] = 0x03;
            b.acl[off++] = 0x01;
        }
    }
    assert_true(off <= sizeof(b.acl));
    b.acl_len = off;

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = big_acl_object_list;
    src.resolve_axdr = big_acl_resolve;
    static const uint8_t get_0001[] = {0x05, 0x01, 0x05, 0x00, 0x01, 0x02, 0x00, 0x00};
    assert_int_equal(run_slave_apdu(get_0001, sizeof(get_0001), &src, &b, out), 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(payload[4], 0x00);
    assert_int_equal(payload[5], DLT698_DAR_DENIED);
    assert_int_equal(b.resolve_calls, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

typedef struct {
    struct tm value;
    int fail_set;
} slave_clock_state_t;

static int test_clock_now(void* ctx, struct tm* out) {
    const slave_clock_state_t* st = (const slave_clock_state_t*)ctx;
    *out = st->value;
    return 0;
}

static int test_clock_set(void* ctx, const struct tm* in) {
    slave_clock_state_t* st = (slave_clock_state_t*)ctx;
    if (st->fail_set) return -1;
    st->value = *in;
    return 0;
}

static void assert_slave_set_4000_dar(const edge_slave_source_ops_t* src, void* ctx,
                                      const uint8_t* apdu, size_t apdu_len, uint8_t want_dar) {
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(apdu, apdu_len, src, ctx, out), 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(payload[4], want_dar);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void assert_slave_get_4000_bytes(const edge_slave_source_ops_t* src, void* ctx,
                                        const uint8_t* want_8bytes) {
    static const uint8_t get_4000[] = {0x05, 0x01, 0x05, 0x40, 0x00, 0x02, 0x00, 0x00};
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(run_slave_apdu(get_4000, sizeof(get_4000), src, ctx, out), 0);

    uint8_t* payload = NULL;
    uint8_t svc = 0;
    decode_payload(out, &payload, &svc);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(payload[4], 0x01);
    assert_memory_equal(payload + 5, want_8bytes, 8);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_slave_set_4000_a2_dar_contract(void** state) {
    (void)state;
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink;
    router_bus_ops_t bus;
    memset(&sink, 0, sizeof(sink));
    memset(&bus, 0, sizeof(bus));
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus),
                     0);

    slave_clock_state_t clock_state = {
        .value =
            {.tm_year = 124, .tm_mon = 4, .tm_mday = 6, .tm_hour = 7, .tm_min = 8, .tm_sec = 9},
        .fail_set = 0};
    dcu_clock_port_t clock = {
        .now = test_clock_now, .set = test_clock_set, .ctx = &clock_state};
    dcu_698_param_set_clock(&clock);
    dcu_698_param_reset();

    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = product_object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_axdr = dcu_north_accept_axdr;

    static const uint8_t t_new[] = {0x1C, 0x07, 0xE9, 0x01, 0x02, 0x03, 0x04, 0x05};
    static const uint8_t t_later[] = {0x1C, 0x07, 0xEA, 0x02, 0x03, 0x04, 0x05, 0x06};
    static const uint8_t set_valid[] = {0x06, 0x01, 0x05, 0x40, 0x00, 0x02, 0x00, 0x1C,
                                        0x07, 0xE9, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00};
    static const uint8_t set_later[] = {0x06, 0x01, 0x05, 0x40, 0x00, 0x02, 0x00, 0x1C,
                                        0x07, 0xEA, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00};
    static const uint8_t set_bad[] = {0x06, 0x01, 0x05, 0x40, 0x00, 0x02, 0x00, 0x11, 0x01, 0x00};

    assert_slave_set_4000_dar(&src, NULL, set_valid, sizeof(set_valid), DLT698_DAR_SUCCESS);
    assert_slave_get_4000_bytes(&src, NULL, t_new);

    clock_state.fail_set = 1;
    assert_slave_set_4000_dar(&src, NULL, set_later, sizeof(set_later),
                              DLT698_DAR_TEMPORARY_FAILURE);
    assert_slave_get_4000_bytes(&src, NULL, t_new);

    clock_state.fail_set = 0;
    assert_slave_set_4000_dar(&src, NULL, set_bad, sizeof(set_bad), DLT698_DAR_TYPE_MISMATCH);
    assert_slave_get_4000_bytes(&src, NULL, t_new);

    fake_record_t ro;
    memset(&ro, 0, sizeof(ro));
    static const uint16_t ro_ois[] = {0x4000};
    ro.acl_len = build_acl(ro.acl, ro_ois, 1);
    edge_slave_source_ops_t ro_src;
    memset(&ro_src, 0, sizeof(ro_src));
    ro_src.object_list = fake_object_list;
    ro_src.resolve_axdr = dcu_north_resolve_axdr;
    ro_src.accept_axdr = dcu_north_accept_axdr;
    assert_slave_set_4000_dar(&ro_src, &ro, set_later, sizeof(set_later), DLT698_DAR_DENIED);
    assert_slave_get_4000_bytes(&src, NULL, t_new);
    (void)t_later;

    dcu_698_object_model_reset();
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
        cmocka_unit_test(test_slave_parses_long_form_acl_224),
        cmocka_unit_test(test_get_maps_source_rc_to_dar),
        cmocka_unit_test(test_slave_accepts_product_object_list_42),
        cmocka_unit_test(test_slave_class17_display_dar),
        cmocka_unit_test(test_slave_class11_collection_dar),
        cmocka_unit_test(test_slave_class12_pulse_dar),
        cmocka_unit_test(test_slave_class25_26_comm_dar),
        cmocka_unit_test(test_slave_class19_devmgr_dar),
        cmocka_unit_test(test_slave_class21_esam_dar),
        cmocka_unit_test(test_slave_class23_group_dar),
        cmocka_unit_test(test_slave_class18_file_dar),
        cmocka_unit_test(test_slave_object_list_does_not_overflow_pool_buffer),
        cmocka_unit_test(test_slave_object_list_fits_production_slave_pool),
        cmocka_unit_test(test_slave_list_item_reserves_frame_trailer),
        cmocka_unit_test(test_slave_encode_failure_clears_half_frame),
        cmocka_unit_test(test_slave_set_read_only_is_denied),
        cmocka_unit_test(test_slave_rejects_over_cap_object_list),
        cmocka_unit_test(test_slave_rejects_acl_method_pool_overflow),
        cmocka_unit_test(test_slave_set_4000_a2_dar_contract),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
