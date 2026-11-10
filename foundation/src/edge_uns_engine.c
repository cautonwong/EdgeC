/**
 * @file edge_uns_engine.c
 * @brief ISA-95 Unified Namespace Engine & Sparkplug B / OPC UA Mirror Implementation
 */

#include "edge_uns_engine.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

int edge_uns_engine_init(edge_uns_engine_t *engine, const char *group_id, const char *edge_node_id) {
    if (!engine) return -1;
    memset(engine, 0, sizeof(edge_uns_engine_t));

    if (group_id) {
        strncpy(engine->group_id, group_id, sizeof(engine->group_id) - 1);
    }
    if (edge_node_id) {
        strncpy(engine->edge_node_id, edge_node_id, sizeof(engine->edge_node_id) - 1);
    }
    engine->seq_number = 0;
    return 0;
}

int edge_uns_register_point(edge_uns_engine_t *engine, const edge_uns_point_def_t *point_def) {
    if (!engine || !point_def) return -1;
    if (engine->node_count >= EDGE_UNS_MAX_NODES) return -2;

    /* Check for duplicate tag_id */
    for (uint32_t i = 0; i < engine->node_count; i++) {
        if (engine->slots[i].def.tag_id == point_def->tag_id) {
            return -3; /* Duplicate */
        }
    }

    uint32_t idx = engine->node_count;
    memcpy(&engine->slots[idx].def, point_def, sizeof(edge_uns_point_def_t));
    if (engine->slots[idx].def.scale == 0.0f) {
        engine->slots[idx].def.scale = 1.0f;
    }
    engine->slots[idx].is_initialized = false;
    engine->slots[idx].is_online = true;
    engine->node_count++;
    return 0;
}

int edge_uns_format_path(const edge_uns_isa95_path_t *path, char *out_buf, size_t max_len) {
    if (!path || !out_buf || max_len == 0) return -1;
    int ret = snprintf(out_buf, max_len, "%s/%s/%s/%s/%s/%s",
                       path->enterprise, path->site, path->area,
                       path->line, path->cell, path->metric_name);
    return (ret > 0 && (size_t)ret < max_len) ? 0 : -2;
}

int edge_uns_find_by_path(const edge_uns_engine_t *engine, const char *full_path) {
    if (!engine || !full_path) return -1;
    char buf[EDGE_UNS_FULL_PATH_LEN];

    for (uint32_t i = 0; i < engine->node_count; i++) {
        if (edge_uns_format_path(&engine->slots[i].def.path, buf, sizeof(buf)) == 0) {
            if (strcmp(buf, full_path) == 0) {
                return (int)i;
            }
        }
    }
    return -1;
}

int edge_uns_find_by_tag(const edge_uns_engine_t *engine, uint32_t tag_id) {
    if (!engine) return -1;
    for (uint32_t i = 0; i < engine->node_count; i++) {
        if (engine->slots[i].def.tag_id == tag_id) {
            return (int)i;
        }
    }
    return -1;
}

int edge_uns_evaluate_point(edge_uns_engine_t *engine, uint32_t tag_id, double new_raw_val,
                            uint64_t timestamp_ms, bool *out_rbe_triggered, double *out_scaled_val) {
    if (!engine) return -1;
    int idx = edge_uns_find_by_tag(engine, tag_id);
    if (idx < 0) return -2;

    edge_uns_node_slot_t *slot = &engine->slots[idx];
    engine->telemetry.total_evaluations++;

    double scaled_val = new_raw_val * (double)slot->def.scale;
    if (out_scaled_val) {
        *out_scaled_val = scaled_val;
    }

    bool trigger = false;
    if (!slot->is_initialized) {
        trigger = true;
        slot->is_initialized = true;
    } else {
        double delta = fabs(scaled_val - slot->last_published_val);
        if (delta >= (double)slot->def.deadband) {
            trigger = true;
        }
    }

    slot->last_raw_val = new_raw_val;
    slot->last_update_ts_ms = timestamp_ms;

    if (trigger) {
        slot->last_published_val = scaled_val;
        engine->telemetry.rbe_events_triggered++;
    } else {
        engine->telemetry.deadband_filtered_count++;
    }

    if (out_rbe_triggered) {
        *out_rbe_triggered = trigger;
    }

    return trigger ? 1 : 0;
}

