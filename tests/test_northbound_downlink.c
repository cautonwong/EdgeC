/**
 * @file test_northbound_downlink.c
 * @brief 北向下行：解析 + 编排层只送字节（ADR-0011 N6-2）
 *
 * ## 两条要锁的线
 *
 * 1. **解析归格式所有者**：`mqtt_json_decode_downlink` 必须严格——
 *    缺字段、嵌套、转义一律判错，绝不返回"半解析"的结果。
 *    （半解析比失败更难查：值对了名字错了，或者反过来。）
 *
 * 2. **编排层只送字节**：`northbound_mgr` 收到下行后原样转给产品的
 *    `on_downlink`，地址和载荷都不动。产品返回非 0 时编排层只记日志，
 *    不重试、不猜测。
 */

#include <cmocka.h>
#include "thread_port_uv.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "edge_north_plane.h"
#include "mqtt_json_codec.h"
#include "northbound_mgr.h"
#include "north_driver_test_port.h"

/* ---- 1. 解析 ---- */

static void test_decode_valid(void** state) {
    (void)state;
    const char* body = "{\"p\":\"VOLTAGE_A\",\"v\":23.5}";
    char name[64];
    double v = 0;
    assert_int_equal(mqtt_json_decode_downlink((const uint8_t*)body, strlen(body), name,
                                               sizeof(name), &v),
                     0);
    assert_string_equal(name, "VOLTAGE_A");
    assert_true(v > 23.49 && v < 23.51);
}

static void test_decode_key_order_and_whitespace(void** state) {
    (void)state;
    /* 键顺序无关、允许空白、允许额外键（忽略）——主站实现差异很常见 */
    const char* body = "{ \"v\" : 7 , \"seq\":12, \"p\" : \"SOC\" }";
    char name[64];
    double v = 0;
    assert_int_equal(mqtt_json_decode_downlink((const uint8_t*)body, strlen(body), name,
                                               sizeof(name), &v),
                     0);
    assert_string_equal(name, "SOC");
    assert_true(v > 6.99 && v < 7.01);
}

static void test_decode_rejects_incomplete(void** state) {
    (void)state;
    char name[64];
    double v = 0;

    /* 缺 v：不能把 value 当 0 用（"关到 0"和"没给值"是两件事） */
    const char* no_v = "{\"p\":\"SOC\"}";
    assert_int_not_equal(mqtt_json_decode_downlink((const uint8_t*)no_v, strlen(no_v), name,
                                                   sizeof(name), &v),
                         0);

    /* 缺 p：不知道该控哪个点 */
    const char* no_p = "{\"v\":1}";
    assert_int_not_equal(mqtt_json_decode_downlink((const uint8_t*)no_p, strlen(no_p), name,
                                                   sizeof(name), &v),
                         0);

    /* 空对象 / 非对象 */
    const char* empty = "{}";
    assert_int_not_equal(mqtt_json_decode_downlink((const uint8_t*)empty, strlen(empty), name,
                                                   sizeof(name), &v),
                         0);
    const char* arr = "[1,2]";
    assert_int_not_equal(
        mqtt_json_decode_downlink((const uint8_t*)arr, strlen(arr), name, sizeof(name), &v), 0);
}

static void test_decode_rejects_unsafe_and_nested(void** state) {
    (void)state;
    char name[64];
    double v = 0;

    /* 名字必须是标识符：带引号/空格的直接拒，别让它进聚合根 */
    const char* bad = "{\"p\":\"A B\",\"v\":1}";
    assert_int_not_equal(
        mqtt_json_decode_downlink((const uint8_t*)bad, strlen(bad), name, sizeof(name), &v), 0);

    /* 嵌套对象没有语义，出现了就是发错了 */
    const char* nested = "{\"p\":\"SOC\",\"v\":{\"x\":1}}";
    assert_int_not_equal(mqtt_json_decode_downlink((const uint8_t*)nested, strlen(nested), name,
                                                   sizeof(name), &v),
                         0);

    /* 转义不支持：名字里不该有，出现即判错（不做半吊子转义） */
    const char* esc = "{\"p\":\"A\\\"B\",\"v\":1}";
    assert_int_not_equal(
        mqtt_json_decode_downlink((const uint8_t*)esc, strlen(esc), name, sizeof(name), &v), 0);
}

