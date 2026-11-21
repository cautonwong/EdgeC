#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "datahub.h"
#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_var.h"
#include "dcu_inventory.h"
#include "dcu_north_adapter.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dlt698_codec.h"
#include "edge_driver.h"
#include "edge_slave_plane.h"
#include "net_buf.h"

extern const edge_driver_t dlt698_driver;

#define SA_LEN 1
#define TS_NS (1234567890ULL * 1000000ULL)
#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))

static int mock_shadow(void *ctx, uint32_t tag_id, uint32_t device_id, void *out_msg) {
    (void)ctx;
    (void)device_id;
    pipe_msg_t *msg = (pipe_msg_t *)out_msg;
    memset(msg, 0, sizeof(*msg));

    double value;
    switch (tag_id) {
        case OAD_ACTIVE_ENERGY:
            value = 12345.0;
            break;
        case OAD_REACTIVE_ENERGY:
            value = 6789.0;
            break;
        case OAD_VOLTAGE:
            value = 220.5;
            break;
        case OAD_CURRENT:
            value = 4.95;
            break;
        case OAD_ACTIVE_POWER_TOTAL:
            value = 1234.5;
            break;
        case OAD_POWER_FACTOR_TOTAL:
            value = 0.95;
            break;
        case OAD_GRID_FREQUENCY:
            value = 50.01;
            break;
        default:
            return -1;
    }
    msg->tag_id = tag_id;
    msg->ts = TS_NS;
    msg->quality = 0;
    msg->payload.val.f = value;
    return 0;
}

static void init_product_model(void) {
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {.msg_size = sizeof(pipe_msg_t), .read_shadow = mock_shadow};
    assert_int_equal(dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, NULL, NULL, bus),
                     0);
}

static void assert_axdr(uint32_t oad, const uint8_t *want, size_t want_len) {
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(dcu_north_resolve_axdr(NULL, oad, buf, sizeof(buf), &len), 0);
    assert_int_equal(len, want_len);
    assert_memory_equal(buf, want, len);
}

static void test_product_variable_axdr_types(void **state) {
    (void)state;
    init_product_model();

    const float v = 220.5f;
    uint32_t bits = 0;
    memcpy(&bits, &v, sizeof(bits));
    const uint8_t want_voltage[] = {DCU_698_TAG_FLOAT32, (uint8_t)(bits >> 24),
                                    (uint8_t)(bits >> 16), (uint8_t)(bits >> 8), (uint8_t)bits};
    assert_axdr(OAD_VOLTAGE, want_voltage, sizeof(want_voltage));

    /* 0010/0020：属性 2 是 double-long-unsigned，属性 4 是 long64-unsigned。 */
    const uint8_t want_energy2[] = {DCU_698_TAG_DOUBLE_LONG_UNSIGNED, 0x00, 0x00, 0x30, 0x39};
    assert_axdr(MK_OAD(0x0010, 2), want_energy2, sizeof(want_energy2));
    const uint8_t want_energy4[] = {
        DCU_698_TAG_LONG64_UNSIGNED, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x39};
    assert_axdr(MK_OAD(0x0010, 4), want_energy4, sizeof(want_energy4));
    const uint8_t want_reactive2[] = {DCU_698_TAG_DOUBLE_LONG_UNSIGNED, 0x00, 0x00, 0x1A, 0x85};
    assert_axdr(MK_OAD(0x0020, 2), want_reactive2, sizeof(want_reactive2));

    const uint8_t want_freq[] = {DCU_698_TAG_LONG_UNSIGNED, 0x00, 0x32};
    assert_axdr(OAD_GRID_FREQUENCY, want_freq, sizeof(want_freq));

    /* 6000 attr 2 档案计数在对象层单独编码为 long-unsigned。 */
    const uint8_t want_archive[] = {DCU_698_TAG_LONG_UNSIGNED, 0x00, 0x00};
    assert_axdr(MK_OAD(OI_METER_ARCHIVE, 2), want_archive, sizeof(want_archive));

    dcu_698_object_model_reset();
}

