/**
 * @file test_dcu_698_wire_vectors.c
 * @brief DCU-D6-02：DL/T 698.45 真帧向量（GET/SET/ACTION/GetRecord/REPORT/RSD/DAR）
 *
 * 每条向量的请求 APDU 与期望应答都取自 `docs/698_parsed.md` 的标准原文（行号见
 * 每个用例的注释），不先跑实现再抄回期望值。帧头/帧尾（68 | L | C | SA | CA |
 * HCS | APDU | FCS | 16）由驱动生成；这里只断言 APDU 内容与 DAR。
 *
 * 入口是真实产品路径：`dlt698_driver.req_slave_exchange` + 对象模型回调
 * （`dcu_north_resolve_axdr` / `dcu_north_accept_axdr` / `dcu_north_accept_action`），
 * 不复制帧解析逻辑。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "dcu_698_appconn.h"
#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_param.h"
#include "dcu_clock_port.h"
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

/* 请求帧：68 | L(2) | 43 | SA描述 | SA | CA | HCS(2) | APDU | FCS(2) | 16
 * 所以 APDU 起点 = 9。 */
#define APDU_OFF 9
#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))

/* ---- 对象模型装配：真实产品回调 + 可控时钟/参数存储 ---- */

static dcu_inventory_t g_inv;
static dcu_poller_t g_poller;
static int g_task_apply_calls;

static int test_task_apply(void *ctx, dcu_poller_t *poller, const dcu_task_op_t *op) {
    (void)ctx;
    (void)poller;
    (void)op;
    g_task_apply_calls++;
    return 0;
}

static struct tm g_clock_value = {
    .tm_year = 124, .tm_mon = 4, .tm_mday = 6, .tm_hour = 7, .tm_min = 8, .tm_sec = 9};

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
    return 1; /* 未落盘：保留默认值 */
}

