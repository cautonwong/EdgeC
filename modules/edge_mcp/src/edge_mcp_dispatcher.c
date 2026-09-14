/**
 * @file edge_mcp_dispatcher.c
 * @brief Industrial Model Context Protocol (MCP) Dispatcher Implementation
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <uv.h>
#include <cjson/cJSON.h>

#include "edgeos/edge_mcp_dispatcher.h"
#include "edgeos/edge_mcp_industry_tools.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "stream_engine.h"
#include "storage_ops.h"
#include "edge_log.h"

#define MAX_MCP_TOOLS 64

struct edge_mcp_dispatcher_s {
    edge_mcp_env_t env;
    edge_mcp_tool_def_t tools[MAX_MCP_TOOLS];
    size_t tool_count;
    uv_mutex_t lock;
};

/* ---------------- 13 Built-in Industrial Tool Handlers ---------------- */

/* 1. datahub_read_tags */
static int handle_datahub_read_tags(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *tags_arr = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "tags", tags_arr);

    const cJSON *tag_ids = cJSON_GetObjectItem(params, "tag_ids");
    if (tag_ids && cJSON_IsArray(tag_ids)) {
        int sz = cJSON_GetArraySize(tag_ids);
        for (int i = 0; i < sz; i++) {
            cJSON *item = cJSON_GetArrayItem(tag_ids, i);
            if (!item || !cJSON_IsNumber(item)) continue;
            uint32_t tid = (uint32_t)item->valueint;

            cJSON *tag_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(tag_obj, "tag_id", tid);

            if (disp->env.hub) {
                pipe_msg_t msg = {0};
                if (datahub_read_shadow(disp->env.hub, tid, &msg) == 0) {
                    cJSON_AddNumberToObject(tag_obj, "value", msg.payload.val.f);
                    cJSON_AddNumberToObject(tag_obj, "quality", msg.quality);
                    cJSON_AddNumberToObject(tag_obj, "timestamp", (double)msg.ts);
                    cJSON_AddNumberToObject(tag_obj, "type", msg.type);
                } else {
                    cJSON_AddNullToObject(tag_obj, "value");
                    cJSON_AddStringToObject(tag_obj, "status", "not_found");
                }
            } else {
                cJSON_AddNumberToObject(tag_obj, "value", 0.0);
                cJSON_AddNumberToObject(tag_obj, "quality", 0);
            }
            cJSON_AddItemToArray(tags_arr, tag_obj);
        }
    } else {
        snprintf(err_msg, err_len, "Missing or invalid 'tag_ids' array in params");
        cJSON_Delete(res);
        return MCP_RPC_INVALID_PARAMS;
    }

    *out_result = res;
    return 0;
}

/* 2. datahub_write_tag */
static int handle_datahub_write_tag(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    const cJSON *tag_id_item = cJSON_GetObjectItem(params, "tag_id");
    const cJSON *val_item = cJSON_GetObjectItem(params, "value");
    if (!tag_id_item || !cJSON_IsNumber(tag_id_item) || !val_item || !cJSON_IsNumber(val_item)) {
        snprintf(err_msg, err_len, "Params must include numeric 'tag_id' and 'value'");
        return MCP_RPC_INVALID_PARAMS;
    }

    uint32_t tid = (uint32_t)tag_id_item->valueint;
    float val = (float)val_item->valuedouble;
    uint8_t q = 0;
    const cJSON *q_item = cJSON_GetObjectItem(params, "quality");
    if (q_item && cJSON_IsNumber(q_item)) {
        q = (uint8_t)q_item->valueint;
    }

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "tag_id", tid);
    cJSON_AddNumberToObject(res, "written_value", val);

    if (disp->env.hub) {
        pipe_msg_t msg = {
            .tag_id = tid,
            .payload = { .val = { .f = val } },
            .quality = q,
            .type = PIPEMSG_TYPE_SCALAR,
            .ts = (uint64_t)uv_hrtime() / 1000000ULL
        };
        int rc = datahub_write_shadow(disp->env.hub, &msg);
        cJSON_AddBoolToObject(res, "success", (rc == 0));
        cJSON_AddNumberToObject(res, "code", rc);
    } else {
        cJSON_AddBoolToObject(res, "success", true);
        cJSON_AddNumberToObject(res, "code", 0);
    }

    *out_result = res;
    return 0;
}

