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
#include "dcu_comm_info.h"
#include "dcu_inventory.h"
#include "dcu_north_adapter.h"
#include "dcu_poller.h"
#include "dcu_relay_map.h"
#include "dcu_router.h"
#include "dlt698_codec.h"
#include "edge_driver.h"
#include "edge_slave_plane.h"
#include "net_buf.h"

extern const edge_driver_t dlt698_driver;

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

static dcu_inventory_t g_inv;
static dcu_poller_t g_poller;

static int setup_model(void **state) {
    (void)state;
    dcu_relay_map_bind(NULL);
    dcu_comm_info_reset();
    assert_int_equal(dcu_inventory_init(&g_inv), 0);
    assert_int_equal(dcu_poller_init(&g_poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(dcu_698_object_model_init(&g_inv, &g_poller, sink, NULL, NULL, NULL, NULL, bus),
                     0);
    return 0;
}

static int teardown_model(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    dcu_poller_destroy(&g_poller);
    dcu_inventory_destroy(&g_inv);
    dcu_relay_map_bind(NULL);
    dcu_comm_info_reset();
    return 0;
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

static void _exchange(const uint8_t *apdu, size_t apdu_len, uint8_t *backing, size_t backing_cap,
                      uint8_t *svc_out, uint8_t **payload_out, size_t *payload_len_out,
                      size_t *frame_len_out) {
    uint8_t frame[256];
    assert_true(apdu_len + 16 <= sizeof(frame));
    size_t flen = _build_frame(frame, apdu, apdu_len);

    edge_slave_source_ops_t src = {0};
    src.object_list = _object_list;
    src.resolve = dcu_north_resolve_scalar;
    src.resolve_axdr = dcu_north_resolve_axdr;
    src.accept_axdr = dcu_north_accept_axdr;
    src.accept_action = dcu_north_accept_action;

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));

    net_buf_t out;
    memset(&out, 0, sizeof(out));
    net_buf_simple_init_with_data(&out.b, backing, backing_cap);
    size_t consumed = 0;
    assert_int_equal(
        dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, NULL, &out, &consumed),
        0);
    if (frame_len_out) *frame_len_out = out.b.len;

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, out.b.data, out.b.len);
    nb.len = out.b.len;
    dlt698_addr_t sa;
    uint8_t choice = 0, piid = 0;
    assert_int_equal(
        dlt698_decode_frame_ex(&nb, &sa, svc_out, &choice, &piid, payload_out, payload_len_out),
        0);
}

static uint8_t _get_dar(uint32_t oad) {
    uint8_t apdu[8] = {
        DLT698_SVC_GET_REQ,
        0x01,
        0x01,
        (uint8_t)(oad >> 24),
        (uint8_t)(oad >> 16),
        (uint8_t)(oad >> 8),
        (uint8_t)oad,
        0x00,
    };
    uint8_t backing[512];
    uint8_t svc = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    _exchange(apdu, sizeof(apdu), backing, sizeof(backing), &svc, &payload, &plen, NULL);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_true(plen >= 6);
    assert_int_equal(payload[4], 0x00); /* GetResult ::= DAR */
    return payload[5];
}

static uint8_t _set_dar(uint32_t oad, const uint8_t *val, size_t val_len) {
    uint8_t apdu[64];
    size_t n = 0;
    apdu[n++] = DLT698_SVC_SET_REQ;
    apdu[n++] = 0x01;
    apdu[n++] = 0x02;
    apdu[n++] = (uint8_t)(oad >> 24);
    apdu[n++] = (uint8_t)(oad >> 16);
    apdu[n++] = (uint8_t)(oad >> 8);
    apdu[n++] = (uint8_t)oad;
    memcpy(apdu + n, val, val_len);
    n += val_len;
    apdu[n++] = 0x00;

    uint8_t backing[512];
    uint8_t svc = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    _exchange(apdu, n, backing, sizeof(backing), &svc, &payload, &plen, NULL);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_true(plen >= 5);
    return payload[4];
}

static uint8_t _action_dar(uint32_t omd, const uint8_t *param, size_t param_len) {
    uint8_t apdu[64];
    size_t n = 0;
    apdu[n++] = DLT698_SVC_ACTION_REQ;
    apdu[n++] = 0x01;
    apdu[n++] = 0x03;
    apdu[n++] = (uint8_t)(omd >> 24);
    apdu[n++] = (uint8_t)(omd >> 16);
    apdu[n++] = (uint8_t)(omd >> 8);
    apdu[n++] = (uint8_t)omd;
    if (param && param_len > 0) {
        memcpy(apdu + n, param, param_len);
        n += param_len;
    } else {
        apdu[n++] = DLT698_TAG_NULL;
    }
    apdu[n++] = 0x00;

    uint8_t backing[512];
    uint8_t svc = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    _exchange(apdu, n, backing, sizeof(backing), &svc, &payload, &plen, NULL);
    assert_int_equal(svc, DLT698_SVC_ACTION_RES);
    assert_true(plen >= 5);
    return payload[4];
}

