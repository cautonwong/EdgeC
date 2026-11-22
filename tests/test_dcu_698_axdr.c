#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "datahub.h"
#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_param.h"
#include "dcu_698_var.h"
#include "dcu_discovery.h"
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
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

/* date_time_s 是设备本地民用时间：固定东八区，期望字节才能确定。 */
static char g_saved_tz[128];
static bool g_had_tz;

static int setup_timezone(void **state) {
    (void)state;
    const char *tz = getenv("TZ");
    g_had_tz = tz != NULL;
    if (g_had_tz) snprintf(g_saved_tz, sizeof(g_saved_tz), "%s", tz);
    setenv("TZ", "CST-8", 1);
    tzset();
    return 0;
}

static int teardown_timezone(void **state) {
    (void)state;
    if (g_had_tz) {
        setenv("TZ", g_saved_tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    return 0;
}

static struct tm g_clock_value;

static int fake_clock_now(void *ctx, struct tm *out) {
    (void)ctx;
    *out = g_clock_value;
    return 0;
}

static int fake_clock_set(void *ctx, const struct tm *in) {
    (void)ctx;
    g_clock_value = *in;
    return 0;
}

static int fake_param_save(void *ctx, uint16_t oi, uint8_t attr, const uint8_t *buf, size_t len) {
    (void)ctx;
    (void)oi;
    (void)attr;
    (void)buf;
    (void)len;
    return 0;
}

static int fake_param_load(void *ctx, uint16_t oi, uint8_t attr, uint8_t *buf, size_t cap,
                           size_t *len) {
    (void)ctx;
    (void)oi;
    (void)attr;
    (void)buf;
    (void)cap;
    (void)len;
    return 1;
}

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
        case TAG_VOLTAGE_B:
            value = 220.1;
            break;
        case TAG_VOLTAGE_C:
            value = 219.9;
            break;
        case OAD_CURRENT:
            value = 4.95;
            break;
        case TAG_CURRENT_B:
            value = 5.0;
            break;
        case TAG_CURRENT_C:
            value = 5.05;
            break;
        case OAD_ACTIVE_POWER_TOTAL:
            value = 1234.5;
            break;
        case TAG_ACTIVE_POWER_A:
            value = 400.0;
            break;
        case TAG_ACTIVE_POWER_B:
            value = 410.0;
            break;
        case TAG_ACTIVE_POWER_C:
            value = 420.0;
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

static dcu_poller_t g_poller;
static dcu_discovery_t g_discovery;
static dcu_task_op_t g_applied_op;
static int g_apply_calls;

static int test_task_apply(void *ctx, dcu_poller_t *poller, const dcu_task_op_t *op) {
    (void)ctx;
    (void)poller;
    g_apply_calls++;
    if (op) g_applied_op = *op;
    return 0;
}

static void init_product_model(void) {
    static dcu_inventory_t inv;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&g_poller), 0);
    dcu_discovery_init(&g_discovery);
    g_discovery.count = 0;
    g_discovery.status = DISCOVERY_STATUS_IDLE;
    router_sink_ops_t sink = {.task_apply = test_task_apply};
    router_bus_ops_t bus = {.msg_size = sizeof(pipe_msg_t), .read_shadow = mock_shadow};
    assert_int_equal(
        dcu_698_object_model_init(&inv, &g_poller, sink, NULL, &g_discovery, NULL, NULL, bus), 0);
}

static void init_product_model_with_events(void);

/* 标准 7.3.24 表193：A11 enum 上报标识（0–3），A12 bool 有效标识。 */
static void test_product_accept_set_event_attrs(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    init_product_model_with_events();

    const uint8_t enum1[] = {0x16, 0x01};
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 11), enum1, sizeof(enum1)),
                     0);
    uint8_t flag = 0xFF;
    assert_int_equal(dcu_698_read(MK_OAD(OI_VOLTAGE_DROP, 11), &flag), 0);
    assert_int_equal(flag, 1);

    const uint8_t bool_false[] = {0x03, 0x00};
    assert_int_equal(
        dcu_north_accept_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 12), bool_false, sizeof(bool_false)),
        0);
    bool enabled = true;
    assert_int_equal(dcu_698_read(MK_OAD(OI_VOLTAGE_DROP, 12), &enabled), 0);
    assert_false(enabled);

    const uint8_t enum4[] = {0x16, 0x04};
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 11), enum4, sizeof(enum4)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_read(MK_OAD(OI_VOLTAGE_DROP, 11), &flag), 0);
    assert_int_equal(flag, 1);

    const uint8_t u8_one[] = {0x11, 0x01};
    assert_int_equal(
        dcu_north_accept_axdr(NULL, MK_OAD(OI_CHANNEL_CONFIG, 4), u8_one, sizeof(u8_one)),
        EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(
        dcu_north_accept_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 6), u8_one, sizeof(u8_one)),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    dcu_698_object_model_reset();
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

    const uint8_t want_voltage[] = {0x01, 0x03, 0x12, 0x08, 0x9D, 0x12,
                                    0x08, 0x99, 0x12, 0x08, 0x97};
    assert_axdr(OAD_VOLTAGE, want_voltage, sizeof(want_voltage));

    /* 0010/0020：属性 2/4 是数组；第一项是总电能量。 */
    const uint8_t want_energy2[] = {0x01, 0x01, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, 0x00, 0x12,
                                    0xD6, 0x44};
    assert_axdr(MK_OAD(0x0010, 2), want_energy2, sizeof(want_energy2));
    const uint8_t want_energy4[] = {
        0x01, 0x01, DCU_698_TAG_LONG64_UNSIGNED, 0x00, 0x00, 0x00, 0x00, 0x07, 0x5B, 0xB2, 0x90};
    assert_axdr(MK_OAD(0x0010, 4), want_energy4, sizeof(want_energy4));
    const uint8_t want_reactive2[] = {0x01, 0x01, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, 0x00, 0x0A,
                                      0x5B, 0xF4};
    assert_axdr(MK_OAD(0x0020, 2), want_reactive2, sizeof(want_reactive2));

    const uint8_t want_freq[] = {DCU_698_TAG_LONG_UNSIGNED, 0x13, 0x89};
    assert_axdr(OAD_GRID_FREQUENCY, want_freq, sizeof(want_freq));

    /* 6000 A2 是配置数组：空档案回空数组，不是标量、也不是 DAR。 */
    uint8_t probe[EDGE_SLAVE_AXDR_MAX];
    size_t probe_len = 0;
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_METER_ARCHIVE, 2), probe, sizeof(probe), &probe_len),
        0);
    assert_int_equal(probe_len, 2);
    assert_int_equal(probe[0], DCU_698_TAG_ARRAY);
    assert_int_equal(probe[1], 0);
    const uint8_t want_discovery_count[] = {DCU_698_TAG_LONG_UNSIGNED, 0x00, 0x00};
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 3), want_discovery_count, sizeof(want_discovery_count));

    dcu_698_object_model_reset();
}