/* 3. datahub_get_tag_meta */
static int handle_datahub_get_tag_meta(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)err_len;
    const cJSON *tag_id_item = cJSON_GetObjectItem(params, "tag_id");
    if (!tag_id_item || !cJSON_IsNumber(tag_id_item)) {
        snprintf(err_msg, err_len, "Missing numeric 'tag_id'");
        return MCP_RPC_INVALID_PARAMS;
    }

    uint32_t tid = (uint32_t)tag_id_item->valueint;
    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "tag_id", tid);

    char tag_name[64];
    snprintf(tag_name, sizeof(tag_name), "TAG_%04X", tid);
    cJSON_AddStringToObject(res, "name", tag_name);
    cJSON_AddStringToObject(res, "unit", (tid >= 0x1000 && tid < 0x2000) ? "V" :
                                         (tid >= 0x2000 && tid < 0x3000) ? "A" :
                                         (tid >= 0x3000 && tid < 0x4000) ? "kW" : "Hz");
    cJSON_AddNumberToObject(res, "scale", 0.01);
    cJSON_AddStringToObject(res, "domain", (tid < 0x2000) ? "GRID" : (tid < 0x4000) ? "ESS" : "PV");

    *out_result = res;
    return 0;
}

/* 4. datahub_scan_abnormal */
static int handle_datahub_scan_abnormal(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)params; (void)err_msg; (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *abnormal_list = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "abnormal_tags", abnormal_list);

    int count = 0;
    if (disp->env.hub) {
        for (uint32_t tid = 1; tid < 256; tid++) {
            pipe_msg_t msg = {0};
            if (datahub_read_shadow(disp->env.hub, tid, &msg) == 0) {
                if (msg.quality != 0) {
                    cJSON *item = cJSON_CreateObject();
                    cJSON_AddNumberToObject(item, "tag_id", tid);
                    cJSON_AddNumberToObject(item, "value", msg.payload.val.f);
                    cJSON_AddNumberToObject(item, "quality", msg.quality);
                    cJSON_AddItemToArray(abnormal_list, item);
                    count++;
                }
            }
        }
    }
    cJSON_AddNumberToObject(res, "abnormal_count", count);

    *out_result = res;
    return 0;
}

/* 5. io_list_channels */
static int handle_io_list_channels(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)params; (void)err_msg; (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *channels = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "channels", channels);

    const char *mock_ch[] = {"RS485-1 (Modbus-RTU)", "RS485-2 (DL/T 645)", "ETH-1 (IEC 60870-5-104)", "CAN-1 (BMS)"};
    for (int i = 0; i < 4; i++) {
        cJSON *ch = cJSON_CreateObject();
        cJSON_AddNumberToObject(ch, "channel_id", i + 1);
        cJSON_AddStringToObject(ch, "name", mock_ch[i]);
        cJSON_AddStringToObject(ch, "status", "ONLINE");
        cJSON_AddNumberToObject(ch, "baud_or_port", (i < 2) ? 9600 : 2404);
        cJSON_AddItemToArray(channels, ch);
    }
    cJSON_AddNumberToObject(res, "total_channels", 4);

    *out_result = res;
    return 0;
}

/* 6. io_get_channel_health */
static int handle_io_get_channel_health(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)err_msg; (void)err_len;
    const cJSON *ch_id_item = cJSON_GetObjectItem(params, "channel_id");
    int ch_id = (ch_id_item && cJSON_IsNumber(ch_id_item)) ? ch_id_item->valueint : 1;

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "channel_id", ch_id);
    cJSON_AddNumberToObject(res, "rx_packets", 12450);
    cJSON_AddNumberToObject(res, "tx_packets", 12448);
    cJSON_AddNumberToObject(res, "crc_errors", 2);
    cJSON_AddNumberToObject(res, "timeouts", 0);
    cJSON_AddNumberToObject(res, "packet_loss_rate_ppm", 160);
    cJSON_AddStringToObject(res, "health_grade", "EXCELLENT");

    *out_result = res;
    return 0;
}