/* ---- 2. 编排层只送字节 ---- */

typedef struct {
    int subs;
    char topic[128];
    void (*cb)(void* user, const char* topic, const uint8_t* p, size_t len);
    void* cb_user;
} fake_transport_t;

static int ft_publish(void* ctx, const char* t, const uint8_t* p, size_t l) {
    (void)ctx;
    (void)t;
    (void)p;
    (void)l;
    return 0;
}
static int ft_subscribe(void* ctx, const char* topic,
                        void (*cb)(void* user, const char* topic, const uint8_t* p, size_t len),
                        void* user) {
    fake_transport_t* ft = (fake_transport_t*)ctx;
    ft->subs++;
    snprintf(ft->topic, sizeof(ft->topic), "%s", topic);
    ft->cb = cb;
    ft->cb_user = user;
    return 0;
}
static bool ft_connected(void* ctx) {
    (void)ctx;
    return true;
}

static int g_downlink_hits;
static char g_downlink_addr[128];
static char g_downlink_body[128];

static int mock_on_downlink(void* ctx, const char* address, const uint8_t* payload, size_t len) {
    (void)ctx;
    g_downlink_hits++;
    snprintf(g_downlink_addr, sizeof(g_downlink_addr), "%s", address ? address : "");
    size_t n = len < sizeof(g_downlink_body) - 1 ? len : sizeof(g_downlink_body) - 1;
    memcpy(g_downlink_body, payload, n);
    g_downlink_body[n] = '\0';
    return 0;
}

static void test_orchestrator_forwards_raw_bytes(void** state) {
    (void)state;
    fake_transport_t ft;
    memset(&ft, 0, sizeof(ft));

    northbound_mgr_t mgr;
    nb_source_ops_t src = {0};
    nb_transport_ops_t t = {
        .ctx = &ft,
        .publish = ft_publish,
        .subscribe = ft_subscribe,
        .is_connected = ft_connected,
    };

    assert_int_equal(northbound_mgr_init_ex(&mgr, src, "test", NULL, t), 0);
    northbound_mgr_set_driver_port(&mgr, test_north_driver_port());
    northbound_mgr_set_address(&mgr, "edge/{node}/{kind}", "G", "N1");

    static const edge_north_source_ops_t port = {.on_downlink = mock_on_downlink};
    northbound_mgr_set_north_source(&mgr, &port, NULL);

    assert_int_equal(northbound_mgr_start(&mgr), 0);

    /* 订阅发生在 poll 里（100ms 一次），等它发生 */
    for (int i = 0; i < 100 && ft.subs == 0; i++) usleep(20000);
    assert_int_equal(ft.subs, 1);
    assert_string_equal(ft.topic, "edge/N1/NCMD");
    assert_non_null(ft.cb);

    /* 编排层必须原样转交：地址不动、载荷不动 */
    g_downlink_hits = 0;
    const char* body = "{\"p\":\"SOC\",\"v\":55}";
    ft.cb(ft.cb_user, "edge/N1/NCMD", (const uint8_t*)body, strlen(body));

    assert_int_equal(g_downlink_hits, 1);
    assert_string_equal(g_downlink_addr, "edge/N1/NCMD");
    assert_string_equal(g_downlink_body, body);

    northbound_mgr_stop(&mgr);
    northbound_mgr_destroy(&mgr);
}

int main(void) {
    edge_thread_port_install_uv();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_decode_valid),
        cmocka_unit_test(test_decode_key_order_and_whitespace),
        cmocka_unit_test(test_decode_rejects_incomplete),
        cmocka_unit_test(test_decode_rejects_unsafe_and_nested),
        cmocka_unit_test(test_orchestrator_forwards_raw_bytes),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
