/**
 * @file test_dlt698_appconn.c
 * @brief Class 20 应用连接：CONNECT/RELEASE 与对象列表 ACL（ADR-0015）。
 *
 * 字节布局依据标准表59/表60/表61/表62 与附录 H.2；对象列表见表182。
 */
#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dlt698_codec.h"
#include "driver_dlt698.h"
#include "edge_driver.h"
#include "edge_slave_plane.h"
#include "net_buf.h"

extern const edge_driver_t dlt698_driver;

static size_t build_frame(uint8_t* out, const uint8_t* apdu, size_t apdu_len) {
    size_t n = 0;
    out[n++] = DLT698_HEAD;
    size_t len_off = n;
    n += 2;
    out[n++] = 0x43;
    out[n++] = 0x00;
    out[n++] = 0x01;
    out[n++] = 0x10;
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

static void assert_response(const net_buf_t* out, const uint8_t* want, size_t want_len) {
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
    assert_int_equal(payload_len + 3, want_len);
    assert_memory_equal(payload - 3, want, want_len);
}

typedef struct {
    uint8_t acl[128];
    size_t acl_len;
} fake_ctx_t;

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
        buf[o++] = 3; /* 可读写 */
        buf[o++] = 0x01;
        buf[o++] = 0;
    }
    return o;
}

static int fake_object_list(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    fake_ctx_t* f = (fake_ctx_t*)ctx;
    if (oad != 0x44000200u) return -1;
    if (f->acl_len > cap) return -1;
    memcpy(buf, f->acl, f->acl_len);
    *len = f->acl_len;
    return 0;
}

static int fake_resolve_axdr(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    fake_ctx_t* f = (fake_ctx_t*)ctx;
    (void)f;
    if (oad == 0x50040200u) {
        static const uint8_t value[] = {0x12, 0x00, 0x28};
        memcpy(buf, value, sizeof(value));
        *len = sizeof(value);
        return 0;
    }
    return EDGE_SLAVE_SRC_NOT_FOUND;
}

static int exchange(uint8_t* session, const uint8_t* apdu, size_t apdu_len, fake_ctx_t* f,
                    net_buf_t* out) {
    uint8_t frame[320];
    size_t flen = build_frame(frame, apdu, apdu_len);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.resolve_axdr = fake_resolve_axdr;
    src.object_list = fake_object_list;
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    return dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, f, out, &consumed);
}

static const uint8_t k_connect_apdu[] = {
    0x02, 0x00, 0x00, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0x00, 0x04, 0x00, 0x01, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x64, 0x00, 0x00,
};

static void test_connect_ok_and_get_allowed(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    fake_ctx_t f;
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);

    assert_int_equal(exchange(session, k_connect_apdu, sizeof(k_connect_apdu), &f, out), 0);
    static const uint8_t want_connect[] = {
        0x82, 0x00, 'E', 'D', 'G', 'E', '0', '0', '0', '1', '2', '6', '1', '1', '2', '2',
        '0', '0', '0', '1', '2', '6', '1', '1', '2', '2', 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0x04, 0x00, 0x04, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00,
        0x64, 0x00, 0x00, 0x00, 0x00,
    };
    assert_response(out, want_connect, sizeof(want_connect));

    net_buf_t* out2 = net_buf_alloc(&pool, 0);
    assert_non_null(out2);
    static const uint8_t get_allowed[] = {0x05, 0x01, 0x02, 0x50, 0x04, 0x02, 0x00, 0x00};
    assert_int_equal(exchange(session, get_allowed, sizeof(get_allowed), &f, out2), 0);
    static const uint8_t want_get[] = {0x85, 0x01, 0x02, 0x50, 0x04, 0x02, 0x00,
                                       0x01, 0x12, 0x00, 0x28, 0x00, 0x00};
    assert_response(out2, want_get, sizeof(want_get));

    net_buf_unref(out);
    net_buf_unref(out2);
    net_buf_pool_destroy(&pool);
}

