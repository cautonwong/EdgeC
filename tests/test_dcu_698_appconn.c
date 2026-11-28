#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_appconn.h"
#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_north_adapter.h"
#include "dcu_router.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dlt698_codec.h"
#include "edge_driver.h"
#include "edge_slave_plane.h"
#include "net_buf.h"

extern const edge_driver_t dlt698_driver;

static int contains(const uint8_t *hay, size_t hay_len, const uint8_t *needle, size_t needle_len) {
    for (size_t i = 0; i + needle_len <= hay_len; i++) {
        if (memcmp(hay + i, needle, needle_len) == 0) return 1;
    }
    return 0;
}

static void test_object_list_shape_and_capacity(void **state) {
    (void)state;
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(dcu_698_appconn_object_list(buf, sizeof(buf), &len), 0);
    assert_true(len > 0 && len <= EDGE_SLAVE_AXDR_MAX);
    assert_int_equal(buf[0], 0x01); /* array */
    assert_int_equal(buf[1], 0x81); /* A-XDR 长格式 1 字节长度 */
    assert_int_equal(buf[2], 138);
    assert_int_equal(buf[3], 0x02); /* structure */
    assert_int_equal(buf[4], 0x02);

    static const uint8_t param_4000[] = {
        0x02, 0x02, 0x50, 0x40, 0x00, 0x02, 0x02, 0x01, 0x03, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03,
        0x02, 0x02, 0x11, 0x03, 0x16, 0x03, 0x02, 0x02, 0x11, 0x04, 0x16, 0x03, 0x01, 0x00,
    };
    assert_true(contains(buf, len, param_4000, sizeof(param_4000)));

    uint8_t small[4];
    size_t small_len = 0;
    assert_int_not_equal(dcu_698_appconn_object_list(small, sizeof(small), &small_len), 0);
}

#define ACL_MAX_RIGHTS 256
typedef struct {
    bool found;
    bool seen[ACL_MAX_RIGHTS];
    uint8_t rights[ACL_MAX_RIGHTS];
    bool method_seen[ACL_MAX_RIGHTS];
    uint8_t method_rights[ACL_MAX_RIGHTS];
} acl_rights_t;

/* 对象列表（表182）：array { structure { OI, { 属性表, 方法表 } } }。
 * 属性 = structure { unsigned id, enum rights }，方法 = structure { unsigned id, 03 01 }。 */