static void test_product_axdr_rejects_small_buffer_and_complex(void **state) {
    (void)state;
    init_product_model();

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;

    /* 三相电压数组需要 11 字节；cap=10 必须失败，不截断。 */
    assert_int_equal(dcu_north_resolve_axdr(NULL, OAD_VOLTAGE, buf, 10, &len),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* 5004 A2 是冻结数据数组而不是条数；Normal GET 不暴露，回 DAR 5。 */
    assert_int_equal(dcu_north_resolve_axdr(NULL, OAD_DAILY_FREEZE, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x6000, 3), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x6000, 18), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);

    dcu_698_object_model_reset();
}

static void test_product_accept_set_paths(void **state) {
    (void)state;
    init_product_model();

    const uint8_t baud[] = {0x06, 0x00, 0x00, 0x09, 0x60}; /* 2400, double-long-unsigned */
    /* F200 的 A2/A3/A4 没有真实可落地的串口重配路径，全部不对外。 */
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(OI_CHANNEL_CONFIG, 2), baud, sizeof(baud)),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    uint32_t got = 0;
    assert_int_equal(dcu_698_read(MK_OAD(OI_CHANNEL_CONFIG, 2), &got), -1);

    const uint8_t like_float[] = {0x17, 0x43, 0x67, 0xB3, 0x33};
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x2000, 2), like_float, sizeof(like_float)),
                     EDGE_SLAVE_SRC_DENIED);

    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(OI_CHANNEL_CONFIG, 4), baud, sizeof(baud)),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);

    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 6), baud, sizeof(baud)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 18), baud, sizeof(baud)),
                     EDGE_SLAVE_SRC_UNDEFINED);

    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x9999, 2), baud, sizeof(baud)),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    dcu_698_object_model_reset();
}

static void test_product_class8_datetime_round_trip(void **state) {
    (void)state;
    init_product_model();

    g_clock_value = (struct tm){
        .tm_year = 124, .tm_mon = 4, .tm_mday = 6, .tm_hour = 7, .tm_min = 8, .tm_sec = 9};
    static const dcu_clock_port_t clock = {
        .now = fake_clock_now, .set = fake_clock_set, .ctx = NULL};
    static const dcu_param_store_t store = {.save = fake_param_save, .load = fake_param_load};
    dcu_698_param_set_clock(&clock);
    dcu_698_param_init(&store);

    static const uint8_t when[] = {0x1C, 0x07, 0xE9, 0x01, 0x02, 0x03, 0x04, 0x05};
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x4000, 2), when, sizeof(when)), 0);
    assert_axdr(MK_OAD(0x4000, 2), when, sizeof(when));

    const uint8_t mode[] = {0x16, 0x02};
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x4000, 3), mode, sizeof(mode)), 0);
    assert_axdr(MK_OAD(0x4000, 3), mode, sizeof(mode));

    const uint8_t precise[] = {0x02, 0x05, 0x11, 1, 0x11, 2, 0x11, 3, 0x11, 4, 0x11, 5};
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x4000, 4), precise, sizeof(precise)), 0);
    assert_axdr(MK_OAD(0x4000, 4), precise, sizeof(precise));

    const uint8_t one[] = {0x11, 0x01};
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x4000, 1), one, sizeof(one)),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x4001, 1), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x4001, 3), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x4005, 2), one, sizeof(one)),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    static const uint8_t bad_month[] = {0x1C, 0x07, 0xE8, 0x0D, 0x06, 0x07, 0x08, 0x09};
    assert_int_equal(dcu_north_accept_axdr(NULL, MK_OAD(0x4000, 2), bad_month, sizeof(bad_month)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_axdr(MK_OAD(0x4000, 2), when, sizeof(when));

    dcu_698_object_model_reset();
}

static void test_product_class2_and_5_have_no_value_yet(void **state) {
    (void)state;
    init_product_model();

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    static const uint16_t defined[] = {0x1010, 0x1011, 0x1012, 0x1013};
    for (size_t i = 0; i < sizeof(defined) / sizeof(defined[0]); i++) {
        for (uint8_t attr = 1; attr <= 3; attr++) {
            assert_int_equal(
                dcu_north_resolve_axdr(NULL, MK_OAD(defined[i], attr), buf, sizeof(buf), &len),
                EDGE_SLAVE_SRC_CLASS_MISMATCH);
        }
    }
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x1010, 4), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);

    for (uint8_t attr = 1; attr <= 6; attr++) {
        assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x200D, attr), buf, sizeof(buf), &len),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
    }
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x200D, 7), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x200E, 2), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);

    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x2141, 2), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_NOT_FOUND);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(0x9999, 2), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    assert_int_equal(dcu_north_accept_action(NULL, MK_OAD(0x1010, 1), NULL, 0),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OAD(0x1010, 2), NULL, 0),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OAD(0x1010, 3), NULL, 0),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OAD(0x200D, 1), NULL, 0),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OAD(0x200E, 2), NULL, 0),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
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

static int high_oad_resolve(void *ctx, uint32_t oad, const void *desc, size_t desc_len,
                            edge_point_value_t *out) {
    (void)ctx;
    (void)desc;
    (void)desc_len;
    if (!out || oad != 0xF2000200U) return -1;
    memset(out, 0, sizeof(*out));
    out->value = 9600.0;
    out->raw = 9600;
    out->width = 4;
    out->type = DLT698_TAG_DOUBLE_LONG;
    return 0;
}

static const edge_slave_source_ops_t k_high_oad_src = {.resolve = high_oad_resolve};

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

