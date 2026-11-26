/**
 * @file test_northbound_ipc_dispatch.c
 * @brief 现场 binaryFlow：通配订阅、精确过滤、分发与响应发布。
 *
 * 全部用 fake transport + fake 端口，不起 broker；订阅只订一条
 * `ipc/{sn}/#`，命中判断来自渲染结果与收到的主题逐字比较。
 */

#include <cmocka.h>
#include "thread_port_uv.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ipc_binary_flow_codec.h"
#include "northbound_mgr.h"
#include "north_driver_test_port.h"

#define TEST_SN "E4B853F333"
#define SET_TOPIC "ipc/E4B853F333/env/ems/emsctrl/Set/binaryFlow"
#define GET_TOPIC "ipc/E4B853F333/env/ems/emsctrl/Get/binaryFlow"
#define SET_RESP_TOPIC "ipc/E4B853F333/env/ems/emsctrl/SetResp/binaryFlow"
#define GET_RESP_TOPIC "ipc/E4B853F333/env/ems/emsctrl/GetResp/binaryFlow"
#define WAIT_MS 3000

typedef struct {
    char topic[NORTH_IPC_TOPIC_MAX];
    void (*cb)(void* user, const char* topic, const uint8_t* p, size_t len);
    void* user;
} fake_sub_t;

typedef struct {
    fake_sub_t subs[4];
    int sub_count;
    int sub_calls;
    int publish_calls;
    char pub_topic[NORTH_IPC_TOPIC_MAX];
    uint8_t pub_payload[256];
    size_t pub_len;
    bool connected;
    uint32_t epoch;
} fake_transport_t;

typedef struct {
    north_ipc_endpoint_t eps[2];
    size_t count;
    int hits;
    north_ipc_verb_t verb;
    uint8_t payload[256];
    size_t len;
    int last_rc;
} fake_port_t;

typedef struct {
    fake_transport_t ft;
    fake_port_t fp;
    northbound_mgr_t mgr;
} harness_t;

static bool _topic_match(const char* pattern, const char* topic) {
    size_t n = strlen(pattern);
    if (n > 0 && pattern[n - 1] == '#') return strncmp(pattern, topic, n - 1) == 0;
    return strcmp(pattern, topic) == 0;
}

static int ft_publish(void* ctx, const char* topic, const uint8_t* payload, size_t len) {
    /* 周期 NBIRTH 也会走这里；本文件只关心 ipc/ 响应。 */
    if (!topic || strncmp(topic, "ipc/", 4) != 0) return 0;
    fake_transport_t* ft = (fake_transport_t*)ctx;
    ft->publish_calls++;
    snprintf(ft->pub_topic, sizeof(ft->pub_topic), "%s", topic ? topic : "");
    ft->pub_len = len < sizeof(ft->pub_payload) ? len : sizeof(ft->pub_payload);
    if (ft->pub_len && payload) memcpy(ft->pub_payload, payload, ft->pub_len);
    return 0;
}

static int ft_subscribe(void* ctx, const char* topic,
                        void (*cb)(void* user, const char* topic, const uint8_t* p, size_t len),
                        void* user) {
    fake_transport_t* ft = (fake_transport_t*)ctx;
    ft->sub_calls++;
    for (int i = 0; i < ft->sub_count; i++) {
        if (strcmp(ft->subs[i].topic, topic) == 0) {
            ft->subs[i].cb = cb;
            ft->subs[i].user = user;
            return 0;
        }
    }
    if (ft->sub_count >= 4) return -1;
    snprintf(ft->subs[ft->sub_count].topic, sizeof(ft->subs[ft->sub_count].topic), "%s", topic);
    ft->subs[ft->sub_count].cb = cb;
    ft->subs[ft->sub_count].user = user;
    ft->sub_count++;
    return 0;
}

static bool ft_connected(void* ctx) {
    return ((fake_transport_t*)ctx)->connected;
}

static uint32_t ft_epoch(void* ctx) {
    return ((fake_transport_t*)ctx)->epoch;
}

static void ft_fire(fake_transport_t* ft, const char* topic, const uint8_t* payload, size_t len) {
    for (int i = 0; i < ft->sub_count; i++) {
        if (ft->subs[i].cb && _topic_match(ft->subs[i].topic, topic)) {
            ft->subs[i].cb(ft->subs[i].user, topic, payload, len);
            return;
        }
    }
}