static void test_product_axdr_rejects_small_buffer_and_complex(void **state) {
    (void)state;
    init_product_model();

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;

    /* 电压需要 5 字节；cap=4 必须失败，不截断 -> 类型/宽度不匹配。 */
    assert_int_equal(dcu_north_resolve_axdr(NULL, OAD_VOLTAGE, buf, 4, &len),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    assert_int_equal(dcu_north_resolve_axdr(NULL, OAD_DAILY_FREEZE, buf, sizeof(buf), &len), 0);
    assert_int_equal(buf[0], DCU_698_TAG_LONG_UNSIGNED);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x6000, 3), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    dcu_698_object_model_reset();
}

static void test_product_accept_set_paths(void **state) {
    (void)state;
    init_product_model();

    const uint8_t baud[] = {0x06, 0x00, 0x00, 0x09, 0x60}; /* 2400, double-long-unsigned */
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(OI_CHANNEL_CONFIG, 2), baud, sizeof(baud)),
                     0);
    uint32_t got = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_CHANNEL_CONFIG, 2), &got), 0);
    assert_int_equal(got, 2400);

    const uint8_t wrong[] = {0x0A, 0x01, 0x41}; /* visible-string，不是 baudrate */
    assert_int_equal(
        dcu_north_accept_axdr(NULL, MK_OAD(OI_CHANNEL_CONFIG, 2), wrong, sizeof(wrong)),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    const uint8_t like_float[] = {0x17, 0x43, 0x67, 0xB3, 0x33};
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x2000, 2), like_float, sizeof(like_float)),
                     EDGE_SLAVE_SRC_DENIED);

    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(OI_CHANNEL_CONFIG, 4), baud, sizeof(baud)),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);

    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x9999, 2), baud, sizeof(baud)),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    dcu_698_object_model_reset();
}

static const uint8_t k_structure[] = {0x01, 0x02, DCU_698_TAG_LONG_UNSIGNED, 0x00, 0x01, 0x12,
                                      0x00, 0x02}; /* array(2) long-unsigned */

static int scalar_resolve(void *ctx, uint32_t oad, const void *desc, size_t desc_len,
                          edge_point_value_t *out) {
    (void)ctx;
    (void)desc;
    (void)desc_len;
    if (!out || oad != OAD_VOLTAGE) return -1;
    memset(out, 0, sizeof(*out));
    out->value = 220.5;
    out->raw = 220;
    out->width = 4;
    out->type = 0x17;
    return 0;
}

static const edge_slave_source_ops_t k_scalar_src = {.resolve = scalar_resolve};

static int structure_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    (void)oad;
    if (!buf || !len || cap < sizeof(k_structure)) return -1;
    memcpy(buf, k_structure, sizeof(k_structure));
    *len = sizeof(k_structure);
    return 0;
}

static const edge_slave_source_ops_t k_structure_src = {.resolve_axdr = structure_resolve};

static int too_big_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    (void)oad;
    (void)buf;
    (void)len;
    return (cap < 4096) ? -1 : 0;
}

static const edge_slave_source_ops_t k_too_big_src = {.resolve_axdr = too_big_resolve};

static int list_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    static const uint8_t voltage[] = {DCU_698_TAG_FLOAT32, 0x43, 0x67, 0xB3, 0x33};
    static const uint8_t count[] = {DCU_698_TAG_LONG_UNSIGNED, 0x00, 0x28};
    const uint8_t *data = NULL;
    size_t n = 0;
    if (oad == OAD_VOLTAGE) {
        data = voltage;
        n = sizeof(voltage);
    } else if (oad == MK_OAD(OI_METER_ARCHIVE, 2)) {
        data = count;
        n = sizeof(count);
    } else {
        return -1;
    }
    if (!buf || !len || cap < n) return -1;
    memcpy(buf, data, n);
    *len = n;
    return 0;
}

static const edge_slave_source_ops_t k_list_src = {.resolve_axdr = list_resolve};