static const uint8_t k_test_acl[] = {
    0x01, 0x0E, 0x02, 0x02, 0x50, 0x00, 0x10, 0x02, 0x02, 0x01, 0x02, 0x02, 0x02, 0x11, 0x02, 0x16,
    0x03, 0x02, 0x02, 0x11, 0x04, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50, 0x00, 0x20, 0x02, 0x02,
    0x01, 0x01, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50, 0x20, 0x00, 0x02,
    0x02, 0x01, 0x04, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x02, 0x02, 0x11, 0x03, 0x16, 0x03, 0x02,
    0x02, 0x11, 0x04, 0x16, 0x03, 0x02, 0x02, 0x11, 0x05, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50,
    0x20, 0x01, 0x02, 0x02, 0x01, 0x04, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x02, 0x02, 0x11, 0x03,
    0x16, 0x03, 0x02, 0x02, 0x11, 0x04, 0x16, 0x03, 0x02, 0x02, 0x11, 0x05, 0x16, 0x03, 0x01, 0x00,
    0x02, 0x02, 0x50, 0x20, 0x04, 0x02, 0x02, 0x01, 0x02, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x02,
    0x02, 0x11, 0x03, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50, 0x20, 0x0A, 0x02, 0x02, 0x01, 0x01,
    0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50, 0x20, 0x0F, 0x02, 0x02, 0x01,
    0x01, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50, 0x50, 0x00, 0x02, 0x02,
    0x01, 0x01, 0x02, 0x02, 0x11, 0x02, 0x16, 0x01, 0x01, 0x00, 0x02, 0x02, 0x50, 0x50, 0x04, 0x02,
    0x02, 0x01, 0x01, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50, 0x60, 0x00,
    0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50, 0x60,
    0x12, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50,
    0xF2, 0x00, 0x02, 0x02, 0x01, 0x02, 0x02, 0x02, 0x11, 0x02, 0x16, 0x03, 0x02, 0x02, 0x11, 0x03,
    0x16, 0x03, 0x01, 0x00, 0x02, 0x02, 0x50, 0x80, 0x00, 0x02, 0x02, 0x01, 0x00, 0x01, 0x07, 0x02,
    0x02, 0x11, 0x01, 0x03, 0x01, 0x02, 0x02, 0x11, 0x7E, 0x03, 0x01, 0x02, 0x02, 0x11, 0x7F, 0x03,
    0x01, 0x02, 0x02, 0x11, 0x80, 0x03, 0x01, 0x02, 0x02, 0x11, 0x81, 0x03, 0x01, 0x02, 0x02, 0x11,
    0x82, 0x03, 0x01, 0x02, 0x02, 0x11, 0x83, 0x03, 0x01, 0x02, 0x02, 0x50, 0x30, 0x00, 0x02, 0x02,
    0x01, 0x05, 0x02, 0x02, 0x11, 0x04, 0x16, 0x01, 0x02, 0x02, 0x11, 0x06, 0x16, 0x01, 0x02, 0x02,
    0x11, 0x0B, 0x16, 0x01, 0x02, 0x02, 0x11, 0x0C, 0x16, 0x01, 0x02, 0x02, 0x11, 0x08, 0x16, 0x01,
    0x01, 0x00,
};

static int test_object_list(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    if (oad != 0x44000200u) return -1;
    if (sizeof(k_test_acl) > cap) return -1;
    memcpy(buf, k_test_acl, sizeof(k_test_acl));
    *len = sizeof(k_test_acl);
    return 0;
}

/* 只给 ACTION 路由用例用的 ACL：放行 5004 方法 3、6012 方法 1/127、未知 OI 9999 方法 1，
 * 让请求越过 ACL 到达适配器/对象表。 */
static const uint8_t k_routing_acl[] = {
    0x01, 0x03, 0x02, 0x02, 0x50, 0x50, 0x04, 0x02, 0x02, 0x01, 0x00, 0x01, 0x01, 0x02, 0x02,
    0x11, 0x03, 0x03, 0x01, 0x02, 0x02, 0x50, 0x60, 0x12, 0x02, 0x02, 0x01, 0x00, 0x01, 0x02,
    0x02, 0x02, 0x11, 0x01, 0x03, 0x01, 0x02, 0x02, 0x11, 0x7F, 0x03, 0x01, 0x02, 0x02, 0x50,
    0x99, 0x99, 0x02, 0x02, 0x01, 0x00, 0x01, 0x01, 0x02, 0x02, 0x11, 0x01, 0x03, 0x01,
};

static int routing_object_list(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    if (oad != 0x44000200u) return -1;
    if (sizeof(k_routing_acl) > cap) return -1;
    memcpy(buf, k_routing_acl, sizeof(k_routing_acl));
    *len = sizeof(k_routing_acl);
    return 0;
}

static int test_exchange_with(void *priv, const uint8_t *frame, size_t frame_len,
                              const edge_slave_source_ops_t *src, net_buf_t *out, size_t *consumed,
                              int (*object_list)(void *, uint32_t, uint8_t *, size_t, size_t *)) {
    edge_slave_source_ops_t wrapped = *src;
    wrapped.object_list = object_list;
    static uint8_t session[1024];
    memset(session, 0, sizeof(session));
    return dlt698_driver.req_slave_exchange(priv, session, frame, frame_len, &wrapped, (void *)src,
                                            out, consumed);
}

