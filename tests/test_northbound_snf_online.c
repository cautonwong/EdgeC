/* Regression: northbound must drive SnF online state even without a service owner. */
#undef NDEBUG
#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <uv.h>

#include "northbound_mgr.h"
#include "snf_manager.h"

static atomic_bool g_marked;
static atomic_int g_published;

static int fake_read_next(void *ctx, uint64_t *seq, pipe_msg_t *out) {
    (void)ctx;
    (void)seq;
    (void)out;
    return 1;
}

static bool fake_is_connected(void *ctx) {
    (void)ctx;
    return true;
}

static int fake_publish(void *ctx, const char *topic, const uint8_t *payload, size_t len) {
    (void)ctx;
    (void)topic;
    (void)payload;
    (void)len;
    atomic_fetch_add(&g_published, 1);
    return 0;
}

static int fake_store_ensure(void *ctx) {
    (void)ctx;
    return 0;
}

static int fake_store_count(void *ctx, int *out) {
    (void)ctx;
    *out = 1;
    return 0;
}

static int fake_store_read(void *ctx, uint32_t limit, snf_record_t *out, uint32_t cap,
                           uint32_t *out_count) {
    (void)ctx;
    if (!cap || !limit) {
        *out_count = 0;
        return 0;
    }
    out[0].ts = 1000;
    out[0].tag_id = 0x1001;
    out[0].value = 220.0;
    out[0].quality = 0;
    *out_count = 1;
    return 0;
}

static int fake_store_mark(void *ctx, const snf_record_t *records, uint32_t count) {
    (void)ctx;
    (void)records;
    (void)count;
    atomic_store(&g_marked, true);
    return 0;
}

static int fake_store_insert(void *ctx, const PipeMsg *msg) {
    (void)ctx;
    (void)msg;
    return 0;
}

static void test_snf_online_without_user_data(void) {
    northbound_mgr_t mgr;
    snf_store_port_t store = {
        .ctx = NULL,
        .ensure_schema = fake_store_ensure,
        .count_unsynced = fake_store_count,
        .read_unsynced = fake_store_read,
        .mark_synced = fake_store_mark,
        .insert = fake_store_insert,
    };
    nb_source_ops_t source = {.ctx = NULL, .read_next = fake_read_next};
    nb_transport_ops_t transport = {
        .ctx = NULL,
        .publish = fake_publish,
        .is_connected = fake_is_connected,
        .pending = NULL,
        .session_epoch = NULL,
    };
    atomic_store(&g_marked, false);
    atomic_store(&g_published, 0);

    assert(northbound_mgr_init_ex(&mgr, source, "test", &store, transport) == 0);
    assert(northbound_mgr_start(&mgr) == 0);
    uv_sleep(6500);
    northbound_mgr_stop(&mgr);
    northbound_mgr_destroy(&mgr);

    assert(atomic_load(&g_marked));
}

int main(void) {
    test_snf_online_without_user_data();
    printf("northbound SnF online regression passed\n");
    return 0;
}