static void test_p4_class13_control(void **state) {
    (void)state;
    static const uint8_t k_8000_attrs[] = {2, 4, 5};
    for (size_t i = 0; i < sizeof(k_8000_attrs); i++) {
        assert_int_equal(_get_dar(MK_OAD(0x8000, k_8000_attrs[i])), DLT698_DAR_TEMPORARY_FAILURE);
    }
    for (uint8_t attr = 2; attr <= 5; attr++) {
        assert_int_equal(_get_dar(MK_OAD(0x8001, attr)), DLT698_DAR_TEMPORARY_FAILURE);
    }
    for (uint16_t oi = 0x8100; oi <= 0x8103; oi++) {
        assert_int_equal(_get_dar(MK_OAD(oi, 2)), DLT698_DAR_TEMPORARY_FAILURE);
    }

    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 1};
    assert_int_equal(_set_dar(MK_OAD(0x8000, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
    assert_int_equal(_set_dar(MK_OAD(0x8001, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
    assert_int_equal(_set_dar(MK_OAD(0x8100, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x8000, 2), u8_val, sizeof(u8_val)),
                     EDGE_SLAVE_SRC_DENIED);

    /* 8000 ACTION 129/130/131 with empty relay map -> DAR 2 */
    const uint8_t p129[] = {
        DLT698_TAG_ARRAY, 1, DLT698_TAG_STRUCTURE, 4,
        DLT698_TAG_OAD, 0xF2, 0x05, 0x02, 0x01,
        DLT698_TAG_UNSIGNED, 5,
        DLT698_TAG_LONG_UNSIGNED, 0x00, 0x0A,
        DLT698_TAG_BOOLEAN, 0x00,
    };
    const uint8_t p130[] = {
        DLT698_TAG_ARRAY, 1, DLT698_TAG_STRUCTURE, 2,
        DLT698_TAG_OAD, 0xF2, 0x05, 0x02, 0x01,
        DLT698_TAG_ENUM, 0x01,
    };
    const uint8_t p131[] = {
        DLT698_TAG_ARRAY, 1, DLT698_TAG_STRUCTURE, 3,
        DLT698_TAG_OAD, 0xF2, 0x05, 0x02, 0x01,
        DLT698_TAG_ENUM, 0x00,
        DLT698_TAG_VISIBLE_STRING, 0x01, '0',
    };
    assert_int_equal(_action_dar(MK_OMD(0x8000, 129), p129, sizeof(p129)),
                     DLT698_DAR_TEMPORARY_FAILURE);
    assert_int_equal(_action_dar(MK_OMD(0x8000, 130), p130, sizeof(p130)),
                     DLT698_DAR_TEMPORARY_FAILURE);
    assert_int_equal(_action_dar(MK_OMD(0x8000, 131), p131, sizeof(p131)),
                     DLT698_DAR_TEMPORARY_FAILURE);

    /* Undefined attr -> DAR 4 */
    uint8_t tmp[64];
    size_t tlen = 0;
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x8000, 99), tmp, sizeof(tmp), &tlen),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x8001, 99), tmp, sizeof(tmp), &tlen),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x8103, 99), tmp, sizeof(tmp), &tlen),
                     EDGE_SLAVE_SRC_UNDEFINED);
}