static int test_exchange(void *priv, const uint8_t *frame, size_t frame_len,
                         const edge_slave_source_ops_t *src, void *src_ctx, net_buf_t *out,
                         size_t *consumed) {
    (void)src_ctx;
    return test_exchange_with(priv, frame, frame_len, src, out, consumed, test_object_list);
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
    int rc = test_exchange(priv, frame, frame_len, src, NULL, out, &consumed);
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
    int rc = test_exchange(priv, frame, frame_len, &k_too_big_src, NULL, out, &consumed);
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
    int rc = test_exchange(priv, frame, frame_len, &k_list_src, NULL, out, &consumed);
    assert_int_equal(rc, 0);

    const uint8_t voltage[] = {0x17, 0x43, 0x67, 0xB3, 0x33};
    const uint8_t count[] = {0x12, 0x00, 0x28};
    assert_true(contains(out->b.data, out->b.len, voltage, sizeof(voltage)));
    assert_true(contains(out->b.data, out->b.len, count, sizeof(count)));

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static int mixed_list_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    static const uint8_t voltage[] = {0x17, 0x43, 0x67, 0xB3, 0x33};
    static const uint8_t power[] = {0x17, 0x00, 0x00, 0x00, 0x00};
    const uint8_t *data = NULL;
    size_t n = 0;
    if (oad == OAD_VOLTAGE) {
        data = voltage;
        n = sizeof(voltage);
    } else if (oad == OAD_ACTIVE_POWER_TOTAL) {
        data = power;
        n = sizeof(power);
    } else {
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    if (!buf || !len || cap < n) return -1;
    memcpy(buf, data, n);
    *len = n;
    return 0;
}

static const edge_slave_source_ops_t k_mixed_list_src = {.resolve_axdr = mixed_list_resolve};

static void assert_apdu(const net_buf_t *out, const uint8_t *want, size_t want_len) {
    assert_int_equal(out->b.len, 9 + want_len + 3);
    assert_memory_equal(out->b.data + 9, want, want_len);
}

static net_buf_t *run_list(const uint32_t *oads, size_t n, const edge_slave_source_ops_t *src,
                           net_buf_pool_t **pool_out) {
    uint8_t frame[128];
    size_t frame_len = build_get_list(frame, oads, n);
    net_buf_pool_t *pool = net_buf_pool_new(8, 4096, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc = test_exchange(priv, frame, frame_len, src, NULL, out, &consumed);
    assert_int_equal(rc, 0);
    *pool_out = pool;
    return out;
}

static void test_driver_normal_list_mixed_success_and_dar(void **state) {
    (void)state;
    const uint32_t oads[] = {OAD_VOLTAGE, OAD_CURRENT, OAD_ACTIVE_POWER_TOTAL};
    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_list(oads, 3, &k_mixed_list_src, &pool);

    const uint8_t want[] = {
        0x85, 0x02, 0x01, 0x03, 0x20, 0x00, 0x02, 0x00, 0x01, 0x17,
        0x43, 0x67, 0xB3, 0x33, 0x20, 0x01, 0x02, 0x00, 0x00, DLT698_DAR_OBJECT_NOT_FOUND,
        0x20, 0x04, 0x02, 0x00, 0x01, 0x17, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
    };
    assert_apdu(out, want, sizeof(want));

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

/* 表73 的 SEQUENCE OF 没有 SIZE 下限；标准对 RCSD 明确写了 0 项合法
 * （docs/698_parsed.md:1880）。空请求要回空结果列表，不能当坏帧丢掉。 */
static void test_driver_normal_list_empty_request(void **state) {
    (void)state;
    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_list(NULL, 0, &k_mixed_list_src, &pool);

    const uint8_t want[] = {0x85, 0x02, 0x01, 0x00, 0x00, 0x00};
    assert_apdu(out, want, sizeof(want));

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

static int g_dup_calls;

static int duplicate_list_resolve(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    (void)oad;
    if (!buf || !len || cap < 3) return -1;
    uint16_t value = (uint16_t)++g_dup_calls;
    buf[0] = DCU_698_TAG_LONG_UNSIGNED;
    buf[1] = (uint8_t)(value >> 8);
    buf[2] = (uint8_t)value;
    *len = 3;
    return 0;
}

static const edge_slave_source_ops_t k_duplicate_list_src = {.resolve_axdr =
                                                                 duplicate_list_resolve};

static void test_driver_normal_list_duplicate_oad_each_answered(void **state) {
    (void)state;
    g_dup_calls = 0;
    const uint32_t oads[] = {MK_OAD(OI_METER_ARCHIVE, 2), MK_OAD(OI_METER_ARCHIVE, 2)};
    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_list(oads, 2, &k_duplicate_list_src, &pool);

    const uint8_t want[] = {
        0x85, 0x02, 0x01, 0x02, 0x60, 0x00, 0x02, 0x00, 0x01, 0x12, 0x00,
        0x01, 0x60, 0x00, 0x02, 0x00, 0x01, 0x12, 0x00, 0x02, 0x00, 0x00,
    };
    assert_apdu(out, want, sizeof(want));
    assert_int_equal(g_dup_calls, 2);

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
    for (size_t i = 0; i < 8; i++) oads[i] = 0x50000200U | (uint32_t)i;

    uint8_t frame[128];
    size_t frame_len = build_get_list(frame, oads, 8);

    net_buf_pool_t *pool = net_buf_pool_new(2, 32768, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc = test_exchange(priv, frame, frame_len, &k_big_list_src, NULL, out, &consumed);
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
    int rc = test_exchange(priv, frame, frame_len, src, NULL, out, &consumed);
    assert_int_equal(rc, 0);
    *pool_out = pool;
    return out;
}

static dcu_event_obj_t s_evt_3000;
static dcu_event_obj_t s_evt_3005;

static void init_product_model_with_events(void) {
    static dcu_inventory_t inv;
    static dcu_poller_t poller;
    assert_int_equal(dcu_inventory_init(&inv), 0);
    assert_int_equal(dcu_poller_init(&poller), 0);
    memset(&s_evt_3000, 0, sizeof(s_evt_3000));
    s_evt_3000.oi = OI_VOLTAGE_DROP;
    s_evt_3000.report_flag = 2;
    s_evt_3000.is_enabled = true;
    memset(&s_evt_3005, 0, sizeof(s_evt_3005));
    s_evt_3005.oi = OI_OVER_CURRENT;
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {.msg_size = sizeof(pipe_msg_t), .read_shadow = mock_shadow};
    assert_int_equal(
        dcu_698_object_model_init(&inv, &poller, sink, NULL, NULL, &s_evt_3000, &s_evt_3005, bus),
        0);
}

static int product_resolve_axdr(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    return dcu_north_resolve_axdr(NULL, oad, buf, cap, len);
}

static const edge_slave_source_ops_t k_product_axdr_src = {.resolve_axdr = product_resolve_axdr};

static void test_event_class24_attrs_on_the_wire(void **state) {
    (void)state;
    init_product_model_with_events();
    uint8_t frame[128];
    net_buf_pool_t *pool = NULL;

    size_t frame_len = build_get_normal(frame, MK_OAD(OI_VOLTAGE_DROP, 11));
    net_buf_t *out = run_get_normal(frame, frame_len, &k_product_axdr_src, &pool);
    assert_int_equal(out->b.data[16], 0x01);
    assert_int_equal(out->b.data[17], DLT698_TAG_ENUM);
    assert_int_equal(out->b.data[18], 2);
    net_buf_unref(out);
    net_buf_pool_destroy(pool);

    frame_len = build_get_normal(frame, MK_OAD(OI_VOLTAGE_DROP, 12));
    out = run_get_normal(frame, frame_len, &k_product_axdr_src, &pool);
    assert_int_equal(out->b.data[16], 0x01);
    assert_int_equal(out->b.data[17], DLT698_TAG_BOOLEAN);
    assert_int_equal(out->b.data[18], 1);
    net_buf_unref(out);
    net_buf_pool_destroy(pool);

    /* A8 已进 ACL 但对象表未声明：源层类不匹配，线上 DAR 5。 */
    frame_len = build_get_normal(frame, MK_OAD(OI_VOLTAGE_DROP, 8));
    out = run_get_normal(frame, frame_len, &k_product_axdr_src, &pool);
    assert_int_equal(out->b.data[16], 0x00);
    assert_int_equal(out->b.data[17], DLT698_DAR_OBJECT_CLASS_MISMATCH);
    net_buf_unref(out);
    net_buf_pool_destroy(pool);

    dcu_698_object_model_reset();
}

/* 表194：A2/A3 的线格式。 */
static void test_event_class24_complex_attrs(void **state) {
    (void)state;
    init_product_model_with_events();

    const uint8_t a4[] = {0x12, 0x00, 0x0A};
    assert_axdr(MK_OAD(OI_VOLTAGE_DROP, 4), a4, sizeof(a4));

    const uint8_t a2_empty[] = {0x01, 0x00};
    assert_axdr(MK_OAD(OI_VOLTAGE_DROP, 2), a2_empty, sizeof(a2_empty));

    s_evt_3000.snapshot_count = 2;
    s_evt_3000.snapshot_oads[0] = OAD_VOLTAGE;
    s_evt_3000.snapshot_oads[1] = OAD_ACTIVE_ENERGY;
    const uint8_t a2_two[] = {0x01, 0x02, 0x51, 0x20, 0x00, 0x02,
                              0x00, 0x51, 0x00, 0x10, 0x02, 0x00};
    assert_axdr(MK_OAD(OI_VOLTAGE_DROP, 2), a2_two, sizeof(a2_two));

    s_evt_3000.record_count = 3;
    const uint8_t a3[] = {0x02, 0x04, 0x12, 0x00, 0x03, 0x12, 0x00,
                          0x00, 0x12, 0x00, 0x00, 0x12, 0x00, 0x00};
    assert_axdr(MK_OAD(OI_VOLTAGE_DROP, 3), a3, sizeof(a3));

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    /* A5 未暴露：表A.4 对 3000 要求 4 字段、对 3005 要求 2 字段结构，dcu_event_obj_t 未存储。 */
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 5), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_OVER_CURRENT, 5), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_CLASS_MISMATCH);

    /* 表194 明确允许 A14 未发生/未结束时编 NULL；A6 有记录条数但记录里没有墙钟，
     * 发生时间必填，必须拒绝而不是编 1970。 */
    const uint8_t a14_empty[] = {0x01, 0x01, 0x02, 0x02, 0x00, 0x00};
    assert_axdr(MK_OAD(OI_VOLTAGE_DROP, 14), a14_empty, sizeof(a14_empty));
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 6), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 7), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_CLASS_MISMATCH);

    dcu_698_object_model_reset();
}