static int setup_model(void **state) {
    (void)state;
    dcu_relay_map_bind(NULL);
    dcu_comm_info_reset();
    assert_int_equal(dcu_inventory_init(&g_inv), 0);
    assert_int_equal(dcu_poller_init(&g_poller), 0);
    g_task_apply_calls = 0;

    router_sink_ops_t sink = {.task_apply = test_task_apply};
    router_bus_ops_t bus = {0};
    assert_int_equal(
        dcu_698_object_model_init(&g_inv, &g_poller, sink, NULL, NULL, NULL, NULL, bus), 0);

    static const dcu_clock_port_t clock = {.now = fake_clock_now, .set = fake_clock_set};
    dcu_698_param_set_clock(&clock);
    dcu_param_store_t store = {.save = fake_param_save, .load = fake_param_load};
    dcu_698_param_init(&store);
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

/* 默认对象列表（真实产品 ACL）。 */
static int product_object_list(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    (void)oad;
    return dcu_698_appconn_object_list(buf, cap, len) == 0 ? 0 : EDGE_SLAVE_SRC_TEMPORARY;
}

static const edge_slave_source_ops_t k_product_src = {
    .object_list = product_object_list,
    .resolve_axdr = dcu_north_resolve_axdr,
    .accept_axdr = dcu_north_accept_axdr,
    .accept_action = dcu_north_accept_action,
};

/* ---- 帧构造 / 交换 ---- */

static size_t build_frame(uint8_t *out, const uint8_t *apdu, size_t apdu_len) {
    size_t n = 0;
    out[n++] = DLT698_HEAD;
    size_t len_off = n;
    n += 2;
    out[n++] = 0x43;
    out[n++] = 0x00; /* SA：type 0，len 1 */
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

/* 交换一帧；返回驱动 rc，响应长度写 *out_len（0 表示没有应答）。 */
static int exchange(void *session, const edge_slave_source_ops_t *src, void *src_ctx,
                    const uint8_t *apdu, size_t apdu_len, uint8_t *backing, size_t cap,
                    size_t *out_len) {
    uint8_t frame[1024];
    assert_true(apdu_len + 16 <= sizeof(frame));
    size_t flen = build_frame(frame, apdu, apdu_len);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    net_buf_t out;
    memset(&out, 0, sizeof(out));
    net_buf_simple_init_with_data(&out.b, backing, cap);
    size_t consumed = 0;
    int rc =
        dlt698_driver.req_slave_exchange(priv, session, frame, flen, src, src_ctx, &out, &consumed);
    if (out_len) *out_len = out.b.len;
    return rc;
}

/* 断言应答 APDU 逐字节相等（帧头由驱动生成，只核 APDU + 帧长）。 */
static void assert_response(const uint8_t *backing, size_t out_len, const uint8_t *want,
                            size_t want_len) {
    assert_int_equal(out_len, (size_t)APDU_OFF + want_len + 3);
    assert_memory_equal(backing + APDU_OFF, want, want_len);
}

/* ---- 1. GET Normal 单项：H.4.2 设置通信地址 → H.3.1 读取通信地址 ---- */

static void test_get_normal_single(void **state) {
    (void)state;
    uint8_t backing[512];
    size_t out_len = 0;
    static uint8_t session[4096];

    /* H.4.2（docs/698_parsed.md:10545）SET 4001 A2 = octet-string 123456789012。
     * 表79（:2824）SetRequestNormal = PIID | OAD | Data | 时间标签。 */
    static const uint8_t set_addr[] = {0x06, 0x01, 0x02, 0x40, 0x01, 0x02, 0x00, 0x09,
                                       0x06, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x00};
    memset(session, 0, sizeof(session));
    assert_int_equal(exchange(session, &k_product_src, NULL, set_addr, sizeof(set_addr), backing,
                              sizeof(backing), &out_len),
                     0);
    static const uint8_t set_resp[] = {0x86, 0x01, 0x02, 0x40, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00};
    assert_response(backing, out_len, set_resp, sizeof(set_resp));

    /* H.3.1（:10204）GET 4001 A2；响应 :10210 = 85 01 01 40010200 01 09 06 12..12 00 00。
     * 表65（:2560）/ 表72（:2676）。 */
    static const uint8_t get_addr[] = {0x05, 0x01, 0x01, 0x40, 0x01, 0x02, 0x00, 0x00};
    assert_int_equal(exchange(session, &k_product_src, NULL, get_addr, sizeof(get_addr), backing,
                              sizeof(backing), &out_len),
                     0);
    static const uint8_t get_resp[] = {0x85, 0x01, 0x01, 0x40, 0x01, 0x02, 0x00, 0x01, 0x09,
                                       0x06, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x00, 0x00};
    assert_response(backing, out_len, get_resp, sizeof(get_resp));
}

/* 反例：截断的 GET Normal（缺 OAD 末字节）。表65 要求 OAD 完整，报文应被丢弃。 */
static void test_get_normal_truncated_is_dropped(void **state) {
    (void)state;
    uint8_t backing[512];
    size_t out_len = 123;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    static const uint8_t truncated[] = {0x05, 0x01, 0x01, 0x40, 0x01};
    assert_int_equal(exchange(session, &k_product_src, NULL, truncated, sizeof(truncated), backing,
                              sizeof(backing), &out_len),
                     -1);
    assert_int_equal(out_len, 0);
}

/* ---- 2. GET NormalList：逐项 DAR（表73 :2700） ---- */

static void test_get_normal_list_per_item_dar(void **state) {
    (void)state;
    /* 以太网口 MAC（4510 A5）回数据；F200 A3 按类定义存在而未实现 → DAR 5。
     * 表73：A-ResultNormal = OAD | Get-Result（[1] Data 或 [0] DAR）。
     * DAR 5 = 对象接口类不符合，表34（:1935）。 */
    dcu_comm_info_t info = {.mac_present = true, .mac = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55}};
    dcu_comm_info_install(&info);

    static const uint8_t get_list[] = {0x05, 0x02, 0x01, 0x02, 0x45, 0x10, 0x05,
                                       0x00, 0xF2, 0x00, 0x03, 0x00, 0x00};
    uint8_t backing[512];
    size_t out_len = 0;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    assert_int_equal(exchange(session, &k_product_src, NULL, get_list, sizeof(get_list), backing,
                              sizeof(backing), &out_len),
                     0);
    static const uint8_t want[] = {
        0x85, 0x02, 0x01, 0x02, 0x45, 0x10, 0x05, 0x00, 0x01, 0x09, 0x06, 0x02, 0x11,
        0x22, 0x33, 0x44, 0x55, 0xF2, 0x00, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00,
    };
    assert_response(backing, out_len, want, sizeof(want));
}

/* 反例：个数=2 却只带 1 个 OAD。长度必须精确匹配（表66 :2575），错位解会得到错值。 */
static void test_get_normal_list_count_mismatch_is_dropped(void **state) {
    (void)state;
    uint8_t backing[512];
    size_t out_len = 123;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    static const uint8_t bad[] = {0x05, 0x02, 0x01, 0x02, 0x45, 0x10, 0x05, 0x00, 0x00};
    assert_int_equal(exchange(session, &k_product_src, NULL, bad, sizeof(bad), backing,
                              sizeof(backing), &out_len),
                     -1);
    assert_int_equal(out_len, 0);
}

/* ---- 3. SET：成功（H.4.1 :10525）与类型错误 ---- */

static void test_set_success_and_type_error(void **state) {
    (void)state;
    uint8_t backing[512];
    size_t out_len = 0;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));

    /* H.4.1：SET 4000 A2（时钟）= date_time_s 2016-01-20 16:27:11。
     * 响应 :10533 = 86 01 02 40000200 00 00 00（表83 :2892）。 */
    static const uint8_t set_clock[] = {0x06, 0x01, 0x02, 0x40, 0x00, 0x02, 0x00, 0x1C,
                                        0x07, 0xE0, 0x01, 0x14, 0x10, 0x1B, 0x0B, 0x00};
    assert_int_equal(exchange(session, &k_product_src, NULL, set_clock, sizeof(set_clock), backing,
                              sizeof(backing), &out_len),
                     0);
    static const uint8_t set_resp[] = {0x86, 0x01, 0x02, 0x40, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00};
    assert_response(backing, out_len, set_resp, sizeof(set_resp));

    /* 反例：同一时钟属性用 long-unsigned 代替 date_time_s。类型不匹配 → DAR 7（表34 :1937）。 */
    static const uint8_t wrong_type[] = {0x06, 0x01, 0x03, 0x40, 0x00, 0x02,
                                         0x00, 0x12, 0x00, 0x01, 0x00};
    assert_int_equal(exchange(session, &k_product_src, NULL, wrong_type, sizeof(wrong_type),
                              backing, sizeof(backing), &out_len),
                     0);
    static const uint8_t wrong_resp[] = {0x86, 0x01, 0x03, 0x40, 0x00,
                                         0x02, 0x00, 0x07, 0x00, 0x00};
    assert_response(backing, out_len, wrong_resp, sizeof(wrong_resp));
}