static void encode_metric_to_buffer(struct net_buf_simple *buf, const edge_uns_node_slot_t *slot, bool is_birth) {
    /* Metric submessage buffer */
    uint8_t sub_raw[128];
    struct net_buf_simple sub;
    net_buf_simple_init_with_data(&sub, sub_raw, sizeof(sub_raw));
    net_buf_simple_reset(&sub);

    /* Name or Alias */
    if (is_birth || slot->def.sparkplug_alias == 0) {
        char path_str[EDGE_UNS_FULL_PATH_LEN];
        edge_uns_format_path(&slot->def.path, path_str, sizeof(path_str));
        spb_encode_string(&sub, SPB_METRIC_NAME, path_str);
    }
    if (slot->def.sparkplug_alias > 0) {
        spb_encode_tag(&sub, SPB_METRIC_ALIAS, 0);
        spb_encode_varint(&sub, slot->def.sparkplug_alias);
    }

    /* Timestamp */
    spb_encode_tag(&sub, SPB_METRIC_TIMESTAMP, 0);
    spb_encode_varint(&sub, slot->last_update_ts_ms);

    /* Datatype */
    spb_encode_tag(&sub, SPB_METRIC_DATATYPE, 0);
    spb_encode_varint(&sub, (uint64_t)slot->def.datatype);

    /* Value */
    if (slot->def.datatype == SPB_TYPE_FLOAT) {
        float fval = (float)slot->last_published_val;
        spb_encode_tag(&sub, SPB_METRIC_FLOAT_VAL, 5); /* 32-bit */
        net_buf_simple_add_mem(&sub, &fval, sizeof(float));
    } else if (slot->def.datatype == SPB_TYPE_DOUBLE) {
        double dval = slot->last_published_val;
        spb_encode_tag(&sub, SPB_METRIC_DOUBLE_VAL, 1); /* 64-bit */
        net_buf_simple_add_mem(&sub, &dval, sizeof(double));
    } else if (slot->def.datatype == SPB_TYPE_BOOLEAN) {
        spb_encode_tag(&sub, SPB_METRIC_BOOL_VAL, 0);
        spb_encode_varint(&sub, slot->last_published_val != 0.0 ? 1 : 0);
    } else {
        /* Default to int64 / uint64 */
        spb_encode_tag(&sub, SPB_METRIC_LONG_VAL, 0);
        spb_encode_varint(&sub, (uint64_t)((int64_t)slot->last_published_val));
    }

    /* Add length-delimited metric to main payload */
    spb_encode_tag(buf, SPB_FIELD_METRICS, 2);
    spb_encode_varint(buf, sub.len);
    net_buf_simple_add_mem(buf, sub.data, sub.len);
}

int edge_uns_generate_sparkplug_nbirth(edge_uns_engine_t *engine, uint8_t *out_buf, size_t buf_cap, size_t *out_len) {
    if (!engine || !out_buf || buf_cap < 64) return -1;

    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, out_buf, buf_cap);
    net_buf_simple_reset(&b);

    /* Payload Timestamp */
    spb_encode_tag(&b, SPB_FIELD_TIMESTAMP, 0);
    spb_encode_varint(&b, engine->birth_timestamp_ms);

    /* Seq = 0 for NBIRTH */
    engine->seq_number = 0;
    spb_encode_tag(&b, SPB_FIELD_SEQ, 0);
    spb_encode_varint(&b, engine->seq_number);

    /* Encode all registered metrics with full names & definitions */
    for (uint32_t i = 0; i < engine->node_count; i++) {
        encode_metric_to_buffer(&b, &engine->slots[i], true);
    }

    if (out_len) *out_len = b.len;
    engine->telemetry.sparkplug_nbirth_count++;
    return 0;
}

int edge_uns_generate_sparkplug_ndata_single(edge_uns_engine_t *engine, uint32_t tag_id,
                                             uint64_t timestamp_ms, uint8_t *out_buf,
                                             size_t buf_cap, size_t *out_len) {
    if (!engine || !out_buf || buf_cap < 32) return -1;
    int idx = edge_uns_find_by_tag(engine, tag_id);
    if (idx < 0) return -2;

    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, out_buf, buf_cap);
    net_buf_simple_reset(&b);

    /* Payload Timestamp */
    spb_encode_tag(&b, SPB_FIELD_TIMESTAMP, 0);
    spb_encode_varint(&b, timestamp_ms);

    /* Seq */
    engine->seq_number = (engine->seq_number + 1) & 0xFF;
    spb_encode_tag(&b, SPB_FIELD_SEQ, 0);
    spb_encode_varint(&b, engine->seq_number);

    encode_metric_to_buffer(&b, &engine->slots[idx], false);

    if (out_len) *out_len = b.len;
    engine->telemetry.sparkplug_ndata_count++;
    return 0;
}

int edge_uns_generate_sparkplug_ndeath(edge_uns_engine_t *engine, uint8_t *out_buf, size_t buf_cap, size_t *out_len) {
    if (!engine || !out_buf || buf_cap < 32) return -1;

    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, out_buf, buf_cap);
    net_buf_simple_reset(&b);

    /* Payload Timestamp */
    spb_encode_tag(&b, SPB_FIELD_TIMESTAMP, 0);
    spb_encode_varint(&b, engine->birth_timestamp_ms);

    /* NDEATH metric bdSeq = 0 */
    uint8_t sub_raw[64];
    struct net_buf_simple sub;
    net_buf_simple_init_with_data(&sub, sub_raw, sizeof(sub_raw));
    net_buf_simple_reset(&sub);

    spb_encode_string(&sub, SPB_METRIC_NAME, "bdSeq");
    spb_encode_tag(&sub, SPB_METRIC_DATATYPE, 0);
    spb_encode_varint(&sub, (uint64_t)SPB_TYPE_UINT64);
    spb_encode_tag(&sub, SPB_METRIC_ULONG_VAL, 0);
    spb_encode_varint(&sub, 0);

    spb_encode_tag(&b, SPB_FIELD_METRICS, 2);
    spb_encode_varint(&b, sub.len);
    net_buf_simple_add_mem(&b, sub.data, sub.len);

    if (out_len) *out_len = b.len;
    engine->telemetry.sparkplug_ndeath_count++;
    return 0;
}

void edge_uns_get_telemetry(const edge_uns_engine_t *engine, edge_uns_telemetry_t *out_telem) {
    if (engine && out_telem) {
        memcpy(out_telem, &engine->telemetry, sizeof(edge_uns_telemetry_t));
    }
}