static size_t fp_count(void* ctx) {
    return ((fake_port_t*)ctx)->count;
}

static int fp_at(void* ctx, size_t index, north_ipc_endpoint_t* out) {
    fake_port_t* fp = (fake_port_t*)ctx;
    if (!out || index >= fp->count) return -1;
    *out = fp->eps[index];
    return 0;
}

static int fp_on_frame(void* ctx, const north_ipc_endpoint_t* ep, north_ipc_verb_t verb,
                       const uint8_t* payload, size_t len) {
    (void)ep;
    fake_port_t* fp = (fake_port_t*)ctx;
    fp->hits++;
    fp->verb = verb;
    fp->len = len < sizeof(fp->payload) ? len : sizeof(fp->payload);
    if (fp->len && payload) memcpy(fp->payload, payload, fp->len);
    fp->last_rc = 0;
    return 0;
}

static north_ipc_port_t _fake_port(fake_port_t* fp) {
    north_ipc_port_t port = {.ctx = fp,
                             .endpoint_count = fp_count,
                             .endpoint_at = fp_at,
                             .on_frame = fp_on_frame};
    return port;
}

static void harness_init(harness_t* h) {
    memset(h, 0, sizeof(*h));
    h->ft.connected = true;
    nb_transport_ops_t t = {
        .ctx = &h->ft,
        .publish = ft_publish,
        .subscribe = ft_subscribe,
        .is_connected = ft_connected,
        .session_epoch = ft_epoch,
    };
    nb_source_ops_t src = {0};
    assert_int_equal(northbound_mgr_init_ex(&h->mgr, src, "test", NULL, t), 0);
    northbound_mgr_set_driver_port(&h->mgr, test_north_driver_port());
    northbound_mgr_set_address(&h->mgr, "edge/{node}/{kind}", "G", "N1");
}

static void harness_stop(harness_t* h) {
    northbound_mgr_stop(&h->mgr);
    northbound_mgr_destroy(&h->mgr);
}

static void wait_sub_calls(fake_transport_t* ft, int calls) {
    for (int i = 0; i < WAIT_MS / 20 && ft->sub_calls < calls; i++) usleep(20000);
    assert_int_equal(ft->sub_calls, calls);
}

static void _one_endpoint(harness_t* h) {
    h->fp.count = 1;
    h->fp.eps[0] = (north_ipc_endpoint_t){"env", "ems", "emsctrl"};
}

static void test_wildcard_subscribed_once(void** state) {
    (void)state;
    harness_t h;
    harness_init(&h);
    _one_endpoint(&h);
    northbound_mgr_set_ipc_port(&h.mgr, _fake_port(&h.fp), TEST_SN);

    assert_int_equal(northbound_mgr_start(&h.mgr), 0);
    wait_sub_calls(&h.ft, 1);
    usleep(1200000); /* 再等两轮 poll，确认没有重复订阅 */

    assert_int_equal(h.ft.sub_calls, 1);
    assert_int_equal(h.ft.sub_count, 1);
    assert_string_equal(h.ft.subs[0].topic, "ipc/E4B853F333/#");
    assert_non_null(h.ft.subs[0].cb);
    harness_stop(&h);
}

static int _mock_downlink(void* ctx, const char* address, const uint8_t* payload, size_t len) {
    (void)ctx;
    (void)address;
    (void)payload;
    (void)len;
    return 0;
}

static void test_no_port_no_subscribe(void** state) {
    (void)state;
    harness_t h;
    harness_init(&h);
    static const edge_north_source_ops_t src = {.on_downlink = _mock_downlink};
    northbound_mgr_set_north_source(&h.mgr, &src, NULL);

    assert_int_equal(northbound_mgr_start(&h.mgr), 0);
    wait_sub_calls(&h.ft, 1);
    usleep(1200000);

    assert_int_equal(h.ft.sub_calls, 1);
    assert_string_equal(h.ft.subs[0].topic, "edge/N1/NCMD");
    harness_stop(&h);
}