/* ---- 3b. DAR 类别：未定义属性 4 / 类不符合 5 / 对象不存在 6（表34 :1934-1936） ---- */

/* 只广告下面三条 OAD，让请求越过 ACL 到达真实 resolver，分别产生 DAR 4/5/6。
 * 产品默认对象列表不会广告“未定义属性/未知 OI”，所以这里用测试 ACL。 */
static int dar_object_list(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    (void)oad;
    static const uint8_t acl[] = {
        0x01,
        0x03,
        /* OI 0010 属性 6 只读（类 1 只定义到 A5 → resolver 回 UNDEFINED） */
        0x02,
        0x02,
        0x50,
        0x00,
        0x10,
        0x02,
        0x02,
        0x01,
        0x01,
        0x02,
        0x02,
        0x11,
        0x06,
        0x16,
        0x01,
        0x01,
        0x00,
        /* OI 1234 属性 2 只读（对象表里没有该 OI → resolver 回 NOT_FOUND） */
        0x02,
        0x02,
        0x50,
        0x12,
        0x34,
        0x02,
        0x02,
        0x01,
        0x01,
        0x02,
        0x02,
        0x11,
        0x02,
        0x16,
        0x01,
        0x01,
        0x00,
        /* OI F200 属性 3 只读（类 22 定义到 A4 但实现未接线 → resolver 回 CLASS_MISMATCH） */
        0x02,
        0x02,
        0x50,
        0xF2,
        0x00,
        0x02,
        0x02,
        0x01,
        0x01,
        0x02,
        0x02,
        0x11,
        0x03,
        0x16,
        0x01,
        0x01,
        0x00,
    };
    if (sizeof(acl) > cap) return -1;
    memcpy(buf, acl, sizeof(acl));
    *len = sizeof(acl);
    return 0;
}