static void test_event_class24_record_and_status_times(void **state) {
    (void)state;
    init_product_model_with_events();

    s_evt_3000.record_count = 1;
    s_evt_3000.records[0].index = 1;
    s_evt_3000.records[0].start_time = (dcu_event_stamp_t){.mono_ns = 123, .epoch_s = 1700000000};
    s_evt_3000.records[0].end_time = (dcu_event_stamp_t){0};

    /* epoch 1700000000 在 TZ=CST-8 下是 2023-11-15T06:13:20；
     * 未结束的 A6 结束时间编表49 非法时标（年 FFFFH、其余 FFH），不是 NULL。 */
    const uint8_t a6_no_snapshots[] = {0x01, 0x01, 0x02, 0x04, 0x06, 0x00, 0x00, 0x00, 0x01,
                                       0x1C, 0x07, 0xE7, 0x0B, 0x0F, 0x06, 0x0D, 0x14, 0x1C,
                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00};
    assert_axdr(MK_OAD(OI_VOLTAGE_DROP, 6), a6_no_snapshots, sizeof(a6_no_snapshots));

    s_evt_3000.snapshot_count = 2;
    s_evt_3000.records[0].snapshot_len[0] = 3;
    s_evt_3000.records[0].snapshot[0][0] = 0x12;
    s_evt_3000.records[0].snapshot[0][1] = 0x08;
    s_evt_3000.records[0].snapshot[0][2] = 0x9D;
    s_evt_3000.records[0].snapshot_len[1] = 0;
    const uint8_t a6_with_snapshots[] = {0x01, 0x01, 0x02, 0x06, 0x06, 0x00, 0x00, 0x00,
                                         0x01, 0x1C, 0x07, 0xE7, 0x0B, 0x0F, 0x06, 0x0D,
                                         0x14, 0x1C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                         0xFF, 0x01, 0x00, 0x12, 0x08, 0x9D, 0x00};
    assert_axdr(MK_OAD(OI_VOLTAGE_DROP, 6), a6_with_snapshots, sizeof(a6_with_snapshots));

    s_evt_3000.last_start_time = (dcu_event_stamp_t){.mono_ns = 456, .epoch_s = 1700000000};
    s_evt_3000.records[0].end_time = (dcu_event_stamp_t){.mono_ns = 789, .epoch_s = 1700003600};
    s_evt_3000.last_end_time = s_evt_3000.records[0].end_time;
    /* 2023-11-15T06:13:20 / 07:13:20（CST-8）。 */
    const uint8_t a14_two_times[] = {0x01, 0x01, 0x02, 0x02, 0x1C, 0x07, 0xE7, 0x0B, 0x0F, 0x06,
                                     0x0D, 0x14, 0x1C, 0x07, 0xE7, 0x0B, 0x0F, 0x07, 0x0D, 0x14};
    assert_axdr(MK_OAD(OI_VOLTAGE_DROP, 14), a14_two_times, sizeof(a14_two_times));

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    s_evt_3000.records[0].start_time =
        (dcu_event_stamp_t){.mono_ns = 123, .epoch_s = 1716500000000000000ULL};
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 6), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    s_evt_3000.records[0].start_time = (dcu_event_stamp_t){.mono_ns = 123, .epoch_s = 0};
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 6), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    s_evt_3000.last_start_time = (dcu_event_stamp_t){.mono_ns = 456, .epoch_s = 0};
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_VOLTAGE_DROP, 14), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    dcu_698_object_model_reset();
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
    int rc = test_exchange(priv, frame, frame_len, &k_list_class_mismatch, NULL, out, &consumed);
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
    int rc = test_exchange(priv, frame, frame_len, src, NULL, out, &consumed);
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

/* ACTION-Request-Normal：07 | 01 | PIID | OMD(4) | Data | 时间标签。 */
static size_t build_action_omd(uint8_t *frame, uint32_t omd, const uint8_t *param,
                               size_t param_len) {
    uint8_t apdu[256] = {
        DLT698_SVC_ACTION_REQ, 0x01,        0x05, (uint8_t)(omd >> 24), (uint8_t)(omd >> 16),
        (uint8_t)(omd >> 8),   (uint8_t)omd};
    size_t i = 7;
    assert_true(i + param_len + 1 <= sizeof(apdu));
    if (param_len > 0) memcpy(apdu + i, param, param_len);
    i += param_len;
    apdu[i++] = DLT698_NO_TIME_TAG;
    return build_request(frame, apdu, i);
}

static const edge_slave_source_ops_t k_action_src = {.accept_action = dcu_north_accept_action};

static uint8_t run_action_dar_omd_acl_initialized(uint32_t omd, const uint8_t *param,
                                                  size_t param_len,
                                                  int (*object_list)(void *, uint32_t, uint8_t *,
                                                                     size_t, size_t *)) {
    uint8_t frame[512];
    size_t frame_len = build_action_omd(frame, omd, param, param_len);

    net_buf_pool_t *pool = net_buf_pool_new(8, 4096, 0);
    assert_non_null(pool);
    net_buf_t *out = net_buf_alloc(pool, 0);
    assert_non_null(out);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc = test_exchange_with(priv, frame, frame_len, &k_action_src, out, &consumed, object_list);
    assert_int_equal(rc, 0);
    assert_int_equal(out->b.data[9], DLT698_SVC_ACTION_RES);
    assert_int_equal(out->b.data[10], 0x01);
    uint8_t dar = out->b.data[16];

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
    return dar;
}