static void test_connect_rejects_bad_version(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    fake_ctx_t f;
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);

    uint8_t apdu[sizeof(k_connect_apdu)];
    memcpy(apdu, k_connect_apdu, sizeof(apdu));
    apdu[3] = 0x11; /* 版本 0x0011 不支持 */
    assert_int_equal(exchange(session, apdu, sizeof(apdu), &f, out), 0);

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
    assert_int_equal(payload[payload_len - 4], 5); /* 协议版本不匹配（表43） */

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_connect_malformed_returns_other_error(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    fake_ctx_t f;
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);

    static const uint8_t short_apdu[] = {0x02, 0x00, 0x00, 0x10, 0x00, 0x00};
    assert_int_equal(exchange(session, short_apdu, sizeof(short_apdu), &f, out), 0);

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
    assert_int_equal(payload[payload_len - 4], 255);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_data_denied_without_list_entry(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    fake_ctx_t f;
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);

    /* 未 CONNECT：预连接窗口也只允许列表内的 OAD。 */
    static const uint8_t get_denied[] = {0x05, 0x01, 0x02, 0x20, 0x00, 0x02, 0x00, 0x00};
    assert_int_equal(exchange(session, get_denied, sizeof(get_denied), &f, out), 0);
    static const uint8_t want_get[] = {0x85, 0x01, 0x02, 0x20, 0x00, 0x02, 0x00,
                                       0x00, 0x03, 0x00, 0x00};
    assert_response(out, want_get, sizeof(want_get));

    net_buf_t* out2 = net_buf_alloc(&pool, 0);
    assert_non_null(out2);
    static const uint8_t set_denied[] = {0x06, 0x01, 0x02, 0x20, 0x00, 0x02,
                                         0x00, 0x12, 0x00, 0x01, 0x00};
    assert_int_equal(exchange(session, set_denied, sizeof(set_denied), &f, out2), 0);
    static const uint8_t want_set[] = {0x86, 0x01, 0x02, 0x20, 0x00, 0x02,
                                       0x00, 0x03, 0x00, 0x00};
    assert_response(out2, want_set, sizeof(want_set));

    net_buf_t* out3 = net_buf_alloc(&pool, 0);
    assert_non_null(out3);
    static const uint8_t action_denied[] = {0x07, 0x01, 0x02, 0x20, 0x00, 0x01, 0x00, 0x00};
    assert_int_equal(exchange(session, action_denied, sizeof(action_denied), &f, out3), 0);
    static const uint8_t want_action[] = {0x87, 0x01, 0x02, 0x20, 0x00, 0x01,
                                          0x00, 0x03, 0x00, 0x00, 0x00};
    assert_response(out3, want_action, sizeof(want_action));

    net_buf_unref(out);
    net_buf_unref(out2);
    net_buf_unref(out3);
    net_buf_pool_destroy(&pool);
}

static void test_second_session_does_not_inherit_acl(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static uint8_t session_a[1024];
    static uint8_t session_b[1024];
    memset(session_a, 0, sizeof(session_a));
    memset(session_b, 0, sizeof(session_b));

    fake_ctx_t allow;
    static const uint16_t ois_allow[] = {0x5004};
    allow.acl_len = build_acl(allow.acl, ois_allow, 1);

    fake_ctx_t deny;
    static const uint16_t ois_deny[] = {0x6000};
    deny.acl_len = build_acl(deny.acl, ois_deny, 1);

    static const uint8_t get_5004[] = {0x05, 0x01, 0x02, 0x50, 0x04, 0x02, 0x00, 0x00};
    assert_int_equal(exchange(session_a, get_5004, sizeof(get_5004), &allow, out), 0);
    static const uint8_t want_allow[] = {0x85, 0x01, 0x02, 0x50, 0x04, 0x02, 0x00,
                                         0x01, 0x12, 0x00, 0x28, 0x00, 0x00};
    assert_response(out, want_allow, sizeof(want_allow));

    net_buf_t* out2 = net_buf_alloc(&pool, 0);
    assert_non_null(out2);
    assert_int_equal(exchange(session_b, get_5004, sizeof(get_5004), &deny, out2), 0);
    static const uint8_t want_deny[] = {0x85, 0x01, 0x02, 0x50, 0x04, 0x02, 0x00,
                                        0x00, 0x03, 0x00, 0x00};
    assert_response(out2, want_deny, sizeof(want_deny));

    net_buf_unref(out);
    net_buf_unref(out2);
    net_buf_pool_destroy(&pool);
}

static void test_release_response(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    fake_ctx_t f;
    static const uint16_t ois[] = {0x5004};
    f.acl_len = build_acl(f.acl, ois, 1);

    assert_int_equal(exchange(session, k_connect_apdu, sizeof(k_connect_apdu), &f, out), 0);
    net_buf_unref(out);

    out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    static const uint8_t release_apdu[] = {0x03, 0x00, 0x00};
    assert_int_equal(exchange(session, release_apdu, sizeof(release_apdu), &f, out), 0);
    static const uint8_t want[] = {0x83, 0x00, 0x00, 0x00, 0x00};
    assert_response(out, want, sizeof(want));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_connect_ok_and_get_allowed),
        cmocka_unit_test(test_connect_rejects_bad_version),
        cmocka_unit_test(test_connect_malformed_returns_other_error),
        cmocka_unit_test(test_data_denied_without_list_entry),
        cmocka_unit_test(test_second_session_does_not_inherit_acl),
        cmocka_unit_test(test_release_response),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