static const edge_slave_source_ops_t k_dar_src = {
    .object_list = dar_object_list,
    .resolve_axdr = dcu_north_resolve_axdr,
};

static void dar_case(const uint8_t *apdu, size_t apdu_len, uint8_t oi_hi, uint8_t oi_lo,
                     uint8_t attr, uint8_t dar) {
    uint8_t backing[512];
    size_t out_len = 0;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    assert_int_equal(
        exchange(session, &k_dar_src, NULL, apdu, apdu_len, backing, sizeof(backing), &out_len), 0);
    static const uint8_t head[] = {0x85, 0x01, 0x01};
    static const uint8_t tail[] = {0x00, 0x00};
    uint8_t want[11];
    size_t w = 0;
    memcpy(want + w, head, sizeof(head));
    w += sizeof(head);
    want[w++] = oi_hi;
    want[w++] = oi_lo;
    want[w++] = attr;
    want[w++] = 0x00;
    want[w++] = 0x00; /* CHOICE [0] DAR */
    want[w++] = dar;
    memcpy(want + w, tail, sizeof(tail));
    w += sizeof(tail);
    assert_response(backing, out_len, want, w);
}

static void test_dar_classes_undefined_class_notfound(void **state) {
    (void)state;
    static const uint8_t get_0010_a6[] = {0x05, 0x01, 0x01, 0x00, 0x10, 0x06, 0x00, 0x00};
    dar_case(get_0010_a6, sizeof(get_0010_a6), 0x00, 0x10, 0x06, DLT698_DAR_OBJECT_UNDEFINED);
    static const uint8_t get_1234_a2[] = {0x05, 0x01, 0x01, 0x12, 0x34, 0x02, 0x00, 0x00};
    dar_case(get_1234_a2, sizeof(get_1234_a2), 0x12, 0x34, 0x02, DLT698_DAR_OBJECT_NOT_FOUND);
    static const uint8_t get_f200_a3[] = {0x05, 0x01, 0x01, 0xF2, 0x00, 0x03, 0x00, 0x00};
    dar_case(get_f200_a3, sizeof(get_f200_a3), 0xF2, 0x00, 0x03, DLT698_DAR_OBJECT_CLASS_MISMATCH);
}

/* 反例：同一 ACL 下未广告的属性（0010 A5）必须在 ACL 层被拒绝，回 DAR 3，
 * 证明上面的 DAR 4 是 resolver 判定而不是 ACL 放行后又恰好拒绝。表34 :1933。 */
static void test_dar_unadvertised_attr_denied(void **state) {
    (void)state;
    static const uint8_t get_0010_a5[] = {0x05, 0x01, 0x01, 0x00, 0x10, 0x05, 0x00, 0x00};
    uint8_t backing[512];
    size_t out_len = 0;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    assert_int_equal(exchange(session, &k_dar_src, NULL, get_0010_a5, sizeof(get_0010_a5), backing,
                              sizeof(backing), &out_len),
                     0);
    static const uint8_t want[] = {0x85, 0x01, 0x01, 0x00, 0x10, 0x05,
                                   0x00, 0x00, 0x03, 0x00, 0x00};
    assert_response(backing, out_len, want, sizeof(want));
}

/* ---- 4. ACTION：6012 方法 127 与 8000 方法 129 ---- */

/* 6012 方法 127 的 A2 单元（形状同 tests/test_dcu_698_tasks.c）。 */
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

/* 表 A.9：8000 方法 129 参数 = array<structure{OAD, unsigned, long-unsigned, bool}>。 */
static const uint8_t k_trip_param[] = {0x01, 0x01, 0x02, 0x04, 0x51, 0x20, 0x00, 0x04,
                                       0x00, 0x11, 0x05, 0x12, 0x00, 0x0A, 0x03, 0x01};