static uint8_t run_action_dar_omd_acl(uint32_t omd, const uint8_t *param, size_t param_len,
                                      int (*object_list)(void *, uint32_t, uint8_t *, size_t,
                                                         size_t *)) {
    init_product_model();
    return run_action_dar_omd_acl_initialized(omd, param, param_len, object_list);
}

static uint8_t run_action_dar_omd(uint32_t omd, const uint8_t *param, size_t param_len) {
    return run_action_dar_omd_acl(omd, param, param_len, test_object_list);
}

static uint8_t run_action_dar(uint8_t method, const uint8_t *param, size_t param_len) {
    return run_action_dar_omd(((uint32_t)OI_REMOTE_CONTROL << 16) | ((uint32_t)method << 8), param,
                              param_len);
}

static void assert_action_dar(uint8_t method, const uint8_t *param, size_t param_len,
                              uint8_t want_dar) {
    assert_int_equal(run_action_dar(method, param, param_len), want_dar);
}

/* 表 A.9：129 = array<structure{OAD, unsigned, long-unsigned, bool}>。 */
static const uint8_t k_trip_param[] = {0x01, 0x01, 0x02, 0x04, 0x51, 0x20, 0x00, 0x04,
                                       0x00, 0x11, 0x05, 0x12, 0x00, 0x0A, 0x03, 0x01};

static void test_action_127_128_null_param(void **state) {
    (void)state;
    const uint8_t null_param[] = {0x00};
    assert_action_dar(127, null_param, sizeof(null_param), DLT698_DAR_TEMPORARY_FAILURE);
    assert_action_dar(128, null_param, sizeof(null_param), DLT698_DAR_TEMPORARY_FAILURE);

    const uint8_t wrong[] = {0x11, 0x00};
    assert_action_dar(127, wrong, sizeof(wrong), DLT698_DAR_TYPE_MISMATCH);
    assert_action_dar(128, wrong, sizeof(wrong), DLT698_DAR_TYPE_MISMATCH);
}

static void test_action_129_trip_param(void **state) {
    (void)state;
    dcu_action_param_t p;
    assert_int_equal(dcu_698_decode_action_param(129, k_trip_param, sizeof(k_trip_param), &p), 0);
    assert_int_equal(p.count, 1);
    assert_int_equal(p.relay_oad, 0x20000400U);
    assert_int_equal(p.alarm_delay_min, 5);
    assert_int_equal(p.limit_minutes, 10);
    assert_true(p.auto_close);

    const uint8_t two_items[] = {0x01, 0x02, 0x02, 0x04, 0x51, 0x20, 0x00, 0x04, 0x00, 0x11,
                                 0x05, 0x12, 0x00, 0x0A, 0x03, 0x01, 0x02, 0x04, 0x51, 0x20,
                                 0x00, 0x04, 0x01, 0x11, 0x06, 0x12, 0x00, 0x0B, 0x03, 0x00};
    assert_int_equal(dcu_698_decode_action_param(129, two_items, sizeof(two_items), &p), 0);
    assert_int_equal(p.count, 2);
    assert_int_equal(p.relay_oad, 0x20000400U);
    assert_action_dar(129, two_items, sizeof(two_items), DLT698_DAR_TEMPORARY_FAILURE);

    assert_action_dar(129, k_trip_param, sizeof(k_trip_param), DLT698_DAR_TEMPORARY_FAILURE);

    const uint8_t missing_field[] = {0x01, 0x01, 0x02, 0x03, 0x51, 0x20, 0x00,
                                     0x04, 0x00, 0x11, 0x05, 0x12, 0x00, 0x0A};
    assert_action_dar(129, missing_field, sizeof(missing_field), DLT698_DAR_TYPE_MISMATCH);

    const uint8_t extra_field[] = {0x01, 0x01, 0x02, 0x05, 0x51, 0x20, 0x00, 0x04, 0x00,
                                   0x11, 0x05, 0x12, 0x00, 0x0A, 0x03, 0x01, 0x11, 0x00};
    assert_action_dar(129, extra_field, sizeof(extra_field), DLT698_DAR_TYPE_MISMATCH);

    const uint8_t bad_oad_type[] = {0x01, 0x01, 0x02, 0x04, 0x12, 0x00, 0x04, 0x00,
                                    0x11, 0x05, 0x12, 0x00, 0x0A, 0x03, 0x01};
    assert_action_dar(129, bad_oad_type, sizeof(bad_oad_type), DLT698_DAR_TYPE_MISMATCH);

    const uint8_t empty_array[] = {0x01, 0x00};
    assert_action_dar(129, empty_array, sizeof(empty_array), DLT698_DAR_TYPE_MISMATCH);

    const uint8_t too_many[] = {0x01, 0x09};
    assert_action_dar(129, too_many, sizeof(too_many), DLT698_DAR_TYPE_MISMATCH);
}

/* 表 A.9：130 = array<structure{OAD, enum}>。 */
static const uint8_t k_close_param[] = {0x01, 0x01, 0x02, 0x02, 0x51, 0x20,
                                        0x00, 0x04, 0x00, 0x16, 0x01};

static void test_action_130_close_param(void **state) {
    (void)state;
    assert_action_dar(130, k_close_param, sizeof(k_close_param), DLT698_DAR_TEMPORARY_FAILURE);

    const uint8_t wrong_command[] = {0x01, 0x01, 0x02, 0x02, 0x51, 0x20,
                                     0x00, 0x04, 0x00, 0x11, 0x01};
    assert_action_dar(130, wrong_command, sizeof(wrong_command), DLT698_DAR_TYPE_MISMATCH);

    const uint8_t missing_command[] = {0x01, 0x01, 0x02, 0x01, 0x51, 0x20, 0x00, 0x04, 0x00};
    assert_action_dar(130, missing_command, sizeof(missing_command), DLT698_DAR_TYPE_MISMATCH);
}

/* 表 A.9：131 = array<structure{OAD, enum, visible-string}>。 */
static const uint8_t k_plain_close_param[] = {0x01, 0x01, 0x02, 0x03, 0x51, 0x20, 0x00,
                                              0x04, 0x00, 0x16, 0x00, 0x0A, 0x06, 0x31,
                                              0x32, 0x33, 0x34, 0x35, 0x36};