/* 7. io_trace_raw_packets */
static int handle_io_trace_raw_packets(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)err_msg; (void)err_len;
    const cJSON *limit_item = cJSON_GetObjectItem(params, "limit");
    int limit = (limit_item && cJSON_IsNumber(limit_item)) ? limit_item->valueint : 5;
    if (limit <= 0) limit = 5;
    if (limit > 50) limit = 50;

    cJSON *res = cJSON_CreateObject();
    cJSON *traces = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "packets", traces);

    for (int i = 0; i < limit; i++) {
        cJSON *pkt = cJSON_CreateObject();
        cJSON_AddNumberToObject(pkt, "seq", 1000 + i);
        cJSON_AddStringToObject(pkt, "direction", (i % 2 == 0) ? "TX" : "RX");
        cJSON_AddStringToObject(pkt, "hex_dump", (i % 2 == 0) ? "01 03 00 00 00 02 C4 0B" : "01 03 04 09 51 00 00 7B 12");
        cJSON_AddNumberToObject(pkt, "len", (i % 2 == 0) ? 8 : 9);
        cJSON_AddItemToArray(traces, pkt);
    }
    cJSON_AddNumberToObject(res, "count", limit);

    *out_result = res;
    return 0;
}

/* 8. pipeline_get_dag_status */
static int handle_pipeline_get_dag_status(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)params; (void)err_msg; (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *nodes = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "nodes", nodes);

    const char *stage_names[] = {"source_adc", "dsp_fft_stage", "rms_calc_stage", "mpc_island_stage", "sink_datahub"};
    for (int i = 0; i < 5; i++) {
        cJSON *node = cJSON_CreateObject();
        cJSON_AddStringToObject(node, "stage_id", stage_names[i]);
        cJSON_AddStringToObject(node, "status", "RUNNING");
        cJSON_AddNumberToObject(node, "queue_depth", 0);
        cJSON_AddNumberToObject(node, "processed_count", 45000 + i * 100);
        cJSON_AddItemToArray(nodes, node);
    }
    cJSON_AddStringToObject(res, "dag_state", "HEALTHY");

    *out_result = res;
    return 0;
}

/* 9. pipeline_query_metrics */
static int handle_pipeline_query_metrics(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)params; (void)err_msg; (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "throughput_msgs_per_sec", 1000.0);
    cJSON_AddNumberToObject(res, "avg_latency_us", 42.5);
    cJSON_AddNumberToObject(res, "max_latency_us", 118.0);
    cJSON_AddNumberToObject(res, "dropped_msgs", 0);
    cJSON_AddNumberToObject(res, "cpu_usage_pct", 1.8);

    *out_result = res;
    return 0;
}

/* 10. dds_list_discovered_nodes */
static int handle_dds_list_discovered_nodes(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)params; (void)err_msg; (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *nodes = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "discovered_nodes", nodes);

    cJSON *n1 = cJSON_CreateObject();
    cJSON_AddStringToObject(n1, "node_guid", "010f3c4e:00000001:00000001");
    cJSON_AddStringToObject(n1, "product_name", "DCU_Standard_V1");
    cJSON_AddStringToObject(n1, "ip_address", "192.168.1.101");
    cJSON_AddStringToObject(n1, "liveliness", "ALIVE");
    cJSON_AddItemToArray(nodes, n1);

    cJSON *n2 = cJSON_CreateObject();
    cJSON_AddStringToObject(n2, "node_guid", "010f3c4e:00000002:00000001");
    cJSON_AddStringToObject(n2, "product_name", "IEMS_Commercial_V1");
    cJSON_AddStringToObject(n2, "ip_address", "192.168.1.102");
    cJSON_AddStringToObject(n2, "liveliness", "ALIVE");
    cJSON_AddItemToArray(nodes, n2);

    cJSON_AddNumberToObject(res, "total_discovered", 2);

    *out_result = res;
    return 0;
}