static void test_action_6012_and_8000(void **state) {
    (void)state;
    uint8_t backing[512];
    size_t out_len = 0;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));

    /* 6012 方法 127 成功：表87 :2970 ActionRequest = PIID | OMD | Data | 时间标签；
     * 响应表91 :3047 = 87 | 01 | PIID | OMD(4) | DAR | Data标志 | FollowReport | 时间标签。
     * 与 H.5.1（:10598）同形，DAR=0（表34 :1930）。 */
    uint8_t action_6012[8 + sizeof(k_add_task_7_60s) + 1];
    size_t n = 0;
    action_6012[n++] = 0x07;
    action_6012[n++] = 0x01;
    action_6012[n++] = 0x01;
    action_6012[n++] = 0x60;
    action_6012[n++] = 0x12;
    action_6012[n++] = 0x7F; /* 方法 127 */
    action_6012[n++] = 0x00; /* 操作模式 */
    memcpy(action_6012 + n, k_add_task_7_60s, sizeof(k_add_task_7_60s));
    n += sizeof(k_add_task_7_60s);
    action_6012[n++] = DLT698_NO_TIME_TAG;

    assert_int_equal(
        exchange(session, &k_product_src, NULL, action_6012, n, backing, sizeof(backing), &out_len),
        0);
    static const uint8_t action_resp[] = {0x87, 0x01, 0x01, 0x60, 0x12, 0x7F,
                                          0x00, 0x00, 0x00, 0x00, 0x00};
    assert_response(backing, out_len, action_resp, sizeof(action_resp));
    assert_true(g_task_apply_calls > 0);

    /* 8000 方法 129：空继电器映射 → 诚实回暂时失效 DAR 2（表34 :1932）。 */
    uint8_t action_8000[8 + sizeof(k_trip_param) + 1];
    n = 0;
    action_8000[n++] = 0x07;
    action_8000[n++] = 0x01;
    action_8000[n++] = 0x02;
    action_8000[n++] = 0x80;
    action_8000[n++] = 0x00;
    action_8000[n++] = 0x81; /* 方法 129 */
    action_8000[n++] = 0x00;
    memcpy(action_8000 + n, k_trip_param, sizeof(k_trip_param));
    n += sizeof(k_trip_param);
    action_8000[n++] = DLT698_NO_TIME_TAG;

    assert_int_equal(
        exchange(session, &k_product_src, NULL, action_8000, n, backing, sizeof(backing), &out_len),
        0);
    static const uint8_t action_8000_resp[] = {0x87, 0x01, 0x02, 0x80, 0x00, 0x81,
                                               0x00, 0x02, 0x00, 0x00, 0x00};
    assert_response(backing, out_len, action_8000_resp, sizeof(action_8000_resp));
}

/* 反例：6012 方法 127 参数截断。形状错误在触碰状态前回 DAR 7，且不应用任务。 */
static void test_action_6012_truncated_dar7(void **state) {
    (void)state;
    uint8_t backing[512];
    size_t out_len = 0;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));

    static const uint8_t truncated[] = {0x01, 0x01, 0x02, 0x0C, 0x11, 0x07, 0x54, 0x00, 0x00, 0x3C};
    uint8_t apdu[8 + sizeof(truncated) + 1];
    size_t n = 0;
    apdu[n++] = 0x07;
    apdu[n++] = 0x01;
    apdu[n++] = 0x01;
    apdu[n++] = 0x60;
    apdu[n++] = 0x12;
    apdu[n++] = 0x7F;
    apdu[n++] = 0x00;
    memcpy(apdu + n, truncated, sizeof(truncated));
    n += sizeof(truncated);
    apdu[n++] = DLT698_NO_TIME_TAG;

    int before = g_task_apply_calls;
    assert_int_equal(
        exchange(session, &k_product_src, NULL, apdu, n, backing, sizeof(backing), &out_len), 0);
    static const uint8_t want[] = {0x87, 0x01, 0x01, 0x60, 0x12, 0x7F,
                                   0x00, 0x07, 0x00, 0x00, 0x00};
    assert_response(backing, out_len, want, sizeof(want));
    assert_int_equal(g_task_apply_calls, before);
}

/* ---- 5. GetRecord：Selector1 / Selector2（表67 :2588、表74、表30 :1792） ---- */

/* 假记录源：只返回一行，行字节是标准 Data 编码的 double-long-unsigned 1。 */
static int fake_record_begin(void *ctx, uint32_t oad, const uint8_t *sel, size_t sel_len,
                             size_t *total) {
    (void)ctx;
    (void)oad;
    assert_true(sel_len > 0);
    /* 选择方法必须是 RSD [1] 或 [2]（表30 :1795）。 */
    assert_true(sel[0] == DLT698_RSD_SELECTOR1 || sel[0] == DLT698_RSD_SELECTOR2);
    *total = 1;
    return 0;
}

