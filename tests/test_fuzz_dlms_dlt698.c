#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "dlms_codec.h"
#include "dlt698_codec.h"
#include "edge_log.h"
#include "net_buf.h"

// --- DLMS/COSEM Fuzz ---

static int setup(void** state) {
    net_buf_pool_t* pool = malloc(sizeof(net_buf_pool_t));
    if (net_buf_pool_init(pool, 10, 512, 0, NULL, NULL) != 0) {
        free(pool);
        return -1;
    }
    *state = pool;
    return 0;
}

static int teardown(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_pool_destroy(pool);
    free(pool);
    return 0;
}

static void test_fuzz_dlms_short(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x01);

    dlms_decoder_t dec;
    dlms_decoder_init(&dec, &in->b);
    dlms_variant_t out;
    int rc = dlms_decode_variant(&dec, &out);
    (void)rc;

    net_buf_unref(in);
}

static void test_fuzz_dlms_overflow(void** state) {
    (void)state;
    static uint8_t raw[4096];
    for (int i = 0; i < (int)sizeof(raw); i++) raw[i] = (uint8_t)(i % 256);

    struct net_buf_simple in;
    net_buf_simple_init_with_data(&in, raw, sizeof(raw));
    in.len = sizeof(raw);

    dlms_decoder_t dec;
    dlms_decoder_init(&dec, &in);
    dlms_variant_t out;
    int rc = dlms_decode_variant(&dec, &out);
    (void)rc;
}

// --- DLT698 Fuzz ---

static void test_fuzz_dlt698_short(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);

    dlt698_addr_t sa;
    uint8_t svc, piid;
    uint8_t* payload = NULL;
    size_t payload_len = 0;
    int rc = dlt698_decode_frame(&in->b, &sa, &svc, &piid, &payload, &payload_len);
    (void)rc;

    net_buf_unref(in);
}

static void test_fuzz_dlt698_wrong_length(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    uint8_t frame[] = {0x68, 0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x16};
    net_buf_simple_add_mem(&in->b, frame, sizeof(frame));

    dlt698_addr_t sa;
    uint8_t svc, piid;
    uint8_t* payload = NULL;
    size_t payload_len = 0;
    int rc = dlt698_decode_frame(&in->b, &sa, &svc, &piid, &payload, &payload_len);
    (void)rc;

    net_buf_unref(in);
}

static void test_fuzz_dlt698_overflow(void** state) {
    (void)state;
    static uint8_t raw[2048];
    for (int i = 0; i < (int)sizeof(raw); i++) raw[i] = (uint8_t)(i % 256);

    struct net_buf_simple in;
    net_buf_simple_init_with_data(&in, raw, sizeof(raw));
    in.len = sizeof(raw);

    dlt698_addr_t sa;
    uint8_t svc, piid;
    uint8_t* payload = NULL;
    size_t payload_len = 0;
    int rc = dlt698_decode_frame(&in, &sa, &svc, &piid, &payload, &payload_len);
    (void)rc;
}

static void test_random_stream(net_buf_pool_t* pool, size_t max_len) {
    const char* env_seed = getenv("EDGE_FUZZ_SEED");
    srand(env_seed ? (unsigned int)strtoul(env_seed, NULL, 0) : 1u);
    for (int iter = 0; iter < 1000; iter++) {
        net_buf_t* in = net_buf_alloc(pool, 0);
        size_t len = (size_t)(rand() % (int)max_len);
        for (size_t i = 0; i < len; i++) net_buf_simple_add_u8(&in->b, (uint8_t)rand());
        uint8_t* raw = in->b.data;
        size_t raw_len = in->b.len;

        dlms_decoder_t dec;
        dlms_decoder_init(&dec, &in->b);
        dlms_variant_t out;
        assert_true(dlms_decode_variant(&dec, &out) <= 0);

        in->b.data = raw;
        in->b.len = (uint16_t)raw_len;
        dlt698_addr_t sa;
        uint8_t svc = 0, piid = 0;
        uint8_t* payload = NULL;
        size_t payload_len = 0;
        int rc = dlt698_decode_frame(&in->b, &sa, &svc, &piid, &payload, &payload_len);
        assert_int_in_range((int)rc, -2, 1);
        net_buf_unref(in);
    }
}

static void test_fuzz_random_bytes(void** state) {
    test_random_stream((net_buf_pool_t*)*state, 300);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_fuzz_dlms_short, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_dlms_overflow, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_dlt698_short, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_dlt698_wrong_length, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_dlt698_overflow, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_random_bytes, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