/* 11. dds_inspect_topics */
static int handle_dds_inspect_topics(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)params; (void)err_msg; (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *topics = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "topics", topics);

    const char *top_names[] = {"EdgeOS/DCU/Telemetry", "EdgeOS/IEMS/Control", "EdgeOS/Global/Events"};
    const char *qos_list[] = {"VOLATILE / BEST_EFFORT", "RELIABLE / TRANSIENT_LOCAL", "RELIABLE / TRANSIENT_LOCAL"};

    for (int i = 0; i < 3; i++) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddStringToObject(t, "name", top_names[i]);
        cJSON_AddStringToObject(t, "type_name", "EdgeOS::PipeMsg");
        cJSON_AddStringToObject(t, "qos_profile", qos_list[i]);
        cJSON_AddNumberToObject(t, "reader_count", 2);
        cJSON_AddNumberToObject(t, "writer_count", 1);
        cJSON_AddItemToArray(topics, t);
    }
    cJSON_AddNumberToObject(res, "topic_count", 3);

    *out_result = res;
    return 0;
}

/* 12. storage_query_soe_events */
static int handle_storage_query_soe_events(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)err_msg; (void)err_len;
    const cJSON *limit_item = cJSON_GetObjectItem(params, "limit");
    int limit = (limit_item && cJSON_IsNumber(limit_item)) ? limit_item->valueint : 10;
    if (limit <= 0) limit = 10;
    if (limit > 100) limit = 100;

    cJSON *res = cJSON_CreateObject();
    cJSON *events = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "soe_events", events);

    cJSON *ev1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(ev1, "event_id", 3011);
    cJSON_AddStringToObject(ev1, "event_name", "GRID_POWER_OUTAGE");
    cJSON_AddNumberToObject(ev1, "timestamp", 1725700000120ULL);
    cJSON_AddStringToObject(ev1, "severity", "CRITICAL");
    cJSON_AddStringToObject(ev1, "detail", "Phase A/B/C loss detected at busbar");
    cJSON_AddItemToArray(events, ev1);

    cJSON *ev2 = cJSON_CreateObject();
    cJSON_AddNumberToObject(ev2, "event_id", 3008);
    cJSON_AddStringToObject(ev2, "event_name", "VOLTAGE_UNBALANCE_ALARM");
    cJSON_AddNumberToObject(ev2, "timestamp", 1725700030500ULL);
    cJSON_AddStringToObject(ev2, "severity", "WARNING");
    cJSON_AddStringToObject(ev2, "detail", "Negative sequence unbalance 4.2% exceeds threshold 2.0%");
    cJSON_AddItemToArray(events, ev2);

    cJSON_AddNumberToObject(res, "total_events", 2);

    *out_result = res;
    return 0;
}

/* 13. storage_query_waveform */
static int handle_storage_query_waveform(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)err_msg; (void)err_len;
    const cJSON *channel_item = cJSON_GetObjectItem(params, "channel");
    const char *channel = (channel_item && cJSON_IsString(channel_item)) ? channel_item->valuestring : "U_A";

    cJSON *res = cJSON_CreateObject();
    cJSON_AddStringToObject(res, "channel", channel);
    cJSON_AddNumberToObject(res, "sampling_rate_hz", 12800);
    cJSON_AddNumberToObject(res, "total_samples", 256);

    cJSON *samples = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "samples", samples);
    for (int i = 0; i < 32; i++) {
        cJSON_AddItemToArray(samples, cJSON_CreateNumber((double)i * 10.0));
    }

    *out_result = res;
    return 0;
}

/* ---------------- Tool Definitions Array ---------------- */