static void test_action_131_password_close_param(void **state) {
    (void)state;
    assert_action_dar(131, k_plain_close_param, sizeof(k_plain_close_param),
                      DLT698_DAR_TEMPORARY_FAILURE);

    const uint8_t missing_password[] = {0x01, 0x01, 0x02, 0x02, 0x51, 0x20,
                                        0x00, 0x04, 0x00, 0x16, 0x00};
    assert_action_dar(131, missing_password, sizeof(missing_password), DLT698_DAR_TYPE_MISMATCH);

    const uint8_t wrong_password_type[] = {0x01, 0x01, 0x02, 0x03, 0x51, 0x20, 0x00,
                                           0x04, 0x00, 0x16, 0x00, 0x09, 0x06, 0x31,
                                           0x32, 0x33, 0x34, 0x35, 0x36};
    assert_action_dar(131, wrong_password_type, sizeof(wrong_password_type),
                      DLT698_DAR_TYPE_MISMATCH);
}

static void test_action_rejects_other_methods_and_trailing_bytes(void **state) {
    (void)state;
    assert_action_dar(126, k_close_param, sizeof(k_close_param), DLT698_DAR_OBJECT_NOT_FOUND);

    const uint8_t trailing[] = {0x00, 0xFF, 0xFF};
    assert_action_dar(127, trailing, sizeof(trailing), DLT698_DAR_TYPE_MISMATCH);

    /* 带真实 TimeTag 的帧不做时效判定，必须回 DAR 7，不能静默忽略时标。 */
    const uint8_t ti_present[] = {0x00, 0x05};
    assert_action_dar(127, ti_present, sizeof(ti_present), DLT698_DAR_TYPE_MISMATCH);
}

static void test_action_non8000_class_defined_unexposed_dar5(void **state) {
    (void)state;
    const uint8_t null_param[] = {0x00};
    /* 5004 方法 3：Class 9 定义。 */
    assert_int_equal(run_action_dar_omd_acl(((uint32_t)0x5004 << 16) | (3u << 8), null_param,
                                            sizeof(null_param), routing_object_list),
                     5);
}

static void test_action_unknown_oi_dar6(void **state) {
    (void)state;
    const uint8_t null_param[] = {0x00};
    assert_int_equal(run_action_dar_omd_acl(((uint32_t)0x9999 << 16) | (1u << 8), null_param,
                                            sizeof(null_param), routing_object_list),
                     6);
}

static void test_action_non8000_reaches_handler(void **state) {
    (void)state;
    const uint8_t null_param[] = {0x00};
    assert_int_equal(run_action_dar_omd_acl(((uint32_t)OI_TASK_SCHEDULER << 16) | (1u << 8),
                                            null_param, sizeof(null_param), routing_object_list),
                     0);
}

/* 6012 方法 127 的 A2 单元，形状同 tests/test_dcu_698_tasks.c。 */
static const uint8_t k_add_task_7_60s[] = {
    0x01, 0x01,             /* array[1] */
    0x02, 0x0C,             /* structure[12] */
    0x11, 0x07,             /* 任务 ID */
    0x54, 0x00, 0x00, 0x3C, /* 周期 TI：0 单位 x 60 */
    0x16, 0x01, 0x11, 0x01, /* 方案类型 / 编号 */
    0x1C, 0,    0,    0,    0,    0,    0,    0,    0x1C, 0, 0, 0, 0, 0, 0, 0, /* 起止时间 */
    0x54, 0x00, 0x00, 0x00, 0x11, 0x00,             /* 延时 / 优先级 */
    0x16, 0x01, 0x12, 0x00, 0x00, 0x12, 0x00, 0x00, /* 状态 / 前后脚本 */
    0x02, 0x02, 0x16, 0x00, 0x01, 0x00,             /* 时段：空表 */
};

static void test_action_6012_method127_valid_body_dar0(void **state) {
    (void)state;
    g_apply_calls = 0;
    assert_int_equal(
        run_action_dar_omd_acl(((uint32_t)OI_TASK_SCHEDULER << 16) | (127u << 8), k_add_task_7_60s,
                               sizeof(k_add_task_7_60s), routing_object_list),
        DLT698_DAR_SUCCESS);
    assert_int_equal(g_apply_calls, 1);
    assert_int_equal(g_applied_op.method, 127);
    assert_int_equal(g_applied_op.units[0].task_id, 7);
    assert_int_equal(g_applied_op.units[0].interval_unit, 0);
    assert_int_equal(g_applied_op.units[0].interval_value, 60);
    dcu_698_object_model_reset();
}

static void test_action_6012_method127_truncated_body_dar7_no_state_change(void **state) {
    (void)state;
    init_product_model();
    assert_int_equal(dcu_poller_add_task(&g_poller, 1, 60, 1), 0);
    const dcu_task_unit_t *existing = dcu_poller_task_find(&g_poller, 1);
    assert_non_null(existing);
    dcu_task_unit_t before = *existing;
    uint16_t count_before = g_poller.task_count;
    g_apply_calls = 0;

    const uint8_t truncated[] = {0x01, 0x01, 0x02, 0x0C, 0x11, 0x07, 0x54, 0x00, 0x00, 0x3C};
    assert_int_equal(
        run_action_dar_omd_acl_initialized(((uint32_t)OI_TASK_SCHEDULER << 16) | (127u << 8),
                                           truncated, sizeof(truncated), routing_object_list),
        DLT698_DAR_TYPE_MISMATCH);
    assert_int_equal(g_apply_calls, 0);
    assert_int_equal(g_poller.task_count, count_before);
    assert_null(dcu_poller_task_find(&g_poller, 7));
    assert_memory_equal(existing, &before, sizeof(before));
    dcu_698_object_model_reset();
}

