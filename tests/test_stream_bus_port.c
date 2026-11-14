#include <cmocka.h>
#include "thread_port_uv.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <uv.h>

#include "stream_bus_port.h"
#include "stream_engine.h"

typedef struct {
    uv_mutex_t lock;
    int publish_count;
    int add_listener_count;
    int remove_listener_count;
    double last_published;
    uint64_t seq;
    pipe_msg_t next;
    bool has_next;
    uv_async_t *handle;
} fake_bus_t;

static fake_bus_t g_bus;

static int fake_publish(void *ctx, const pipe_msg_t *msg) {
    fake_bus_t *bus = ctx;
    uv_mutex_lock(&bus->lock);
    bus->publish_count++;
    bus->last_published = msg->payload.val.f;
    uv_mutex_unlock(&bus->lock);
    return 0;
}

static int fake_write_shadow(void *ctx, const pipe_msg_t *msg) {
    (void)ctx;
    (void)msg;
    return 0;
}

static int fake_read_shadow(void *ctx, uint32_t tag_id, pipe_msg_t *out) {
    (void)ctx;
    (void)tag_id;
    (void)out;
    return -1;
}

static int fake_read_ring(void *ctx, uint64_t *seq, pipe_msg_t *out) {
    fake_bus_t *bus = ctx;
    uv_mutex_lock(&bus->lock);
    if (!bus->has_next) {
        uv_mutex_unlock(&bus->lock);
        return -1;
    }
    *out = bus->next;
    bus->has_next = false;
    bus->seq++;
    *seq = bus->seq;
    uv_mutex_unlock(&bus->lock);
    return 0;
}

static uint64_t fake_ring_seq(void *ctx) {
    fake_bus_t *bus = ctx;
    return bus->seq;
}

static int fake_add_listener(void *ctx, uv_async_t *handle) {
    fake_bus_t *bus = ctx;
    bus->handle = handle;
    bus->add_listener_count++;
    return 0;
}

static int fake_remove_listener(void *ctx, uv_async_t *handle) {
    fake_bus_t *bus = ctx;
    (void)handle;
    bus->remove_listener_count++;
    return 0;
}

static stream_bus_port_t make_port(fake_bus_t *bus) {
    stream_bus_port_t port = {
        .ctx = bus,
        .publish = fake_publish,
        .write_shadow = fake_write_shadow,
        .read_shadow = fake_read_shadow,
        .read_ring = fake_read_ring,
        .ring_seq = fake_ring_seq,
        .add_listener = fake_add_listener,
        .remove_listener = fake_remove_listener,
    };
    return port;
}

static stage_ret_t double_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)stage;
    ev->payload.val.f *= 2.0;
    return STAGE_OK;
}

static void test_engine_uses_only_the_port(void **state) {
    (void)state;
    memset(&g_bus, 0, sizeof(g_bus));
    uv_mutex_init(&g_bus.lock);
    g_bus.next.type = PIPEMSG_TYPE_SCALAR;
    g_bus.next.tag_id = 42;
    g_bus.next.payload.val.f = 1.0;

    stream_bus_port_t port = make_port(&g_bus);
    stream_engine_storage_t storage;
    stream_engine_t *engine =
        stream_engine_create_in(&storage, &port, uv_default_loop(), NULL, NULL);
    assert_non_null(engine);

    pipeline_t *pipeline = stream_engine_add_pipeline(engine, "port_test", 42);
    assert_non_null(pipeline);
    assert_non_null(pipeline_add_stage(pipeline, "double", double_proc, NULL, NULL, NULL, NULL));

    stream_engine_start(engine);
    assert_int_equal(g_bus.add_listener_count, 1);
    assert_non_null(g_bus.handle);

    uv_mutex_lock(&g_bus.lock);
    g_bus.has_next = true;
    uv_mutex_unlock(&g_bus.lock);
    uv_async_send(g_bus.handle);
    uv_sleep(100);

    uv_mutex_lock(&g_bus.lock);
    int published = g_bus.publish_count;
    double value = g_bus.last_published;
    uv_mutex_unlock(&g_bus.lock);
    assert_int_equal(published, 1);
    assert_true(value == 2.0);

    stream_engine_destroy(engine);
    assert_int_equal(g_bus.remove_listener_count, 1);
    uv_mutex_destroy(&g_bus.lock);
}

int main(void) {
    edge_thread_port_install_uv();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_engine_uses_only_the_port),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