static int _acl_rights_for(uint16_t oi, acl_rights_t *out) {
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    if (dcu_698_appconn_object_list(buf, sizeof(buf), &len) != 0 || len < 2) return -1;
    memset(out, 0, sizeof(*out));
    size_t off = 0;
    if (buf[off++] != 0x01 || off >= len) return -1;
    uint16_t n = 0;
    uint8_t first = buf[off++];
    if (first & 0x80) {
        uint8_t nbytes = (uint8_t)(first & 0x7Fu);
        if (nbytes == 0 || nbytes > 2 || len - off < nbytes) return -1;
        for (uint8_t b = 0; b < nbytes; b++) n = (uint16_t)((n << 8) | buf[off++]);
    } else {
        n = first;
    }
    for (uint16_t i = 0; i < n; i++) {
        if (off + 7 > len || buf[off++] != 0x02 || buf[off++] != 0x02 || buf[off++] != 0x50)
            return -1;
        uint16_t cur = (uint16_t)((buf[off] << 8) | buf[off + 1]);
        off += 2;
        if (off + 3 > len || buf[off++] != 0x02 || buf[off++] != 0x02 || buf[off++] != 0x01)
            return -1;
        uint8_t na = buf[off++];
        for (uint8_t a = 0; a < na; a++) {
            if (off + 4 > len || buf[off++] != 0x02 || buf[off++] != 0x02 ||
                buf[off++] != 0x11) {
                return -1;
            }
            uint8_t id = buf[off++];
            if (off >= len || buf[off++] != 0x16 || off >= len) return -1;
            uint8_t rights = buf[off++];
            if (cur == oi) {
                out->found = true;
                out->seen[id] = true;
                out->rights[id] = rights;
            }
        }
        if (off >= len || buf[off++] != 0x01 || off >= len) return -1;
        uint8_t nm = buf[off++];
        for (uint8_t m = 0; m < nm; m++) {
            if (off + 6 > len || buf[off++] != 0x02 || buf[off++] != 0x02 ||
                buf[off++] != 0x11) {
                return -1;
            }
            uint8_t id = buf[off++];
            if (off >= len || buf[off++] != 0x03 || off >= len) return -1;
            uint8_t rights = buf[off++];
            if (cur == oi) {
                out->found = true;
                out->method_seen[id] = true;
                out->method_rights[id] = rights;
            }
        }
        if (cur == oi) out->found = true;
    }
    return off == len ? 0 : -1;
}
static int _object_list(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    (void)oad;
    return dcu_698_appconn_object_list(buf, cap, len) == 0 ? 0 : EDGE_SLAVE_SRC_TEMPORARY;
}
static size_t _build_frame(uint8_t *out, const uint8_t *apdu, size_t apdu_len) {
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
static void _exchange_apdu(const uint8_t *apdu, size_t apdu_len, uint8_t expected_svc,
                           uint8_t *out_payload, size_t max_payload, size_t *out_len) {
    uint8_t frame[64];
    size_t flen = _build_frame(frame, apdu, apdu_len);
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = _object_list;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_axdr = dcu_north_accept_axdr;
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    static uint8_t backing[512];
    net_buf_t out;
    memset(&out, 0, sizeof(out));
    net_buf_simple_init_with_data(&out.b, backing, sizeof(backing));
    size_t consumed = 0;
    assert_int_equal(dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, NULL, &out,
                                                      &consumed),
                     0);
    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, out.b.data, out.b.len);
    nb.len = out.b.len;
    dlt698_addr_t sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload, &payload_len),
                     0);
    assert_int_equal(svc, expected_svc);
    assert_true(payload_len <= max_payload);
    memcpy(out_payload, payload, payload_len);
    *out_len = payload_len;
}
static uint8_t _set_dar_oi(uint16_t oi, uint8_t attr, const uint8_t *value, size_t value_len) {
    uint8_t apdu[32];
    size_t n = 0;
    apdu[n++] = DLT698_SVC_SET_REQ;
    apdu[n++] = 0x01;
    apdu[n++] = 0x05;
    apdu[n++] = (uint8_t)(oi >> 8);
    apdu[n++] = (uint8_t)oi;
    apdu[n++] = attr;
    apdu[n++] = 0x00;
    memcpy(apdu + n, value, value_len);
    n += value_len;
    uint8_t payload[64];
    size_t plen = 0;
    _exchange_apdu(apdu, n, DLT698_SVC_SET_RES, payload, sizeof(payload), &plen);
    assert_true(plen >= 5);
    return payload[4]; /* OAD(4) | DAR | FollowReport | 时间标签 */
}
static uint8_t _set_dar(uint8_t attr, const uint8_t *value, size_t value_len) {
    return _set_dar_oi(0x3000, attr, value, value_len);
}
static uint8_t _get_dar_oi(uint16_t oi, uint8_t attr) {
    uint8_t apdu[8] = {
        DLT698_SVC_GET_REQ, 0x01, 0x05, (uint8_t)(oi >> 8), (uint8_t)oi, attr, 0x00,
    };
    uint8_t payload[64];
    size_t plen = 0;
    _exchange_apdu(apdu, sizeof(apdu), DLT698_SVC_GET_RES, payload, sizeof(payload), &plen);
    assert_true(plen >= 6);
    assert_int_equal(payload[4], 0x00);
    return payload[5];
}
static dcu_inventory_t g_inv;
static dcu_poller_t g_poller;
static dcu_event_obj_t g_evt;
static int setup_model(void **state) {
    (void)state;
    assert_int_equal(dcu_inventory_init(&g_inv), 0);
    assert_int_equal(dcu_poller_init(&g_poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(dcu_698_object_model_init(&g_inv, &g_poller, sink, NULL, NULL, &g_evt, NULL,
                                               bus),
                     0);
    return 0;
}
static int teardown_model(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    return 0;
}
static void test_event_acl_advertises_only_a11_a12_writable(void **state) {
    (void)state;
    acl_rights_t acl;
    assert_int_equal(_acl_rights_for(0x3000, &acl), 0);
    assert_true(acl.seen[6] && acl.rights[6] == 1);
    assert_true(acl.seen[8] && acl.rights[8] == 3);
    assert_true(acl.seen[9] && acl.rights[9] == 3);
    assert_true(acl.seen[11] && acl.rights[11] == 3);
    assert_true(acl.seen[12] && acl.rights[12] == 3);
}
static void test_event_set_dar_by_attribute(void **state) {
    (void)state;
    const uint8_t empty_array[] = {DLT698_TAG_ARRAY, 0};
    assert_int_equal(dcu_north_accept_axdr(NULL, 0x30000600u, empty_array, sizeof(empty_array)),
                     EDGE_SLAVE_SRC_DENIED);
    assert_int_equal(_set_dar(6, empty_array, sizeof(empty_array)), DLT698_DAR_DENIED);
    assert_int_equal(_set_dar(8, empty_array, sizeof(empty_array)),
                     DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_int_equal(_set_dar(9, empty_array, sizeof(empty_array)),
                     DLT698_DAR_OBJECT_CLASS_MISMATCH);
    const uint8_t report[] = {DLT698_TAG_ENUM, 1};
    assert_int_equal(_set_dar(11, report, sizeof(report)), DLT698_DAR_SUCCESS);
    const uint8_t enabled[] = {DLT698_TAG_BOOLEAN, 1};
    assert_int_equal(_set_dar(12, enabled, sizeof(enabled)), DLT698_DAR_SUCCESS);
    const uint8_t bad_report[] = {DLT698_TAG_ENUM, 4};
    assert_int_equal(_set_dar(11, bad_report, sizeof(bad_report)), DLT698_DAR_TYPE_MISMATCH);
}
static void test_class22_acl_advertises_read_only_attrs(void **state) {
    (void)state;
    static const uint16_t ois[] = {0xF201, 0xF202, 0xF203, 0xF204, 0xF205, 0xF206,
                                   0xF207, 0xF208, 0xF209, 0xF20A, 0xF20B, 0xF20C};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        acl_rights_t acl;
        assert_int_equal(_acl_rights_for(ois[i], &acl), 0);
        assert_true(acl.seen[2]);
        assert_int_equal(acl.rights[2], 1);
    }
    acl_rights_t acl;
    assert_int_equal(_acl_rights_for(0xF203, &acl), 0);
    assert_int_equal(acl.rights[4], 1);
    assert_int_equal(_acl_rights_for(0xF204, &acl), 0);
    assert_int_equal(acl.rights[4], 1);
    assert_int_equal(_acl_rights_for(0xF206, &acl), 0);
    assert_int_equal(acl.rights[4], 1);
    assert_int_equal(_acl_rights_for(0xF209, &acl), 0);
    assert_int_equal(acl.rights[5], 1);
    assert_int_equal(acl.rights[6], 1);
    assert_int_equal(_acl_rights_for(0xF20C, &acl), 0);
    assert_int_equal(acl.rights[4], 1);
    assert_int_equal(acl.rights[5], 1);
}
static void test_class14_15_16_acl_is_read_only(void **state) {
    (void)state;
    static const uint16_t ois[] = {0x2100, 0x2101, 0x2102, 0x2103, 0x2104, 0x2110, 0x2111,
                                   0x2112, 0x2113, 0x2114, 0x2120, 0x2121, 0x2122, 0x2123,
                                   0x2124};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        acl_rights_t acl;
        assert_int_equal(_acl_rights_for(ois[i], &acl), 0);
        for (uint8_t attr = 1; attr <= 3; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (uint8_t method = 1; method <= 4; method++) {
            assert_true(acl.method_seen[method]);
            assert_int_equal(acl.method_rights[method], 1);
        }
    }
}
static void test_class17_acl_is_read_only(void **state) {
    (void)state;
    static const uint16_t ois[] = {0xF300, 0xF301};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        acl_rights_t acl;
        assert_int_equal(_acl_rights_for(ois[i], &acl), 0);
        assert_false(acl.seen[1]);
        for (uint8_t attr = 2; attr <= 4; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (uint8_t method = 1; method <= 6; method++) {
            assert_true(acl.method_seen[method]);
            assert_int_equal(acl.method_rights[method], 1);
        }
    }
}
static void test_class11_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    assert_int_equal(_acl_rights_for(0x7000, &acl), 0);
    for (int attr = 1; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
    for (uint8_t method = 127; method <= 129; method++) {
        assert_true(acl.method_seen[method]);
        assert_int_equal(acl.method_rights[method], 1);
    }
    assert_false(acl.method_seen[1]);
    assert_false(acl.method_seen[130]);

    assert_int_equal(_acl_rights_for(0x7010, &acl), 0);
    assert_false(acl.seen[1]);
    assert_true(acl.seen[2]);
    assert_int_equal(acl.rights[2], 1);
    assert_false(acl.seen[3]);
    assert_false(acl.seen[4]);
    for (uint8_t method = 127; method <= 130; method++) {
        assert_true(acl.method_seen[method]);
        assert_int_equal(acl.method_rights[method], 1);
    }
    assert_false(acl.method_seen[1]);
    assert_false(acl.method_seen[131]);
}
static void test_class12_pulse_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    /* 表157 类定义 A1..A19；16/17 的归属在表157/表158 间冲突、18/19 未裁决，
     * ACL 只广告 1..15。 */
    for (uint16_t oi = 0x2401; oi <= 0x2408; oi++) {
        assert_int_equal(_acl_rights_for(oi, &acl), 0);
        assert_false(acl.seen[0]);
        for (uint8_t attr = 1; attr <= 15; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (int attr = 16; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
        for (uint8_t method = 1; method <= 4; method++) {
            assert_true(acl.method_seen[method]);
            assert_int_equal(acl.method_rights[method], 1);
        }
        assert_false(acl.method_seen[5]);
    }
}

static void test_class25_26_comm_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    /* 表196/199 的类定义属性全部只读广告；方法只有标准定义的方法 1（复位）。 */
    static const uint16_t wireless[] = {0x4500, 0x4501};
    for (size_t i = 0; i < sizeof(wireless) / sizeof(wireless[0]); i++) {
        assert_int_equal(_acl_rights_for(wireless[i], &acl), 0);
        assert_false(acl.seen[0]);
        for (uint8_t attr = 1; attr <= 11; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (int attr = 12; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
        assert_true(acl.method_seen[1]);
        assert_int_equal(acl.method_rights[1], 1);
        assert_false(acl.method_seen[2]);
    }
    for (uint16_t oi = 0x4510; oi <= 0x4517; oi++) {
        assert_int_equal(_acl_rights_for(oi, &acl), 0);
        assert_false(acl.seen[0]);
        for (uint8_t attr = 1; attr <= 5; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (int attr = 6; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
        assert_true(acl.method_seen[1]);
        assert_int_equal(acl.method_rights[1], 1);
        assert_false(acl.method_seen[2]);
    }
}

static void test_class19_devmgr_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    /* 表178 的 A1..A10 与方法 1..6 全部只读广告；4307..4309 另定义 127..129，
     * 同样只读广告。4300 没有 OI 专属方法，127..129 不广告。 */
    static const uint16_t ois[] = {0x4300, 0x4307, 0x4308, 0x4309};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        assert_int_equal(_acl_rights_for(ois[i], &acl), 0);
        assert_false(acl.seen[0]);
        for (uint8_t attr = 1; attr <= 10; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (int attr = 11; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
        for (uint8_t method = 1; method <= 6; method++) {
            assert_true(acl.method_seen[method]);
            assert_int_equal(acl.method_rights[method], 1);
        }
        assert_false(acl.method_seen[7]);
        if (ois[i] == 0x4300) {
            for (int method = 127; method < ACL_MAX_RIGHTS; method++) {
                assert_false(acl.method_seen[method]);
            }
        } else {
            for (uint8_t method = 127; method <= 129; method++) {
                assert_true(acl.method_seen[method]);
                assert_int_equal(acl.method_rights[method], 1);
            }
            assert_false(acl.method_seen[130]);
        }
    }
}

static void test_class21_esam_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    /* 表184 的 A1..A15 与方法 1..12 全部只读广告。属性只读，SET 回 DAR 3。 */
    assert_int_equal(_acl_rights_for(0xF100, &acl), 0);
    assert_false(acl.seen[0]);
    for (uint8_t attr = 1; attr <= 15; attr++) {
        assert_true(acl.seen[attr]);
        assert_int_equal(acl.rights[attr], 1);
    }
    for (int attr = 16; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
    for (uint8_t method = 1; method <= 12; method++) {
        assert_true(acl.method_seen[method]);
        assert_int_equal(acl.method_rights[method], 1);
    }
    for (int method = 13; method < ACL_MAX_RIGHTS; method++) assert_false(acl.method_seen[method]);
}

static void test_class23_group_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    /* 表190 的 A1..A18、方法 1..5；属性与方法都只读。 */
    for (uint16_t oi = 0x2301; oi <= 0x2308; oi++) {
        assert_int_equal(_acl_rights_for(oi, &acl), 0);
        assert_false(acl.seen[0]);
        for (uint8_t attr = 1; attr <= 18; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (int attr = 19; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
        for (uint8_t method = 1; method <= 5; method++) {
            assert_true(acl.method_seen[method]);
            assert_int_equal(acl.method_rights[method], 1);
        }
        for (int method = 6; method < ACL_MAX_RIGHTS; method++) assert_false(acl.method_seen[method]);
    }
}

static void test_class18_file_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    /* 表175/表A.11：F000 广告 A1..A5，F001/F002 广告 A1..A4；方法 1..8 全部广告，
     * F001 另有 OI 方法 9/10。 */
    static const uint8_t max_attr[3] = {5, 4, 4};
    static const uint8_t max_method[3] = {8, 10, 8};
    static const uint16_t ois[3] = {0xF000, 0xF001, 0xF002};
    for (size_t i = 0; i < 3; i++) {
        assert_int_equal(_acl_rights_for(ois[i], &acl), 0);
        assert_false(acl.seen[0]);
        for (uint8_t attr = 1; attr <= max_attr[i]; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (int attr = max_attr[i] + 1; attr < ACL_MAX_RIGHTS; attr++) {
            assert_false(acl.seen[attr]);
        }
        for (uint8_t method = 1; method <= max_method[i]; method++) {
            assert_true(acl.method_seen[method]);
            assert_int_equal(acl.method_rights[method], 1);
        }
        for (int method = max_method[i] + 1; method < ACL_MAX_RIGHTS; method++) {
            assert_false(acl.method_seen[method]);
        }
    }
}

/* 类 22 的模型掩码就是向外广告的属性集：掩码加了属性却忘 ACL 行时，主站永远
 * 拿不到它（D4-08a 的 F201–F20C 缺陷）。其他接口类的 ACL 是刻意子集，
 * 掩码多于 ACL 是设计，不在本断言范围。 */
static void test_class22_object_masks_are_covered_by_acl(void **state) {
    (void)state;
    int missing = 0;
    size_t count = dcu_698_object_count();
    assert_true(count > 0);
    for (size_t i = 0; i < count; i++) {
        const dcu_698_object_t *obj = dcu_698_object_at(i);
        assert_non_null(obj);
        if (obj->class_id != IC_PERIPHERAL) continue;
        acl_rights_t acl;
        assert_int_equal(_acl_rights_for(obj->oi, &acl), 0);
        for (int attr = 1; attr < ACL_MAX_RIGHTS; attr++) {
            if (!dcu_mask_test(obj->attr_mask, (uint8_t)attr)) continue;
            if (!acl.seen[attr] || acl.rights[attr] == 0) {
                print_message("OI 0x%04X attr %d masked but not advertised in ACL\n", obj->oi,
                              attr);
                missing++;
            }
        }
    }
    assert_int_equal(missing, 0);
}
static void test_class13_control_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    assert_int_equal(_acl_rights_for(0x8001, &acl), 0);
    assert_true(acl.found);
    assert_false(acl.seen[1]);
    for (uint8_t attr = 2; attr <= 5; attr++) {
        assert_true(acl.seen[attr]);
        assert_int_equal(acl.rights[attr], 1);
    }
    for (int attr = 6; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
    for (int method = 0; method < ACL_MAX_RIGHTS; method++) assert_false(acl.method_seen[method]);

    static const uint16_t a2_ois[] = {0x8100, 0x8101, 0x8102, 0x8103};
    for (size_t i = 0; i < sizeof(a2_ois) / sizeof(a2_ois[0]); i++) {
        assert_int_equal(_acl_rights_for(a2_ois[i], &acl), 0);
        assert_true(acl.found);
        assert_false(acl.seen[1]);
        assert_true(acl.seen[2]);
        assert_int_equal(acl.rights[2], 1);
        for (int attr = 3; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
        for (int method = 0; method < ACL_MAX_RIGHTS; method++) assert_false(acl.method_seen[method]);
    }

    static const uint16_t empty_ois[] = {0x810E, 0x810F, 0x8110};
    for (size_t i = 0; i < sizeof(empty_ois) / sizeof(empty_ois[0]); i++) {
        assert_int_equal(_acl_rights_for(empty_ois[i], &acl), 0);
        assert_true(acl.found);
        for (int attr = 0; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
        for (int method = 0; method < ACL_MAX_RIGHTS; method++) assert_false(acl.method_seen[method]);
    }
}

static void test_class13_control_get_reaches_resolver_and_set_denied(void **state) {
    (void)state;
    for (uint8_t attr = 2; attr <= 5; attr++) {
        assert_int_equal(_get_dar_oi(0x8001, attr), DLT698_DAR_TEMPORARY_FAILURE);
    }
    static const uint16_t a2_ois[] = {0x8100, 0x8101, 0x8102, 0x8103};
    for (size_t i = 0; i < sizeof(a2_ois) / sizeof(a2_ois[0]); i++) {
        assert_int_equal(_get_dar_oi(a2_ois[i], 2), DLT698_DAR_TEMPORARY_FAILURE);
    }
    assert_int_equal(_get_dar_oi(0x810E, 2), DLT698_DAR_DENIED);

    const uint8_t null_val[] = {0x00};
    assert_int_equal(_set_dar_oi(0x8001, 2, null_val, sizeof(null_val)), DLT698_DAR_DENIED);
    assert_int_equal(_set_dar_oi(0x8100, 2, null_val, sizeof(null_val)), DLT698_DAR_DENIED);
    assert_int_equal(_set_dar_oi(0x8103, 2, null_val, sizeof(null_val)), DLT698_DAR_DENIED);
}

static void test_class7_event_acl_is_read_only(void **state) {
    (void)state;
    static const uint16_t ois[] = {0x3009, 0x300C, 0x3011, 0x3013};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        acl_rights_t acl;
        assert_int_equal(_acl_rights_for(ois[i], &acl), 0);
        assert_true(acl.found);
        assert_false(acl.seen[0]);
        for (uint8_t attr = 1; attr <= 10; attr++) {
            assert_true(acl.seen[attr]);
            assert_int_equal(acl.rights[attr], 1);
        }
        for (int attr = 11; attr < ACL_MAX_RIGHTS; attr++) assert_false(acl.seen[attr]);
        for (int method = 0; method < ACL_MAX_RIGHTS; method++) assert_false(acl.method_seen[method]);
    }
}

static void test_class24_300b_acl_is_read_only(void **state) {
    (void)state;
    acl_rights_t acl;
    assert_int_equal(_acl_rights_for(0x300B, &acl), 0);
    assert_true(acl.found);
    for (int attr = 0; attr < ACL_MAX_RIGHTS; attr++) {
        bool want = attr >= 5 && attr <= 9;
        assert_int_equal(acl.seen[attr], want);
        if (want) assert_int_equal(acl.rights[attr], 1);
    }
    for (int method = 0; method < ACL_MAX_RIGHTS; method++) assert_false(acl.method_seen[method]);
}

static void test_class7_event_get_dar5_and_set_denied(void **state) {
    (void)state;
    static const uint16_t ois[] = {0x3009, 0x300C, 0x3011, 0x3013};
    const uint8_t null_val[] = {0x00};
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        for (uint8_t attr = 1; attr <= 10; attr++) {
            assert_int_equal(_get_dar_oi(ois[i], attr), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
        assert_int_equal(_get_dar_oi(ois[i], 11), DLT698_DAR_DENIED);
        assert_int_equal(_set_dar_oi(ois[i], 2, null_val, sizeof(null_val)), DLT698_DAR_DENIED);
        assert_int_equal(_set_dar_oi(ois[i], 8, null_val, sizeof(null_val)), DLT698_DAR_DENIED);
    }
}

static void test_class12_pulse_get_dar5_and_set_denied(void **state) {
    (void)state;
    const uint8_t null_val[] = {0x00};
    for (uint16_t oi = 0x2402; oi <= 0x2408; oi++) {
        for (uint8_t attr = 1; attr <= 15; attr++) {
            assert_int_equal(_get_dar_oi(oi, attr), DLT698_DAR_OBJECT_CLASS_MISMATCH);
            assert_int_equal(_set_dar_oi(oi, attr, null_val, sizeof(null_val)), DLT698_DAR_DENIED);
        }
        for (uint8_t attr = 16; attr <= 20; attr++) {
            assert_int_equal(_get_dar_oi(oi, attr), DLT698_DAR_DENIED);
        }
        for (uint8_t method = 1; method <= 4; method++) {
            assert_int_equal(
                dcu_north_accept_action(NULL, ((uint32_t)oi << 16) | ((uint32_t)method << 8),
                                        null_val, sizeof(null_val)),
                EDGE_SLAVE_SRC_CLASS_MISMATCH);
        }
        assert_int_equal(
            dcu_north_accept_action(NULL, ((uint32_t)oi << 16) | (5u << 8), null_val,
                                    sizeof(null_val)),
            EDGE_SLAVE_SRC_UNDEFINED);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_object_list_shape_and_capacity),
        cmocka_unit_test(test_class22_acl_advertises_read_only_attrs),
        cmocka_unit_test(test_class14_15_16_acl_is_read_only),
        cmocka_unit_test(test_class17_acl_is_read_only),
        cmocka_unit_test(test_class11_acl_is_read_only),
        cmocka_unit_test(test_class12_pulse_acl_is_read_only),
        cmocka_unit_test_setup_teardown(test_class12_pulse_get_dar5_and_set_denied, setup_model,
                                        teardown_model),
        cmocka_unit_test(test_class25_26_comm_acl_is_read_only),
        cmocka_unit_test(test_class19_devmgr_acl_is_read_only),
        cmocka_unit_test(test_class21_esam_acl_is_read_only),
        cmocka_unit_test(test_class23_group_acl_is_read_only),
        cmocka_unit_test(test_class18_file_acl_is_read_only),
        cmocka_unit_test(test_class13_control_acl_is_read_only),
        cmocka_unit_test_setup_teardown(test_class13_control_get_reaches_resolver_and_set_denied,
                                        setup_model, teardown_model),
        cmocka_unit_test(test_class7_event_acl_is_read_only),
        cmocka_unit_test(test_class24_300b_acl_is_read_only),
        cmocka_unit_test_setup_teardown(test_class7_event_get_dar5_and_set_denied, setup_model,
                                        teardown_model),
        cmocka_unit_test(test_class22_object_masks_are_covered_by_acl),
        cmocka_unit_test_setup_teardown(test_event_acl_advertises_only_a11_a12_writable,
                                        setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_event_set_dar_by_attribute, setup_model,
                                        teardown_model),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