static int fake_record_row(void *ctx, uint32_t oad, size_t index, uint8_t *row, size_t cap,
                           size_t *row_len) {
    (void)ctx;
    (void)oad;
    assert_int_equal(index, 0);
    static const uint8_t value[] = {DLT698_TAG_DOUBLE_LONG_UNSIGNED, 0x00, 0x00, 0x00, 0x01};
    assert_true(cap >= sizeof(value));
    memcpy(row, value, sizeof(value));
    *row_len = sizeof(value);
    return 0;
}

static int fake_record_end(void *ctx, uint32_t oad) {
    (void)ctx;
    (void)oad;
    return 0;
}

static void get_record_case(size_t selector, const uint8_t *sel_tail, size_t sel_tail_len,
                            uint8_t piid, const uint8_t *want, size_t want_len) {
    static const edge_slave_source_ops_t src = {
        .object_list = product_object_list,
        .record = {.begin = fake_record_begin, .row = fake_record_row, .end = fake_record_end},
    };
    uint8_t apdu[64];
    size_t n = 0;
    apdu[n++] = DLT698_SVC_GET_REQ;
    apdu[n++] = DLT698_CHOICE_GET_RECORD;
    apdu[n++] = piid;
    apdu[n++] = 0x50;
    apdu[n++] = 0x04;
    apdu[n++] = 0x02;
    apdu[n++] = 0x00; /* OAD 5004 A2 = 日冻结 */
    apdu[n++] = (uint8_t)selector;
    memcpy(apdu + n, sel_tail, sel_tail_len);
    n += sel_tail_len;
    apdu[n++] = DLT698_NO_TIME_TAG;

    uint8_t backing[512];
    size_t out_len = 0;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    assert_int_equal(exchange(session, &src, NULL, apdu, n, backing, sizeof(backing), &out_len), 0);
    assert_response(backing, out_len, want, want_len);
}

static void test_get_record_selector1_and_2(void **state) {
    (void)state;
    /* Selector1（表30 :1809）= OAD(20210200 冻结时间) + date_time_s。 */
    static const uint8_t sel1[] = {0x20, 0x21, 0x02, 0x00, 0x1C, 0x07, 0xE0,
                                   0x01, 0x14, 0x00, 0x00, 0x00, 0x01, /* RCSD 个数 = 1 */
                                   0x00, 0x00, 0x10, 0x02, 0x00};      /* CSD[0] OAD 0010 A2 */
    static const uint8_t want1[] = {
        0x85, 0x03, 0x01, 0x50, 0x04, 0x02, 0x00, /* svc | [3]Record | PIID | OAD */
        0x01, 0x00, 0x00, 0x10, 0x02, 0x00,       /* RCSD */
        0x01, 0x01,                               /* 记录数据 | M=1 */
        0x06, 0x00, 0x00, 0x00, 0x01,             /* 行：double-long-unsigned 1 */
        0x00, 0x00,
    };
    get_record_case(DLT698_RSD_SELECTOR1, sel1, sizeof(sel1), 0x01, want1, sizeof(want1));

    /* Selector2（表30 :1817）= OAD + 起始值 + 结束值 + 数据间隔（NULL）。区间前闭后开。 */
    static const uint8_t sel2[] = {0x20, 0x21, 0x02, 0x00, 0x1C, 0x07, 0xE0, 0x01,
                                   0x14, 0x00, 0x00, 0x00,                         /* 起始 */
                                   0x1C, 0x07, 0xE0, 0x01, 0x15, 0x00, 0x00, 0x00, /* 结束 */
                                   0x00, /* 间隔 = NULL */
                                   0x01, 0x00, 0x00, 0x10, 0x02, 0x00};
    static const uint8_t want2[] = {
        0x85, 0x03, 0x02, 0x50, 0x04, 0x02, 0x00, 0x01, 0x00, 0x00, 0x10,
        0x02, 0x00, 0x01, 0x01, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    };
    get_record_case(DLT698_RSD_SELECTOR2, sel2, sizeof(sel2), 0x02, want2, sizeof(want2));
}

