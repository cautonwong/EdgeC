#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "net_buf.h"
#include "dlms_codec.h"
#include "dlt698_codec.h"
#include "edge_log.h"

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
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    for (int i = 0; i < 4096; i++)
        net_buf_simple_add_u8(&in->b, (uint8_t)(i % 256));

    dlms_decoder_t dec;
    dlms_decoder_init(&dec, &in->b);
    dlms_variant_t out;
    int rc = dlms_decode_variant(&dec, &out);
    (void)rc;

    net_buf_unref(in);
}

// --- DLT698 Fuzz ---

static void test_fuzz_dlt698_short(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);

    dlt698_addr_t sa;
    uint8_t svc, piid;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
    int rc = dlt698_decode_frame(&in->b, &sa, &svc, &piid, &payload, &payload_len);
    (void)rc;

    net_buf_unref(in);
}

static void test_fuzz_dlt698_wrong_length(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    uint8_t frame[] = { 0x68, 0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x16 };
    net_buf_simple_add_mem(&in->b, frame, sizeof(frame));

    dlt698_addr_t sa;
    uint8_t svc, piid;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
    int rc = dlt698_decode_frame(&in->b, &sa, &svc, &piid, &payload, &payload_len);
    (void)rc;

    net_buf_unref(in);
}

static void test_fuzz_dlt698_overflow(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    for (int i = 0; i < 2048; i++)
        net_buf_simple_add_u8(&in->b, (uint8_t)(i % 256));

    dlt698_addr_t sa;
    uint8_t svc, piid;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
    int rc = dlt698_decode_frame(&in->b, &sa, &svc, &piid, &payload, &payload_len);
    (void)rc;

    net_buf_unref(in);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_fuzz_dlms_short, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_dlms_overflow, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_dlt698_short, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_dlt698_wrong_length, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_dlt698_overflow, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
