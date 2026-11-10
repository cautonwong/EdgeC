#include "edge_mcp_datahub_adapter.h"

#include <stdio.h>
#include <string.h>

#include "datahub.h"

static int _read_tag(void *ctx, uint32_t tag_id, edge_mcp_tag_value_t *out) {
    if (!out) return -1;
    pipe_msg_t msg = {0};
    int rc = datahub_read_shadow((datahub_t *)ctx, tag_id, &msg);
    if (rc != 0) return rc;
    out->tag_id = msg.tag_id;
    out->value = msg.payload.val.f;
    out->quality = msg.quality;
    out->type = msg.type;
    out->timestamp_ms = msg.ts;
    return 0;
}

static int _write_tag(void *ctx, const edge_mcp_tag_value_t *value) {
    if (!value) return -1;
    pipe_msg_t msg = {
        .tag_id = value->tag_id,
        .payload = {.val = {.f = value->value}},
        .quality = value->quality,
        .type = value->type,
        .ts = value->timestamp_ms,
    };
    return datahub_write_shadow((datahub_t *)ctx, &msg);
}

static int _get_tag_meta(void *ctx, uint32_t tag_id, edge_mcp_tag_meta_t *out) {
    if (!ctx || !out) return -1;
    datahub_t *hub = (datahub_t *)ctx;
    const edge_tag_meta_t *meta = NULL;
    uint32_t count = datahub_get_metadata_count(hub);
    for (uint32_t i = 0; i < count; i++) {
        const edge_tag_meta_t *candidate = datahub_get_metadata_by_index(hub, i);
        if (candidate && candidate->id == tag_id) {
            meta = candidate;
            break;
        }
    }
    if (!meta) return -1;

    memset(out, 0, sizeof(*out));
    out->tag_id = meta->id;
    if (meta->name) snprintf(out->name, sizeof(out->name), "%s", meta->name);
    if (meta->unit) snprintf(out->unit, sizeof(out->unit), "%s", meta->unit);
    if (meta->category) snprintf(out->category, sizeof(out->category), "%s", meta->category);
    out->min_value = meta->min;
    out->max_value = meta->max;
    return 0;
}

edge_mcp_tool_port_t edge_mcp_datahub_tool_port(datahub_t *hub) {
    edge_mcp_tool_port_t port = {
        .ctx = hub,
        .read_tag = _read_tag,
        .write_tag = _write_tag,
        .get_tag_meta = _get_tag_meta,
    };
    return port;
}