static void test_driver_high_byte_oad_has_no_signed_shift(void **state) {
    (void)state;
    uint8_t frame[128];

    size_t frame_len = build_get_normal(frame, 0xF2000200U);
    net_buf_pool_t *pool = NULL;
    net_buf_t *out = run_get_normal(frame, frame_len, &k_high_oad_src, &pool);
    assert_int_equal(out->b.data[9], DLT698_SVC_GET_RES);
    assert_int_equal(out->b.data[10], DLT698_CHOICE_GET_NORMAL);
    assert_int_equal(out->b.data[12], 0xF2);
    assert_int_equal(out->b.data[13], 0x00);
    assert_int_equal(out->b.data[14], 0x02);
    assert_int_equal(out->b.data[15], 0x00);
    assert_int_equal(out->b.data[16], 0x01);
    assert_int_equal(out->b.data[17], DLT698_TAG_DOUBLE_LONG);
    net_buf_unref(out);
    net_buf_pool_destroy(pool);

    const uint8_t value[] = {DLT698_TAG_DOUBLE_LONG, 0x00, 0x00, 0x25, 0x80};
    frame_len = build_set(frame, 0xF2000200U, value, sizeof(value), true);
    out = run_set(frame, frame_len, &k_set_src, &pool);
    assert_int_equal(out->b.data[9], DLT698_SVC_SET_RES);
    assert_int_equal(out->b.data[12], 0xF2);
    assert_int_equal(out->b.data[13], 0x00);
    assert_int_equal(out->b.data[14], 0x02);
    assert_int_equal(out->b.data[15], 0x00);
    assert_int_equal(out->b.data[16], DLT698_DAR_SUCCESS);
    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

/* 表 A.7：A2 结果数组、A3/A6 记录数、A10 空闲/搜表中。 */
static void test_discovery_standard_attribute_shapes(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    init_product_model();

    const uint8_t empty_array[] = {DLT698_TAG_ARRAY, 0x00};
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 2), empty_array, sizeof(empty_array));
    const uint8_t zero_count[] = {DLT698_TAG_LONG_UNSIGNED, 0x00, 0x00};
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 3), zero_count, sizeof(zero_count));
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 6), zero_count, sizeof(zero_count));
    const uint8_t idle[] = {DLT698_TAG_ENUM, DISCOVERY_STATUS_IDLE};
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 10), idle, sizeof(idle));

    assert_int_equal(dcu_discovery_start(&g_discovery, 1), 0);
    const uint8_t scanning[] = {DLT698_TAG_ENUM, DISCOVERY_STATUS_SCANNING};
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 10), scanning, sizeof(scanning));
    dcu_discovery_tick(&g_discovery, g_discovery.started_ms + 60000);
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 10), idle, sizeof(idle));
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 2), empty_array, sizeof(empty_array));
    assert_axdr(MK_OAD(OI_METER_DISCOVERY, 3), zero_count, sizeof(zero_count));

    /* 结果非空时缺 6003 的硬件字段：A2 回 DAR 2，不编造。 */
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    g_discovery.count = 1;
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_METER_DISCOVERY, 2), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_TEMPORARY);
    g_discovery.count = 0;

    /* 输出缓冲不足是暂时失效（DAR 2），不是帧类型错误。 */
    uint8_t tiny[1];
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_METER_DISCOVERY, 2), tiny, sizeof(tiny), &len),
        EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_METER_DISCOVERY, 3), tiny, sizeof(tiny), &len),
        EDGE_SLAVE_SRC_TEMPORARY);

    /* A5/A7/A8/A9 由表 A.7 定义但未暴露 -> DAR 5；A11 未定义 -> DAR 4。 */
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_METER_DISCOVERY, 5), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_METER_DISCOVERY, 8), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_METER_DISCOVERY, 11), buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_UNDEFINED);

    dcu_698_object_model_reset();
}

static void test_discovery_action_start_duplicate_and_clear(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    init_product_model();

    const uint8_t duration_1[] = {DLT698_TAG_LONG_UNSIGNED, 0x00, 0x01};
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(OI_METER_DISCOVERY, 127), duration_1,
                                             sizeof(duration_1)),
                     0);
    assert_int_equal(g_discovery.status, DISCOVERY_STATUS_SCANNING);

    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(OI_METER_DISCOVERY, 127), duration_1,
                                             sizeof(duration_1)),
                     EDGE_SLAVE_SRC_DENIED);

    /* 参数形状：unsigned 不是 long-unsigned -> DAR 7。 */
    const uint8_t wrong_tag[] = {DLT698_TAG_UNSIGNED, 0x01};
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(OI_METER_DISCOVERY, 127), wrong_tag,
                                             sizeof(wrong_tag)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* 128 参数是 NULL；给 long-unsigned -> DAR 7。 */
    const uint8_t null_param[] = {DLT698_TAG_NULL};
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(OI_METER_DISCOVERY, 128), null_param,
                                             sizeof(null_param)),
                     0);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(OI_METER_DISCOVERY, 128), duration_1,
                                             sizeof(duration_1)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(OI_METER_DISCOVERY, 129), NULL, 0),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    const uint8_t empty[1] = {0};
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(OI_METER_DISCOVERY, 129), empty, 0),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    dcu_698_object_model_reset();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_product_variable_axdr_types),
        cmocka_unit_test(test_discovery_standard_attribute_shapes),
        cmocka_unit_test(test_discovery_action_start_duplicate_and_clear),
        cmocka_unit_test(test_event_class24_attrs_on_the_wire),
        cmocka_unit_test(test_event_class24_complex_attrs),
        cmocka_unit_test(test_event_class24_record_and_status_times),
        cmocka_unit_test(test_product_axdr_rejects_small_buffer_and_complex),
        cmocka_unit_test(test_driver_scalar_fallback_still_works),
        cmocka_unit_test(test_driver_high_byte_oad_has_no_signed_shift),
        cmocka_unit_test(test_driver_carries_structure_axdr),
        cmocka_unit_test(test_driver_rejects_axdr_buffer_shortage),
        cmocka_unit_test(test_driver_normal_list_uses_axdr_per_item),
        cmocka_unit_test(test_driver_normal_list_mixed_success_and_dar),
        cmocka_unit_test(test_driver_normal_list_empty_request),
        cmocka_unit_test(test_driver_normal_list_duplicate_oad_each_answered),
        cmocka_unit_test(test_driver_normal_list_reuses_one_buffer),
        cmocka_unit_test(test_get_normal_dar_mapping),
        cmocka_unit_test(test_get_normal_encode_type_mismatch_returns_dar),
        cmocka_unit_test(test_normal_list_dar_mapping),
        cmocka_unit_test(test_product_accept_set_paths),
        cmocka_unit_test(test_product_class8_datetime_round_trip),
        cmocka_unit_test(test_product_class2_and_5_have_no_value_yet),
        cmocka_unit_test(test_product_accept_set_event_attrs),
        cmocka_unit_test(test_set_response_dar_paths),
        cmocka_unit_test(test_set_missing_data_dar7),
        cmocka_unit_test(test_set_scalar_fallback),
        cmocka_unit_test(test_set_scalar_fallback_rejects_structure),
        cmocka_unit_test(test_action_127_128_null_param),
        cmocka_unit_test(test_action_129_trip_param),
        cmocka_unit_test(test_action_130_close_param),
        cmocka_unit_test(test_action_131_password_close_param),
        cmocka_unit_test(test_action_rejects_other_methods_and_trailing_bytes),
        cmocka_unit_test(test_action_non8000_class_defined_unexposed_dar5),
        cmocka_unit_test(test_action_unknown_oi_dar6),
        cmocka_unit_test(test_action_non8000_reaches_handler),
        cmocka_unit_test(test_action_6012_method127_valid_body_dar0),
        cmocka_unit_test(test_action_6012_method127_truncated_body_dar7_no_state_change),
    };
    return cmocka_run_group_tests(tests, setup_timezone, teardown_timezone);
}