/* 68 | L(2,LE) | C | SA描述 | SA | CA | HCS(2) | APDU | FCS(2) | 16 */
static size_t build_request(uint8_t *frame, const uint8_t *apdu, size_t apdu_len) {
    size_t i = 0;
    frame[i++] = DLT698_HEAD;
    size_t len_pos = i;
    i += 2;
    frame[i++] = 0x43;
    frame[i++] = (uint8_t)((0u << 6) | (SA_LEN - 1));
    frame[i++] = 0x01;
    frame[i++] = 0x10;
    size_t hcs_pos = i;
    i += 2;
    memcpy(frame + i, apdu, apdu_len);
    i += apdu_len;

    size_t total = i + 3;
    uint16_t l_field = (uint16_t)((total - 2) & 0x3FFF);
    frame[len_pos] = (uint8_t)(l_field & 0xFF);
    frame[len_pos + 1] = (uint8_t)(l_field >> 8);

    uint16_t hcs = dlt698_calculate_fcs(frame + 1, hcs_pos - 1);
    frame[hcs_pos] = (uint8_t)(hcs & 0xFF);
    frame[hcs_pos + 1] = (uint8_t)(hcs >> 8);

    uint16_t fcs = dlt698_calculate_fcs(frame + 1, i - 1);
    frame[i++] = (uint8_t)(fcs & 0xFF);
    frame[i++] = (uint8_t)(fcs >> 8);
    frame[i++] = DLT698_END;
    return i;
}

static size_t build_get_normal(uint8_t *frame, uint32_t oad) {
    uint8_t apdu[9] = {DLT698_SVC_GET_REQ, DLT698_CHOICE_GET_NORMAL, 0x01};
    apdu[3] = (uint8_t)(oad >> 24);
    apdu[4] = (uint8_t)(oad >> 16);
    apdu[5] = (uint8_t)(oad >> 8);
    apdu[6] = (uint8_t)oad;
    apdu[7] = DLT698_NO_TIME_TAG;
    return build_request(frame, apdu, 8);
}

static size_t build_get_list(uint8_t *frame, const uint32_t *oads, size_t n) {
    uint8_t apdu[64] = {DLT698_SVC_GET_REQ, DLT698_CHOICE_GET_NORMAL_LIST, 0x01};
    size_t i = 3;
    apdu[i++] = (uint8_t)n;
    for (size_t k = 0; k < n; k++) {
        apdu[i++] = (uint8_t)(oads[k] >> 24);
        apdu[i++] = (uint8_t)(oads[k] >> 16);
        apdu[i++] = (uint8_t)(oads[k] >> 8);
        apdu[i++] = (uint8_t)oads[k];
    }
    apdu[i++] = DLT698_NO_TIME_TAG;
    return build_request(frame, apdu, i);
}

static bool contains(const uint8_t *hay, size_t hay_len, const uint8_t *needle, size_t needle_len) {
    for (size_t i = 0; i + needle_len <= hay_len; i++) {
        if (memcmp(hay + i, needle, needle_len) == 0) return true;
    }
    return false;
}