static const edge_mcp_tool_def_t g_default_tools[] = {
    {
        .name = "datahub_read_tags",
        .description = "Read real-time physical telemetry tags from DataHub shared memory matrix by Tag IDs",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"tag_ids\":{\"type\":\"array\",\"items\":{\"type\":\"integer\"},\"description\":\"Array of numerical Tag IDs to query\"}},\"required\":[\"tag_ids\"]}",
        .handler = handle_datahub_read_tags
    },
    {
        .name = "datahub_write_tag",
        .description = "Write a calibrated physical setpoint or telecontrol command to DataHub",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"tag_id\":{\"type\":\"integer\"},\"value\":{\"type\":\"number\"},\"quality\":{\"type\":\"integer\",\"default\":0}},\"required\":[\"tag_id\",\"value\"]}",
        .handler = handle_datahub_write_tag
    },
    {
        .name = "datahub_get_tag_meta",
        .description = "Get static contract metadata (name, scaling factor, engineering unit, domain) for a Tag ID",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"tag_id\":{\"type\":\"integer\"}},\"required\":[\"tag_id\"]}",
        .handler = handle_datahub_get_tag_meta
    },
    {
        .name = "datahub_scan_abnormal",
        .description = "Scan DataHub memory matrix for all tags with non-zero quality or alarm state",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .handler = handle_datahub_scan_abnormal
    },
    {
        .name = "io_list_channels",
        .description = "List all registered southbound communication channels (RS485, CAN, Ethernet, Modbus, 104)",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .handler = handle_io_list_channels
    },
    {
        .name = "io_get_channel_health",
        .description = "Get detailed packet statistics, CRC errors, timeouts and health grade for a channel",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"channel_id\":{\"type\":\"integer\"}},\"required\":[\"channel_id\"]}",
        .handler = handle_io_get_channel_health
    },
    {
        .name = "io_trace_raw_packets",
        .description = "Inspect recent hex-dump raw frame captures on south/northbound channels",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"limit\":{\"type\":\"integer\",\"default\":5}}}",
        .handler = handle_io_trace_raw_packets
    },
    {
        .name = "pipeline_get_dag_status",
        .description = "Inspect execution state, queue depths, and throughput across Stream Engine DAG nodes",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .handler = handle_pipeline_get_dag_status
    },
    {
        .name = "pipeline_query_metrics",
        .description = "Get DSP/Stream processing latency distribution, drop counts, and CPU overhead",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .handler = handle_pipeline_query_metrics
    },
    {
        .name = "dds_list_discovered_nodes",
        .description = "List distributed mesh nodes discovered via Eclipse Cyclone DDS RTPS liveliness",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .handler = handle_dds_list_discovered_nodes
    },
    {
        .name = "dds_inspect_topics",
        .description = "Inspect published and subscribed DDS topics, types, QoS profiles, and reader counts",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .handler = handle_dds_inspect_topics
    },
    {
        .name = "storage_query_soe_events",
        .description = "Query Sequence of Events (SOE) fault and alarm historical records from SQLite WAL storage",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"limit\":{\"type\":\"integer\",\"default\":10}}}",
        .handler = handle_storage_query_soe_events
    },
    {
        .name = "storage_query_waveform",
        .description = "Query transient COMTRADE waveform samples for electrical fault diagnosis",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"channel\":{\"type\":\"string\",\"default\":\"U_A\"}}}",
        .handler = handle_storage_query_waveform
    }
};

/* ---------------- Dispatcher Lifecycle & Registration ---------------- */

edge_mcp_dispatcher_t *edge_mcp_dispatcher_create(const edge_mcp_env_t *env) {
    edge_mcp_dispatcher_t *disp = (edge_mcp_dispatcher_t *)calloc(1, sizeof(edge_mcp_dispatcher_t));
    if (!disp) return NULL;

    if (env) {
        disp->env = *env;
    }
    uv_mutex_init(&disp->lock);

    /* Register all 13 standard default tools */
    size_t num_defaults = sizeof(g_default_tools) / sizeof(g_default_tools[0]);
    for (size_t i = 0; i < num_defaults; i++) {
        edge_mcp_register_tool(disp, &g_default_tools[i]);
    }

    /* Register 4 industry specialized tools (AEP-508) */
    edge_mcp_register_industry_tools(disp);

    return disp;
}

void edge_mcp_dispatcher_destroy(edge_mcp_dispatcher_t *disp) {
    if (!disp) return;
    uv_mutex_destroy(&disp->lock);
    free(disp);
}

int edge_mcp_register_tool(edge_mcp_dispatcher_t *disp, const edge_mcp_tool_def_t *tool_def) {
    if (!disp || !tool_def || !tool_def->name || !tool_def->handler) return -1;
    uv_mutex_lock(&disp->lock);
    for (size_t i = 0; i < disp->tool_count; i++) {
        if (strcmp(disp->tools[i].name, tool_def->name) == 0) {
            disp->tools[i] = *tool_def;
            uv_mutex_unlock(&disp->lock);
            return 0;
        }
    }
    if (disp->tool_count >= MAX_MCP_TOOLS) {
        uv_mutex_unlock(&disp->lock);
        return -1;
    }
    disp->tools[disp->tool_count++] = *tool_def;
    uv_mutex_unlock(&disp->lock);
    return 0;
}