static void test_set_get_dispatch(void** state) {
    (void)state;
    harness_t h;
    harness_init(&h);
    _one_endpoint(&h);
    northbound_mgr_set_ipc_port(&h.mgr, _fake_port(&h.fp), TEST_SN);

    assert_int_equal(northbound_mgr_start(&h.mgr), 0);
    wait_sub_calls(&h.ft, 1);

    const uint8_t set_payload[] = {0x00, 0x01, 0x02, 0x7F};
    ft_fire(&h.ft, SET_TOPIC, set_payload, sizeof(set_payload));
    assert_int_equal(h.fp.hits, 1);
    assert_int_equal(h.fp.verb, NORTH_IPC_SET);
    assert_int_equal(h.fp.len, sizeof(set_payload));
    assert_memory_equal(h.fp.payload, set_payload, sizeof(set_payload));

    ft_fire(&h.ft, GET_TOPIC, NULL, 0);
    assert_int_equal(h.fp.hits, 2);
    assert_int_equal(h.fp.verb, NORTH_IPC_GET);
    assert_int_equal(h.fp.len, 0);
    assert_int_equal(h.fp.last_rc, 0);
    harness_stop(&h);
}

static void test_unknown_and_response_ignored(void** state) {
    (void)state;
    harness_t h;
    harness_init(&h);
    _one_endpoint(&h);
    northbound_mgr_set_ipc_port(&h.mgr, _fake_port(&h.fp), TEST_SN);

    assert_int_equal(northbound_mgr_start(&h.mgr), 0);
    wait_sub_calls(&h.ft, 1);

    ft_fire(&h.ft, "ipc/E4B853F333/env/ems/other/Set/binaryFlow", (const uint8_t*)"x", 1);
    ft_fire(&h.ft, SET_RESP_TOPIC, (const uint8_t*)"x", 1);
    ft_fire(&h.ft, GET_RESP_TOPIC, (const uint8_t*)"x", 1);
    ft_fire(&h.ft, "ipc/E4B853F333/env/ems/emsctrl/Set/json", (const uint8_t*)"x", 1);
    ft_fire(&h.ft, "edge/N1/NCMD", (const uint8_t*)"x", 1);

    assert_int_equal(h.fp.hits, 0);
    harness_stop(&h);
}

static northbound_mgr_t* g_codec_mgr;

static int _codec_on_frame(void* ctx, const north_ipc_endpoint_t* ep, north_ipc_verb_t verb,
                           const uint8_t* payload, size_t len) {
    fake_port_t* fp = (fake_port_t*)ctx;
    fp->hits++;
    fp->verb = verb;
    fp->len = len;
    fp->last_rc = -1;

    ipc_bf_header_t header;
    ipc_bf_cursor_t cur;
    ipc_bf_block_t block;
    if (ipc_bf_decode_header(payload, len, &header, &cur) < 0) return -1;
    int rc;
    while ((rc = ipc_bf_block_next(&cur, &block)) == 0) {
    }
    if (rc < 0) return -1;

    fp->last_rc = 0;
    if (g_codec_mgr) {
        northbound_mgr_publish_ipc(g_codec_mgr, ep, NORTH_IPC_SET_RESP, payload, len);
    }
    return 0;
}

static void test_malformed_frame_rejected(void** state) {
    (void)state;
    harness_t h;
    harness_init(&h);
    _one_endpoint(&h);
    north_ipc_port_t port = _fake_port(&h.fp);
    port.on_frame = _codec_on_frame;
    northbound_mgr_set_ipc_port(&h.mgr, port, TEST_SN);
    g_codec_mgr = &h.mgr;

    assert_int_equal(northbound_mgr_start(&h.mgr), 0);
    wait_sub_calls(&h.ft, 1);

    /* P4-02b 向量：1 块，block 0x8031，start_sub 1，sub_num 10，data_len 0。 */
    const uint8_t frame[] = {0x00, 0x00, 0x03, 0x37, 0x00, 0x00, 0x00, 0x0b, 0x01, 0x00,
                             0x00, 0x80, 0x31, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x00};
    ft_fire(&h.ft, SET_TOPIC, frame, sizeof(frame));
    assert_int_equal(h.fp.hits, 1);
    assert_int_equal(h.fp.last_rc, 0);
    assert_int_equal(h.ft.publish_calls, 1);
    assert_string_equal(h.ft.pub_topic, SET_RESP_TOPIC);
    assert_int_equal(h.ft.pub_len, sizeof(frame));
    assert_memory_equal(h.ft.pub_payload, frame, sizeof(frame));

    h.fp.hits = 0;
    h.ft.publish_calls = 0;
    ft_fire(&h.ft, SET_TOPIC, frame, sizeof(frame) - 2);
    assert_int_equal(h.fp.hits, 1);
    assert_int_equal(h.fp.last_rc, -1);
    assert_int_equal(h.ft.publish_calls, 0);

    g_codec_mgr = NULL;
    harness_stop(&h);
}