static void test_p4_class18_file_transfer(void **state) {
    (void)state;
    static const uint16_t ois[] = {0xF000, 0xF001, 0xF002};
    static const uint8_t max_attr[] = {5, 4, 4};
    for (size_t i = 0; i < 3; i++) {
        for (uint8_t a = 1; a <= max_attr[i]; a++) {
            assert_int_equal(_get_dar(MK_OAD(ois[i], a)), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
    }

    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 0};
    for (size_t i = 0; i < 3; i++) {
        assert_int_equal(_set_dar(MK_OAD(ois[i], 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
    }

    for (uint8_t m = 1; m <= 10; m++) {
        assert_int_equal(_action_dar(MK_OMD(0xF001, m), NULL, 0), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    }

    assert_int_equal(dcu_698_call(MK_OMD(0xF001, 11), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(0xF000, 9), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0xF002, 9), NULL, 0),
                     EDGE_SLAVE_SRC_UNDEFINED);
}

static void test_p4_class19_device_mgmt(void **state) {
    (void)state;
    static const uint16_t ois[] = {0x4300, 0x4307, 0x4308, 0x4309};
    for (size_t i = 0; i < 4; i++) {
        for (uint8_t a = 1; a <= 10; a++) {
            assert_int_equal(_get_dar(MK_OAD(ois[i], a)), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
    }

    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 0};
    for (size_t i = 0; i < 4; i++) {
        assert_int_equal(_set_dar(MK_OAD(ois[i], 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
    }

    for (size_t i = 0; i < 4; i++) {
        for (uint8_t m = 1; m <= 6; m++) {
            assert_int_equal(_action_dar(MK_OMD(ois[i], m), NULL, 0),
                             DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
        if (ois[i] != 0x4300) {
            for (uint8_t m = 127; m <= 129; m++) {
                assert_int_equal(_action_dar(MK_OMD(ois[i], m), NULL, 0),
                                 DLT698_DAR_OBJECT_CLASS_MISMATCH);
            }
        }
    }

    assert_int_equal(dcu_698_call(MK_OMD(0x4300, 127), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(0x4307, 130), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(0x4309, 7), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
}

static void test_p4_class20_appconn_and_acl(void **state) {
    (void)state;
    uint8_t list_buf[EDGE_SLAVE_AXDR_MAX];
    size_t list_len = 0;
    assert_int_equal(dcu_698_appconn_object_list(list_buf, sizeof(list_buf), &list_len), 0);
    assert_true(list_len > 3);
    assert_int_equal(list_buf[0], 0x01);
    assert_int_equal(list_buf[1], 0x81);
    assert_int_equal(list_buf[2], 0x8A);

    /* Full 698 GET 4400 A2 frame -> 7351 B response */
    const uint8_t get_4400[] = {DLT698_SVC_GET_REQ, 0x01, 0x01, 0x44, 0x00, 0x02, 0x00, 0x00};
    static uint8_t backing[8192];
    uint8_t svc = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    size_t frame_len = 0;
    _exchange(get_4400, sizeof(get_4400), backing, sizeof(backing), &svc, &payload, &plen,
              &frame_len);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(frame_len, 7351);
    assert_true(plen > 8);
    assert_int_equal(payload[4], 0x01); /* GetResult ::= Data */
    assert_int_equal(payload[5], 0x01);
    assert_int_equal(payload[6], 0x81);
    assert_int_equal(payload[7], 0x8A);

    /* ACL enforcement: authorized GET passes ACL vs unauthorized SET -> DAR 3 */
    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 0};
    assert_int_equal(_set_dar(MK_OAD(0x4400, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
    assert_int_equal(_set_dar(MK_OAD(0x4300, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);

    /* Unknown OI: DAR 3 at ACL layer, DAR 6 at resolver */
    assert_int_equal(_get_dar(MK_OAD(0x1234, 2)), DLT698_DAR_DENIED);
    uint8_t tmp[64];
    size_t tlen = 0;
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x1234, 2), tmp, sizeof(tmp), &tlen),
                     EDGE_SLAVE_SRC_NOT_FOUND);
}

static void test_p4_class21_esam(void **state) {
    (void)state;
    for (uint8_t a = 1; a <= 15; a++) {
        assert_int_equal(_get_dar(MK_OAD(0xF100, a)), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    }

    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 0};
    assert_int_equal(_set_dar(MK_OAD(0xF100, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);

    for (uint8_t m = 1; m <= 12; m++) {
        assert_int_equal(_action_dar(MK_OMD(0xF100, m), NULL, 0), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    }

    assert_int_equal(dcu_698_call(MK_OMD(0xF100, 13), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_call(MK_OMD(0xF100, 99), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
}

static void test_p4_class22_peripheral(void **state) {
    (void)state;
    /* F200 existing product behavior: A2 -> DAR 2, A3/A4 -> DAR 5 */
    assert_int_equal(_get_dar(MK_OAD(0xF200, 2)), DLT698_DAR_TEMPORARY_FAILURE);
    assert_int_equal(_get_dar(MK_OAD(0xF200, 3)), DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_int_equal(_get_dar(MK_OAD(0xF200, 4)), DLT698_DAR_OBJECT_CLASS_MISMATCH);

    /* F201..F20C honest DAR 2/5 */
    for (uint16_t oi = 0xF201; oi <= 0xF20C; oi++) {
        assert_int_equal(_get_dar(MK_OAD(oi, 2)), DLT698_DAR_TEMPORARY_FAILURE);
        uint8_t tmp[64];
        size_t tlen = 0;
        assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(oi, 3), tmp, sizeof(tmp), &tlen),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
        assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(oi, 99), tmp, sizeof(tmp), &tlen),
                         EDGE_SLAVE_SRC_UNDEFINED);
    }
    assert_int_equal(_get_dar(MK_OAD(0xF203, 4)), DLT698_DAR_TEMPORARY_FAILURE);
    assert_int_equal(_get_dar(MK_OAD(0xF209, 5)), DLT698_DAR_TEMPORARY_FAILURE);
    assert_int_equal(_get_dar(MK_OAD(0xF20C, 5)), DLT698_DAR_TEMPORARY_FAILURE);

    /* SET on read-only F201 -> DAR 3 */
    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 0};
    assert_int_equal(_set_dar(MK_OAD(0xF201, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0xF201, 2), u8_val, sizeof(u8_val)),
                     EDGE_SLAVE_SRC_DENIED);
}

static void test_p4_class23_group(void **state) {
    (void)state;
    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 0};
    uint8_t tmp[64];
    size_t tlen = 0;

    for (uint16_t oi = 0x2301; oi <= 0x2308; oi++) {
        for (uint8_t a = 1; a <= 18; a++) {
            assert_int_equal(_get_dar(MK_OAD(oi, a)), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
        assert_int_equal(_set_dar(MK_OAD(oi, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
        for (uint8_t m = 1; m <= 5; m++) {
            assert_int_equal(_action_dar(MK_OMD(oi, m), NULL, 0), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
        assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(oi, 19), tmp, sizeof(tmp), &tlen),
                         EDGE_SLAVE_SRC_UNDEFINED);
        assert_int_equal(dcu_698_call(MK_OMD(oi, 6), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    }
}

static void test_p4_class25_wireless_public(void **state) {
    (void)state;
    static const uint16_t ois[] = {0x4500, 0x4501};
    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 0};
    uint8_t tmp[64];
    size_t tlen = 0;

    for (size_t i = 0; i < 2; i++) {
        for (uint8_t a = 1; a <= 11; a++) {
            assert_int_equal(_get_dar(MK_OAD(ois[i], a)), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
        assert_int_equal(_set_dar(MK_OAD(ois[i], 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
        assert_int_equal(_action_dar(MK_OMD(ois[i], 1), NULL, 0), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(ois[i], 12), tmp, sizeof(tmp), &tlen),
                         EDGE_SLAVE_SRC_UNDEFINED);
        assert_int_equal(dcu_698_call(MK_OMD(ois[i], 2), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    }
}

static void test_p4_class26_ethernet(void **state) {
    (void)state;
    dcu_comm_info_t info = {
        .mac_present = true,
        .mac = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55},
    };
    dcu_comm_info_install(&info);

    /* 4510 A5 MAC valid octet-string on axdr path, DAR 5 on scalar path */
    const uint8_t get_4510_a5[] = {DLT698_SVC_GET_REQ, 0x01, 0x01, 0x45, 0x10, 0x05, 0x00, 0x00};
    uint8_t backing[512];
    uint8_t svc = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    _exchange(get_4510_a5, sizeof(get_4510_a5), backing, sizeof(backing), &svc, &payload, &plen,
              NULL);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_true(plen >= 13);
    assert_int_equal(payload[4], 0x01); /* GetResult ::= Data */
    assert_int_equal(payload[5], DLT698_TAG_OCTET_STRING);
    assert_int_equal(payload[6], 6);
    assert_memory_equal(payload + 7, info.mac, 6);

    edge_point_value_t pv;
    assert_int_equal(dcu_north_resolve_scalar(NULL, MK_OAD(0x4510, 5), NULL, 0, &pv),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);

    dcu_comm_info_reset();
    assert_int_equal(_get_dar(MK_OAD(0x4510, 5)), DLT698_DAR_OBJECT_CLASS_MISMATCH);

    const uint8_t u8_val[] = {DLT698_TAG_UNSIGNED, 0};
    uint8_t tmp[64];
    size_t tlen = 0;
    for (uint16_t oi = 0x4510; oi <= 0x4517; oi++) {
        for (uint8_t a = 1; a <= 4; a++) {
            assert_int_equal(_get_dar(MK_OAD(oi, a)), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
        if (oi != 0x4510) {
            assert_int_equal(_get_dar(MK_OAD(oi, 5)), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        }
        assert_int_equal(_set_dar(MK_OAD(oi, 2), u8_val, sizeof(u8_val)), DLT698_DAR_DENIED);
        assert_int_equal(_action_dar(MK_OMD(oi, 1), NULL, 0), DLT698_DAR_OBJECT_CLASS_MISMATCH);
        assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(oi, 6), tmp, sizeof(tmp), &tlen),
                         EDGE_SLAVE_SRC_UNDEFINED);
        assert_int_equal(dcu_698_call(MK_OMD(oi, 2), NULL, NULL), EDGE_SLAVE_SRC_UNDEFINED);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_p4_class13_control, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_p4_class18_file_transfer, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_p4_class19_device_mgmt, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_p4_class20_appconn_and_acl, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_p4_class21_esam, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_p4_class22_peripheral, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_p4_class23_group, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_p4_class25_wireless_public, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_p4_class26_ethernet, setup_model, teardown_model),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