/* 反例：Selector1 缺时间值与 RCSD。表67 要求完整结构，报文应被丢弃。 */
static void test_get_record_truncated_is_dropped(void **state) {
    (void)state;
    static const edge_slave_source_ops_t src = {
        .object_list = product_object_list,
        .record = {.begin = fake_record_begin, .row = fake_record_row, .end = fake_record_end},
    };
    static const uint8_t apdu[] = {0x05, 0x03, 0x01, 0x50, 0x04, 0x02,
                                   0x00, 0x01, 0x20, 0x21, 0x02, 0x00};
    uint8_t backing[512];
    size_t out_len = 123;
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    assert_int_equal(
        exchange(session, &src, NULL, apdu, sizeof(apdu), backing, sizeof(backing), &out_len), -1);
    assert_int_equal(out_len, 0);
}

/* ---- 6. REPORT：帧布局（表94/95）与确认匹配（表98/99） ---- */

static uint8_t g_report_body[64];
static size_t g_report_body_len;

static int fake_report_next(void *ctx, uint8_t *body, size_t cap, size_t *len) {
    (void)ctx;
    if (g_report_body_len > cap) return -1;
    memcpy(body, g_report_body, g_report_body_len);
    *len = g_report_body_len;
    return 0;
}

static int report_object_list(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    (void)oad;
    /* OI 0x3000 的极简对象列表：无属性、无方法（只要 ACL 能装载）。 */
    static const uint8_t acl[] = {0x01, 0x01, 0x02, 0x02, 0x50, 0x30, 0x00,
                                  0x02, 0x02, 0x01, 0x00, 0x01, 0x00};
    if (sizeof(acl) > cap) return -1;
    memcpy(buf, acl, sizeof(acl));
    *len = sizeof(acl);
    return 0;
}

/* CONNECT-Request（表59）：公共连接，版本 0x0010、window>0、apdu_max>0。 */
static size_t build_connect_apdu(uint8_t *apdu, uint8_t piid) {
    size_t o = 0;
    apdu[o++] = 0x02;
    apdu[o++] = piid;
    apdu[o++] = 0x00;
    apdu[o++] = 0x10;
    for (int i = 0; i < 8; i++) apdu[o++] = 0;
    for (int i = 0; i < 16; i++) apdu[o++] = 0;
    apdu[o++] = 0x00;
    apdu[o++] = 0x00;
    apdu[o++] = 0x00;
    apdu[o++] = 0x00;
    apdu[o++] = 0x01; /* window */
    apdu[o++] = 0x00;
    apdu[o++] = 0x05; /* apdu max */
    for (int i = 0; i < 4; i++) apdu[o++] = 0;
    apdu[o++] = 0x00; /* mechanism */
    return o;
}

static uint8_t g_report_session[4096];

static void report_confirm_case(uint8_t response_piid, bool expect_resent) {
    memset(g_report_session, 0, sizeof(g_report_session));
    edge_slave_source_ops_t src = {.object_list = report_object_list,
                                   .report = {.next = fake_report_next, .ctx = NULL}};

    uint8_t apdu[64];
    size_t alen = build_connect_apdu(apdu, 0x01);
    uint8_t backing[512];
    size_t out_len = 0;
    assert_int_equal(
        exchange(g_report_session, &src, NULL, apdu, alen, backing, sizeof(backing), &out_len), 0);
    assert_true(out_len > 0);

    /* 上报体：CHOICE[1] | PIID 0x07 | 1 项 | OAD 30000400 | [1] Data long-unsigned 1。 */
    static const uint8_t body[] = {
        DLT698_REPORT_LIST, 0x07, 0x01, 0x30, 0x00, 0x04, 0x00, 0x01, 0x12, 0x00, 0x01};
    memcpy(g_report_body, body, sizeof(body));
    g_report_body_len = sizeof(body);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    net_buf_t *tx1 = NULL;
    assert_int_equal(dlt698_driver.req_tick(g_report_session, NULL, NULL, &src, NULL, &pool, &tx1),
                     0);
    assert_non_null(tx1);

    /* 未确认前不重发（ADR-0017）。 */
    net_buf_t *tx2 = NULL;
    assert_int_equal(dlt698_driver.req_tick(g_report_session, NULL, NULL, &src, NULL, &pool, &tx2),
                     0);
    assert_null(tx2);

    /* 主站回 REPORT-Response（表99 :3183）：CHOICE | PIID | OAD 个数。 */
    static const uint8_t resp_tail[] = {0x00};
    uint8_t resp[8];
    size_t rn = 0;
    resp[rn++] = DLT698_SVC_REPORT_RES;
    resp[rn++] = DLT698_REPORT_LIST;
    resp[rn++] = response_piid;
    memcpy(resp + rn, resp_tail, sizeof(resp_tail));
    rn += sizeof(resp_tail);
    assert_int_equal(
        exchange(g_report_session, &src, NULL, resp, rn, backing, sizeof(backing), &out_len), 0);
    assert_int_equal(out_len, 0); /* 确认本身不产生应答帧 */

    net_buf_t *tx3 = NULL;
    assert_int_equal(dlt698_driver.req_tick(g_report_session, NULL, NULL, &src, NULL, &pool, &tx3),
                     0);
    if (expect_resent) {
        assert_non_null(tx3);
    } else {
        assert_null(tx3);
    }

    net_buf_unref(tx1);
    if (tx3) net_buf_unref(tx3);
    net_buf_pool_destroy(&pool);
}