static void test_publish_ipc_topic_and_bytes(void** state) {
    (void)state;
    harness_t h;
    harness_init(&h);
    northbound_mgr_set_ipc_port(&h.mgr, _fake_port(&h.fp), TEST_SN);

    const north_ipc_endpoint_t ep = {"env", "ems", "emsctrl"};
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};

    assert_int_equal(
        northbound_mgr_publish_ipc(&h.mgr, &ep, NORTH_IPC_SET_RESP, payload, sizeof(payload)), 0);
    assert_int_equal(h.ft.publish_calls, 1);
    assert_string_equal(h.ft.pub_topic, SET_RESP_TOPIC);
    assert_int_equal(h.ft.pub_len, sizeof(payload));
    assert_memory_equal(h.ft.pub_payload, payload, sizeof(payload));

    assert_int_equal(northbound_mgr_publish_ipc(&h.mgr, &ep, NORTH_IPC_GET_RESP, payload, 0), 0);
    assert_int_equal(h.ft.publish_calls, 2);
    assert_string_equal(h.ft.pub_topic, GET_RESP_TOPIC);
    assert_int_equal(h.ft.pub_len, 0);

    assert_int_equal(
        northbound_mgr_publish_ipc(&h.mgr, &ep, NORTH_IPC_SET, payload, sizeof(payload)), -1);
    assert_int_equal(
        northbound_mgr_publish_ipc(&h.mgr, &ep, NORTH_IPC_GET, payload, sizeof(payload)), -1);
    assert_int_equal(h.ft.publish_calls, 2);

    harness_stop(&h);
}

static void test_set_port_resubscribes(void** state) {
    (void)state;
    harness_t h;
    harness_init(&h);
    _one_endpoint(&h);
    northbound_mgr_set_ipc_port(&h.mgr, _fake_port(&h.fp), TEST_SN);

    assert_int_equal(northbound_mgr_start(&h.mgr), 0);
    wait_sub_calls(&h.ft, 1);

    northbound_mgr_set_ipc_port(&h.mgr, _fake_port(&h.fp), TEST_SN);
    wait_sub_calls(&h.ft, 2);

    /* fake transport 与 mqtt_producer 一样按主题去重，槽位仍只有一条。 */
    assert_int_equal(h.ft.sub_count, 1);
    assert_string_equal(h.ft.subs[0].topic, "ipc/E4B853F333/#");
    harness_stop(&h);
}

static void test_epoch_does_not_resubscribe(void** state) {
    (void)state;
    harness_t h;
    harness_init(&h);
    _one_endpoint(&h);
    northbound_mgr_set_ipc_port(&h.mgr, _fake_port(&h.fp), TEST_SN);

    assert_int_equal(northbound_mgr_start(&h.mgr), 0);
    wait_sub_calls(&h.ft, 1);

    h.ft.epoch = 1;
    usleep(1200000); /* 让 poll 看到新代数 */
    assert_int_equal(h.ft.sub_calls, 1);
    harness_stop(&h);
}

int main(void) {
    edge_thread_port_install_uv();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wildcard_subscribed_once),
        cmocka_unit_test(test_no_port_no_subscribe),
        cmocka_unit_test(test_set_get_dispatch),
        cmocka_unit_test(test_unknown_and_response_ignored),
        cmocka_unit_test(test_malformed_frame_rejected),
        cmocka_unit_test(test_publish_ipc_topic_and_bytes),
        cmocka_unit_test(test_set_port_resubscribes),
        cmocka_unit_test(test_epoch_does_not_resubscribe),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