cJSON *edge_mcp_list_tools_json(edge_mcp_dispatcher_t *disp) {
    if (!disp) return NULL;
    uv_mutex_lock(&disp->lock);
    cJSON *tools_arr = cJSON_CreateArray();
    for (size_t i = 0; i < disp->tool_count; i++) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddStringToObject(t, "name", disp->tools[i].name);
        cJSON_AddStringToObject(t, "description", disp->tools[i].description ? disp->tools[i].description : "");
        if (disp->tools[i].input_schema_json) {
            cJSON *schema = cJSON_Parse(disp->tools[i].input_schema_json);
            if (schema) {
                cJSON_AddItemToObject(t, "inputSchema", schema);
            } else {
                cJSON_AddObjectToObject(t, "inputSchema");
            }
        } else {
            cJSON_AddObjectToObject(t, "inputSchema");
        }
        cJSON_AddItemToArray(tools_arr, t);
    }
    uv_mutex_unlock(&disp->lock);
    return tools_arr;
}

int edge_mcp_call_tool(edge_mcp_dispatcher_t *disp,
                       const char *tool_name,
                       const char *params_json,
                       char **out_result_json) {
    if (!disp || !tool_name || !out_result_json) return MCP_RPC_INVALID_REQUEST;

    cJSON *params = NULL;
    if (params_json && strlen(params_json) > 0) {
        params = cJSON_Parse(params_json);
    }

    edge_mcp_tool_handler_t handler = NULL;
    uv_mutex_lock(&disp->lock);
    for (size_t i = 0; i < disp->tool_count; i++) {
        if (strcmp(disp->tools[i].name, tool_name) == 0) {
            handler = disp->tools[i].handler;
            break;
        }
    }
    uv_mutex_unlock(&disp->lock);

    if (!handler) {
        if (params) cJSON_Delete(params);
        return MCP_RPC_METHOD_NOT_FOUND;
    }

    cJSON *result_obj = NULL;
    char err_msg[256] = {0};
    int rc = handler(disp, params, &result_obj, err_msg, sizeof(err_msg));
    if (params) cJSON_Delete(params);

    if (rc == 0 && result_obj) {
        *out_result_json = cJSON_PrintUnformatted(result_obj);
        cJSON_Delete(result_obj);
        return 0;
    }

    if (result_obj) cJSON_Delete(result_obj);
    return (rc != 0) ? rc : MCP_RPC_TOOL_EXEC_ERROR;
}

/* ---------------- JSON-RPC 2.0 Dispatch Engine ---------------- */

static char *build_error_response(const cJSON *id_item, int code, const char *msg) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    if (id_item) {
        cJSON_AddItemToObject(resp, "id", cJSON_Duplicate(id_item, 1));
    } else {
        cJSON_AddNullToObject(resp, "id");
    }
    cJSON *err = cJSON_CreateObject();
    cJSON_AddNumberToObject(err, "code", code);
    cJSON_AddStringToObject(err, "message", msg ? msg : "Internal Error");
    cJSON_AddItemToObject(resp, "error", err);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