static void test_report_frame_layout(void **state) {
    (void)state;
    /* 表95 :3124 ReportNotificationList = PIID-ACD | SEQUENCE OF A-ResultNormal；
     * 帧格式见 5.1（68 | L | C3 | SA | CA | HCS | 08 | body | 00 | FCS | 16）。 */
    static const uint8_t body[] = {
        DLT698_REPORT_LIST, 0x07, 0x01, 0x30, 0x00, 0x04, 0x00, 0x01, 0x12, 0x00, 0x01};
    uint8_t raw_body[64];
    size_t blen = 0;
    dlt698_get_item_t item = {
        .oad = 0x30000400u,
        .dar = 0,
        .data = (const uint8_t[]){DLT698_TAG_LONG_UNSIGNED, 0x00, 0x01},
        .dlen = 3,
    };
    assert_int_equal(dlt698_encode_report_notification(raw_body, sizeof(raw_body),
                                                       DLT698_REPORT_LIST, 0x07, &item, 1, &blen),
                     0);
    assert_memory_equal(raw_body, body, sizeof(body));
    assert_int_equal(blen, sizeof(body));

    net_buf_t out;
    memset(&out, 0, sizeof(out));
    uint8_t backing[256];
    net_buf_simple_init_with_data(&out.b, backing, sizeof(backing));
    dlt698_addr_t sa = {.len = 1, .type = 0};
    sa.addr[0] = 0x01;
    assert_int_equal(dlt698_encode_report_frame(&sa, raw_body, blen, &out), 0);

    /* APDU = 08 | body | 00（时间标签）。 */
    static const uint8_t want[] = {0x08, 0x01, 0x07, 0x01, 0x30, 0x00, 0x04,
                                   0x00, 0x01, 0x12, 0x00, 0x01, 0x00};
    assert_int_equal(out.b.len, (size_t)APDU_OFF + sizeof(want) + 3);
    assert_memory_equal(out.b.data + APDU_OFF, want, sizeof(want));
}

static void test_report_confirm_matching(void **state) {
    (void)state;
    report_confirm_case(0x06, false); /* PIID 不匹配：忽略，不解除 pending */
    report_confirm_case(0x07, true);  /* PIID 匹配：解除 pending，下一帧可发 */
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_get_normal_single, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_get_normal_truncated_is_dropped, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_get_normal_list_per_item_dar, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_get_normal_list_count_mismatch_is_dropped, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_set_success_and_type_error, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_dar_classes_undefined_class_notfound, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_dar_unadvertised_attr_denied, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_action_6012_and_8000, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_action_6012_truncated_dar7, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_get_record_selector1_and_2, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_get_record_truncated_is_dropped, setup_model,
                                        teardown_model),
        cmocka_unit_test_setup_teardown(test_report_frame_layout, setup_model, teardown_model),
        cmocka_unit_test_setup_teardown(test_report_confirm_matching, setup_model, teardown_model),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