static void exchange_and_check(const uint8_t *frame, size_t frame_len,
                               const edge_slave_source_ops_t *src, const uint8_t *want,
                               size_t want_len) {
    net_buf_pool_t *pool = net_buf_pool_new(8, 4096, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc =
        dlt698_driver.req_slave_exchange(priv, NULL, frame, frame_len, src, NULL, out, &consumed);
    assert_int_equal(rc, 0);
    assert_true(consumed > 0);
    assert_true(contains(out->b.data, out->b.len, want, want_len));

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static void test_driver_scalar_fallback_still_works(void **state) {
    (void)state;
    uint8_t frame[128];
    size_t frame_len = build_get_normal(frame, OAD_VOLTAGE);

    const float v = 220.5f;
    uint32_t bits = 0;
    memcpy(&bits, &v, sizeof(bits));
    const uint8_t want[] = {0x17, (uint8_t)(bits >> 24), (uint8_t)(bits >> 16),
                            (uint8_t)(bits >> 8), (uint8_t)bits};
    exchange_and_check(frame, frame_len, &k_scalar_src, want, sizeof(want));
}

static void test_driver_carries_structure_axdr(void **state) {
    (void)state;
    uint8_t frame[128];
    size_t frame_len = build_get_normal(frame, MK_OAD(0x0010, 2));
    exchange_and_check(frame, frame_len, &k_structure_src, k_structure, sizeof(k_structure));
}

static void test_driver_rejects_axdr_buffer_shortage(void **state) {
    (void)state;
    uint8_t frame[128];
    size_t frame_len = build_get_normal(frame, OAD_VOLTAGE);

    net_buf_pool_t *pool = net_buf_pool_new(8, 4096, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc = dlt698_driver.req_slave_exchange(priv, NULL, frame, frame_len, &k_too_big_src, NULL,
                                              out, &consumed);
    assert_int_equal(rc, 0);
    /* 缓冲不足现在是明确的 DAR 应答（-1 按 NOT_FOUND 处理），不是静默超时。 */
    assert_int_equal(out->b.data[16], 0x00);
    assert_int_equal(out->b.data[17], DLT698_DAR_OBJECT_NOT_FOUND);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static void test_driver_normal_list_uses_axdr_per_item(void **state) {
    (void)state;
    const uint32_t oads[] = {OAD_VOLTAGE, MK_OAD(OI_METER_ARCHIVE, 2)};
    uint8_t frame[128];
    size_t frame_len = build_get_list(frame, oads, 2);

    net_buf_pool_t *pool = net_buf_pool_new(8, 4096, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc = dlt698_driver.req_slave_exchange(priv, NULL, frame, frame_len, &k_list_src, NULL, out,
                                              &consumed);
    assert_int_equal(rc, 0);

    const uint8_t voltage[] = {0x17, 0x43, 0x67, 0xB3, 0x33};
    const uint8_t count[] = {0x12, 0x00, 0x28};
    assert_true(contains(out->b.data, out->b.len, voltage, sizeof(voltage)));
    assert_true(contains(out->b.data, out->b.len, count, sizeof(count)));

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static int big_list_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    const size_t n = EDGE_SLAVE_AXDR_MAX - 8;
    if (!buf || !len || cap < n) return -1;
    uint8_t idx = (uint8_t)(oad & 0xFF);
    memset(buf, 0, n);
    buf[0] = 0xA0;
    buf[1] = idx;
    buf[2] = 0xB0;
    buf[3] = idx;
    buf[n - 2] = 0xC0;
    buf[n - 1] = idx;
    *len = n;
    return 0;
}

static const edge_slave_source_ops_t k_big_list_src = {.resolve_axdr = big_list_resolve};

static void test_driver_normal_list_reuses_one_buffer(void **state) {
    (void)state;
    uint32_t oads[8];
    for (size_t i = 0; i < 8; i++) oads[i] = 0x50000000U | (uint32_t)i;

    uint8_t frame[128];
    size_t frame_len = build_get_list(frame, oads, 8);

    net_buf_pool_t *pool = net_buf_pool_new(2, 32768, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc = dlt698_driver.req_slave_exchange(priv, NULL, frame, frame_len, &k_big_list_src, NULL,
                                              out, &consumed);
    assert_int_equal(rc, 0);

    /* 帧头 9 字节后：85 | 02 | PIID | n。 */
    assert_true(out->b.len > 16000);
    assert_int_equal(out->b.data[9], DLT698_SVC_GET_RES);
    assert_int_equal(out->b.data[10], DLT698_CHOICE_GET_NORMAL_LIST);
    assert_int_equal(out->b.data[12], 8);

    /* 每项的首尾标记都必须在：证明 8 份 2040 字节都被完整写入、没有互相覆盖。 */
    for (uint8_t k = 0; k < 8; k++) {
        const uint8_t head[] = {0xA0, k, 0xB0, k};
        const uint8_t tail[] = {0xC0, k};
        assert_true(contains(out->b.data, out->b.len, head, sizeof(head)));
        assert_true(contains(out->b.data, out->b.len, tail, sizeof(tail)));
    }

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static int g_src_rc;

static int dar_scalar_resolve(void *ctx, uint32_t oad, const void *desc, size_t desc_len,
                              edge_point_value_t *out) {
    (void)ctx;
    (void)oad;
    (void)desc;
    (void)desc_len;
    (void)out;
    return g_src_rc;
}

static const edge_slave_source_ops_t k_dar_src = {.resolve = dar_scalar_resolve};

static int bad_type_resolve(void *ctx, uint32_t oad, const void *desc, size_t desc_len,
                            edge_point_value_t *out) {
    (void)ctx;
    (void)desc;
    (void)desc_len;
    if (!out || oad != OAD_VOLTAGE) return EDGE_SLAVE_SRC_NOT_FOUND;
    memset(out, 0, sizeof(*out));
    out->value = 220.5;
    out->width = 4;
    out->type = 0x99; /* 没有对应编码器 */
    return 0;
}

static const edge_slave_source_ops_t k_bad_type_src = {.resolve = bad_type_resolve};

static net_buf_t *run_get_normal(const uint8_t *frame, size_t frame_len,
                                 const edge_slave_source_ops_t *src, net_buf_pool_t **pool_out) {
    net_buf_pool_t *pool = net_buf_pool_new(8, 4096, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc =
        dlt698_driver.req_slave_exchange(priv, NULL, frame, frame_len, src, NULL, out, &consumed);
    assert_int_equal(rc, 0);
    *pool_out = pool;
    return out;
}

static void assert_get_normal_dar(int src_rc, uint8_t want_dar) {
    g_src_rc = src_rc;
    uint8_t frame[128];
    size_t frame_len = build_get_normal(frame, OAD_VOLTAGE);

    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_get_normal(frame, frame_len, &k_dar_src, &pool);
    /* APDU: 85 | 01 | PIID | OAD(4) | 00 | DAR | 00 | 00，APDU 从偏移 9 起。 */
    assert_int_equal(out->b.data[9], DLT698_SVC_GET_RES);
    assert_int_equal(out->b.data[10], DLT698_CHOICE_GET_NORMAL);
    assert_int_equal(out->b.data[16], 0x00);
    assert_int_equal(out->b.data[17], want_dar);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static void test_get_normal_dar_mapping(void **state) {
    (void)state;
    assert_get_normal_dar(EDGE_SLAVE_SRC_NOT_FOUND, DLT698_DAR_OBJECT_NOT_FOUND);
    assert_get_normal_dar(EDGE_SLAVE_SRC_CLASS_MISMATCH, DLT698_DAR_OBJECT_CLASS_MISMATCH);
    assert_get_normal_dar(EDGE_SLAVE_SRC_TYPE_MISMATCH, DLT698_DAR_TYPE_MISMATCH);
    assert_get_normal_dar(EDGE_SLAVE_SRC_UNDEFINED, DLT698_DAR_OBJECT_UNDEFINED);
    assert_get_normal_dar(EDGE_SLAVE_SRC_TEMPORARY, DLT698_DAR_TEMPORARY_FAILURE);
}

static void test_get_normal_encode_type_mismatch_returns_dar(void **state) {
    (void)state;
    uint8_t frame[128];
    size_t frame_len = build_get_normal(frame, OAD_VOLTAGE);

    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_get_normal(frame, frame_len, &k_bad_type_src, &pool);
    assert_int_equal(out->b.data[16], 0x00);
    assert_int_equal(out->b.data[17], DLT698_DAR_TYPE_MISMATCH);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static int list_class_mismatch(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    (void)oad;
    (void)buf;
    (void)cap;
    (void)len;
    return EDGE_SLAVE_SRC_CLASS_MISMATCH;
}

static const edge_slave_source_ops_t k_list_class_mismatch = {.resolve_axdr = list_class_mismatch};

static void test_normal_list_dar_mapping(void **state) {
    (void)state;
    uint32_t oads[] = {OAD_VOLTAGE};
    uint8_t frame[128];
    size_t frame_len = build_get_list(frame, oads, 1);

    net_buf_pool_t *pool = net_buf_pool_new(8, 4096, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc = dlt698_driver.req_slave_exchange(priv, NULL, frame, frame_len, &k_list_class_mismatch,
                                              NULL, out, &consumed);
    assert_int_equal(rc, 0);
    /* APDU: 85 | 02 | PIID | n | OAD(4) | 00 | DAR ...，APDU 从偏移 9 起。 */
    assert_int_equal(out->b.data[9], DLT698_SVC_GET_RES);
    assert_int_equal(out->b.data[10], DLT698_CHOICE_GET_NORMAL_LIST);
    assert_int_equal(out->b.data[12], 1);
    assert_int_equal(out->b.data[17], 0x00);
    assert_int_equal(out->b.data[18], DLT698_DAR_OBJECT_CLASS_MISMATCH);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static int g_set_rc;
static int g_set_calls;
static uint32_t g_set_oad;

static int set_accept(void *ctx, uint32_t oad, const uint8_t *axdr, size_t len) {
    (void)ctx;
    (void)axdr;
    (void)len;
    g_set_calls++;
    g_set_oad = oad;
    return g_set_rc;
}

static const edge_slave_source_ops_t k_set_src = {.accept_axdr = set_accept};

static double g_scalar_value;
static int g_scalar_calls;

static int scalar_accept(void *ctx, uint32_t oad, double value) {
    (void)ctx;
    (void)oad;
    g_scalar_value = value;
    g_scalar_calls++;
    return 0;
}

static const edge_slave_source_ops_t k_scalar_accept_src = {.accept = scalar_accept};

static size_t build_set(uint8_t *frame, uint32_t oad, const uint8_t *data, size_t dlen,
                        bool with_time_tag) {
    uint8_t apdu[64] = {DLT698_SVC_SET_REQ, 0x01, 0x01};
    size_t i = 3;
    apdu[i++] = (uint8_t)(oad >> 24);
    apdu[i++] = (uint8_t)(oad >> 16);
    apdu[i++] = (uint8_t)(oad >> 8);
    apdu[i++] = (uint8_t)oad;
    if (data && dlen > 0) {
        memcpy(apdu + i, data, dlen);
        i += dlen;
    }
    if (with_time_tag) apdu[i++] = DLT698_NO_TIME_TAG;
    return build_request(frame, apdu, i);
}

static net_buf_t *run_set(uint8_t *frame, size_t frame_len, const edge_slave_source_ops_t *src,
                          net_buf_pool_t **pool_out) {
    net_buf_pool_t *pool = net_buf_pool_new(8, 4096, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc =
        dlt698_driver.req_slave_exchange(priv, NULL, frame, frame_len, src, NULL, out, &consumed);
    assert_int_equal(rc, 0);
    *pool_out = pool;
    return out;
}

static void assert_set_dar(uint32_t oad, const uint8_t *data, size_t dlen, bool with_time_tag,
                           int src_rc, uint8_t want_dar) {
    g_set_rc = src_rc;
    int calls_before = g_set_calls;
    uint8_t frame[128];
    size_t frame_len = build_set(frame, oad, data, dlen, with_time_tag);

    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_set(frame, frame_len, &k_set_src, &pool);
    /* APDU: 86 | 01 | PIID | OAD(4) | DAR | 00 | 00，APDU 从偏移 9 起。 */
    assert_int_equal(out->b.data[9], DLT698_SVC_SET_RES);
    assert_int_equal(out->b.data[10], 0x01);
    assert_int_equal(out->b.data[16], want_dar);
    assert_int_equal(out->b.data[17], 0x00);
    assert_int_equal(out->b.data[18], 0x00);
    if (want_dar == DLT698_DAR_SUCCESS) assert_int_equal(g_set_calls, calls_before + 1);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static void test_set_response_dar_paths(void **state) {
    (void)state;
    const uint32_t oad = MK_OAD(OI_CHANNEL_CONFIG, 2);
    const uint8_t baud[] = {0x06, 0x00, 0x00, 0x09, 0x60};

    assert_set_dar(oad, baud, sizeof(baud), true, 0, DLT698_DAR_SUCCESS);
    assert_int_equal(g_set_oad, oad);
    const uint8_t f32[] = {0x17, 0x43, 0x67, 0xB3, 0x33};
    assert_set_dar(oad, f32, sizeof(f32), true, 0, DLT698_DAR_SUCCESS);
    assert_set_dar(oad, baud, sizeof(baud), true, EDGE_SLAVE_SRC_TYPE_MISMATCH,
                   DLT698_DAR_TYPE_MISMATCH);
    assert_set_dar(oad, baud, sizeof(baud), true, EDGE_SLAVE_SRC_DENIED, DLT698_DAR_DENIED);
    assert_set_dar(oad, baud, sizeof(baud), true, EDGE_SLAVE_SRC_NOT_FOUND,
                   DLT698_DAR_OBJECT_NOT_FOUND);
}

static void test_set_missing_data_dar7(void **state) {
    (void)state;
    g_set_calls = 0;
    g_set_rc = 0;
    uint8_t frame[128];
    size_t frame_len = build_set(frame, MK_OAD(OI_CHANNEL_CONFIG, 2), NULL, 0, false);

    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_set(frame, frame_len, &k_set_src, &pool);
    assert_int_equal(out->b.data[9], DLT698_SVC_SET_RES);
    assert_int_equal(out->b.data[16], DLT698_DAR_TYPE_MISMATCH);
    assert_int_equal(g_set_calls, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static void test_set_scalar_fallback(void **state) {
    (void)state;
    const uint8_t value[] = {0x12, 0x09, 0x60}; /* long-unsigned 2400 */
    uint8_t frame[128];
    size_t frame_len = build_set(frame, MK_OAD(OI_CHANNEL_CONFIG, 2), value, sizeof(value), true);

    g_scalar_value = 0.0;
    g_scalar_calls = 0;
    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_set(frame, frame_len, &k_scalar_accept_src, &pool);
    assert_int_equal(out->b.data[16], DLT698_DAR_SUCCESS);
    assert_float_equal((float)g_scalar_value, 2400.0f, 0.001f);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);

    const uint8_t neg[] = {0x14, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; /* long64 -1 */
    frame_len = build_set(frame, MK_OAD(OI_CHANNEL_CONFIG, 2), neg, sizeof(neg), true);
    out = run_set(frame, frame_len, &k_scalar_accept_src, &pool);
    assert_int_equal(out->b.data[16], DLT698_DAR_SUCCESS);
    assert_float_equal((float)g_scalar_value, -1.0f, 0.001f);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static void test_set_scalar_fallback_rejects_structure(void **state) {
    (void)state;
    const uint8_t structure[] = {0x02, 0x01, 0x12, 0x00, 0x01};
    uint8_t frame[128];
    size_t frame_len =
        build_set(frame, MK_OAD(OI_CHANNEL_CONFIG, 2), structure, sizeof(structure), true);

    g_scalar_calls = 0;
    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_set(frame, frame_len, &k_scalar_accept_src, &pool);
    assert_int_equal(out->b.data[16], DLT698_DAR_TYPE_MISMATCH);
    assert_int_equal(g_scalar_calls, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_product_variable_axdr_types),
        cmocka_unit_test(test_product_axdr_rejects_small_buffer_and_complex),
        cmocka_unit_test(test_driver_scalar_fallback_still_works),
        cmocka_unit_test(test_driver_carries_structure_axdr),
        cmocka_unit_test(test_driver_rejects_axdr_buffer_shortage),
        cmocka_unit_test(test_driver_normal_list_uses_axdr_per_item),
        cmocka_unit_test(test_driver_normal_list_reuses_one_buffer),
        cmocka_unit_test(test_get_normal_dar_mapping),
        cmocka_unit_test(test_get_normal_encode_type_mismatch_returns_dar),
        cmocka_unit_test(test_normal_list_dar_mapping),
        cmocka_unit_test(test_product_accept_set_paths),
        cmocka_unit_test(test_set_response_dar_paths),
        cmocka_unit_test(test_set_missing_data_dar7),
        cmocka_unit_test(test_set_scalar_fallback),
        cmocka_unit_test(test_set_scalar_fallback_rejects_structure),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