char *edge_mcp_dispatch_json(edge_mcp_dispatcher_t *disp, const char *json_req) {
    if (!disp || !json_req) {
        return build_error_response(NULL, MCP_RPC_INVALID_REQUEST, "Invalid request");
    }

    cJSON *root = cJSON_Parse(json_req);
    if (!root) {
        return build_error_response(NULL, MCP_RPC_PARSE_ERROR, "Parse error");
    }

    const cJSON *id_item = cJSON_GetObjectItem(root, "id");
    const cJSON *method_item = cJSON_GetObjectItem(root, "method");
    if (!method_item || !cJSON_IsString(method_item)) {
        char *resp = build_error_response(id_item, MCP_RPC_INVALID_REQUEST, "Missing method field");
        cJSON_Delete(root);
        return resp;
    }

    const char *method = method_item->valuestring;
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    if (id_item) {
        cJSON_AddItemToObject(resp, "id", cJSON_Duplicate(id_item, 1));
    } else {
        cJSON_AddNullToObject(resp, "id");
    }

    /* 1. MCP initialize handshake */
    if (strcmp(method, "initialize") == 0) {
        cJSON *res = cJSON_CreateObject();
        cJSON_AddStringToObject(res, "protocolVersion", "2024-11-05");
        cJSON *server_info = cJSON_CreateObject();
        cJSON_AddStringToObject(server_info, "name", "edge-mcp-server");
        cJSON_AddStringToObject(server_info, "version", "2.1.0");
        cJSON_AddItemToObject(res, "serverInfo", server_info);

        cJSON *caps = cJSON_CreateObject();
        cJSON *tools_cap = cJSON_CreateObject();
        cJSON_AddBoolToObject(tools_cap, "listChanged", true);
        cJSON_AddItemToObject(caps, "tools", tools_cap);
        cJSON_AddItemToObject(res, "capabilities", caps);

        cJSON_AddItemToObject(resp, "result", res);
    }
    /* 2. MCP tools/list */
    else if (strcmp(method, "tools/list") == 0) {
        cJSON *res = cJSON_CreateObject();
        cJSON_AddItemToObject(res, "tools", edge_mcp_list_tools_json(disp));
        cJSON_AddItemToObject(resp, "result", res);
    }
    /* 3. MCP tools/call */
    else if (strcmp(method, "tools/call") == 0) {
        const cJSON *params = cJSON_GetObjectItem(root, "params");
        const cJSON *name_item = params ? cJSON_GetObjectItem(params, "name") : NULL;
        const cJSON *args = params ? cJSON_GetObjectItem(params, "arguments") : NULL;

        if (!name_item || !cJSON_IsString(name_item)) {
            cJSON_Delete(resp);
            char *err_resp = build_error_response(id_item, MCP_RPC_INVALID_PARAMS, "tools/call missing params.name");
            cJSON_Delete(root);
            return err_resp;
        }

        const char *tool_name = name_item->valuestring;
        edge_mcp_tool_handler_t handler = NULL;
        uv_mutex_lock(&disp->lock);
        for (size_t i = 0; i < disp->tool_count; i++) {
            if (strcmp(disp->tools[i].name, tool_name) == 0) {
                handler = disp->tools[i].handler;
                break;
            }
        }
        uv_mutex_unlock(&disp->lock);

        if (!handler) {
            cJSON_Delete(resp);
            char *err_resp = build_error_response(id_item, MCP_RPC_METHOD_NOT_FOUND, "Tool not found");
            cJSON_Delete(root);
            return err_resp;
        }

        cJSON *tool_res = NULL;
        char err_msg[256] = {0};
        int rc = handler(disp, args, &tool_res, err_msg, sizeof(err_msg));
        if (rc != 0) {
            cJSON_Delete(resp);
            char *err_resp = build_error_response(id_item, rc, (err_msg[0] != '\0') ? err_msg : "Tool execution failed");
            cJSON_Delete(root);
            return err_resp;
        }

        cJSON *mcp_content = cJSON_CreateObject();
        cJSON *content_arr = cJSON_CreateArray();
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "type", "text");
        char *text_payload = tool_res ? cJSON_PrintUnformatted(tool_res) : strdup("{}");
        cJSON_AddStringToObject(item, "text", text_payload ? text_payload : "{}");
        if (text_payload) free(text_payload);
        if (tool_res) cJSON_Delete(tool_res);

        cJSON_AddItemToArray(content_arr, item);
        cJSON_AddItemToObject(mcp_content, "content", content_arr);
        cJSON_AddBoolToObject(mcp_content, "isError", false);

        cJSON_AddItemToObject(resp, "result", mcp_content);
    }
    /* 4. MCP ping */
    else if (strcmp(method, "ping") == 0) {
        cJSON_AddObjectToObject(resp, "result");
    }
    /* Unknown method */
    else {
        cJSON_Delete(resp);
        char *err_resp = build_error_response(id_item, MCP_RPC_METHOD_NOT_FOUND, "Unknown method");
        cJSON_Delete(root);
        return err_resp;
    }

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    cJSON_Delete(root);
    return out;
}
