#include "stream_bus_datahub_adapter.h"

#include "datahub.h"

static int _publish(void *ctx, const pipe_msg_t *msg) {
    return datahub_publish((datahub_t *)ctx, msg);
}

static int _write_shadow(void *ctx, const pipe_msg_t *msg) {
    return datahub_write_shadow((datahub_t *)ctx, msg);
}

static int _read_shadow(void *ctx, uint32_t tag_id, pipe_msg_t *out) {
    return datahub_read_shadow((datahub_t *)ctx, tag_id, out);
}

static int _read_ring(void *ctx, uint64_t *seq, pipe_msg_t *out) {
    return datahub_read_ring((datahub_t *)ctx, seq, out);
}

static uint64_t _ring_seq(void *ctx) {
    return datahub_get_ring_seq((datahub_t *)ctx);
}

static int _add_listener(void *ctx, uv_async_t *handle) {
    return datahub_add_listener((datahub_t *)ctx, handle);
}

static int _remove_listener(void *ctx, uv_async_t *handle) {
    return datahub_remove_listener((datahub_t *)ctx, handle);
}

stream_bus_port_t stream_bus_datahub_port(datahub_t *hub) {
    stream_bus_port_t port = {
        .ctx = hub,
        .publish = _publish,
        .write_shadow = _write_shadow,
        .read_shadow = _read_shadow,
        .read_ring = _read_ring,
        .ring_seq = _ring_seq,
        .add_listener = _add_listener,
        .remove_listener = _remove_listener,
    };
    return port;
}

stream_engine_t *stream_engine_create_datahub(datahub_t *hub, uv_loop_t *loop, void *orch,
                                              const edge_stage_factory_entry_t *registry) {
    stream_bus_port_t bus = stream_bus_datahub_port(hub);
    return stream_engine_create(&bus, loop, orch, registry);
}

stream_engine_t *stream_engine_create_in_datahub(stream_engine_storage_t *storage, datahub_t *hub,
                                                 uv_loop_t *loop, void *orch,
                                                 const edge_stage_factory_entry_t *registry) {
    stream_bus_port_t bus = stream_bus_datahub_port(hub);
    return stream_engine_create_in(storage, &bus, loop, orch, registry);
}
