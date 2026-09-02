/**
 * @file datahub_dds.c
 * @brief Cyclone DDS Industrial Distributed Bus & Four-Remote QoS Policy Matrix (ARCH-DDS-01)
 */

#include "datahub_dds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "dds/dds.h"
#include "dlt698_objects.h"
#include "dlt698_objects.c"

typedef struct {
    dds_entity_t participant;
    dds_entity_t topic_yc;
    dds_entity_t topic_yx;
    dds_entity_t topic_yk;
    dds_entity_t writer_yc;
    dds_entity_t writer_yx;
    dds_entity_t writer_yk;
    dds_entity_t reader_yc;
    dds_entity_t reader_yx;
    dds_entity_t reader_yk;
    bool is_initialized;
    uint32_t domain_id;
} dds_backend_ctx_t;

// --- 四遥工业级 QoS 策略矩阵 ---

void *datahub_dds_create_qos_yc(void) {
    dds_qos_t *qos = dds_create_qos();
    if (qos) {
        dds_qset_reliability(qos, DDS_RELIABILITY_BEST_EFFORT, 0);
        dds_qset_durability(qos, DDS_DURABILITY_VOLATILE);
        dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, 10);
    }
    return (void *)qos;
}

void *datahub_dds_create_qos_yx(void) {
    dds_qos_t *qos = dds_create_qos();
    if (qos) {
        dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(50));
        dds_qset_durability(qos, DDS_DURABILITY_TRANSIENT_LOCAL);
        dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, 100);
    }
    return (void *)qos;
}

void *datahub_dds_create_qos_yk(void) {
    dds_qos_t *qos = dds_create_qos();
    if (qos) {
        dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(1000));
        dds_qset_durability(qos, DDS_DURABILITY_VOLATILE);
        dds_qset_history(qos, DDS_HISTORY_KEEP_ALL, 0);
    }
    return (void *)qos;
}

void *datahub_dds_create_qos_yt(void) {
    dds_qos_t *qos = dds_create_qos();
    if (qos) {
        dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(500));
        dds_qset_durability(qos, DDS_DURABILITY_VOLATILE);
        dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, 1);
    }
    return (void *)qos;
}

// 辅助：转换 pipe_msg_t 为 IDL 结构
static inline void _pipe_msg_to_dlt698(const pipe_msg_t *msg, dlt698_Dlt698AttributeData *sample) {
    sample->oad = msg->tag_id;
    sample->class_id = (uint16_t)dlt698_CLASS_DATA;
    sample->attribute_id = 2;
    sample->double_val = msg->payload.val.f;
    sample->timestamp_ms = msg->ts / 1000;
}

static int dds_init_impl(datahub_t *hub, const datahub_config_t *config) {
    if (!hub) return -1;

    hub->total_publishes = 0;
    hub->network_packets_tx = 0;
    hub->network_packets_rx = 0;
    hub->metrics_shadow_writes = 0;
    hub->total_shadow_reads = 0;
    hub->backend_drops = 0;

    dds_backend_ctx_t *ctx = calloc(1, sizeof(dds_backend_ctx_t));
    if (!ctx) return -1;

    uint32_t domain_id = config ? config->dds_domain_id : 0;
    ctx->domain_id = domain_id;
    ctx->participant = dds_create_participant((dds_domainid_t)domain_id, NULL, NULL);
    if (ctx->participant < 0) {
        LOG_WARN(LOG_CAT_CORE, "Cyclone DDS participant creation deferred or failed (rc=%d)", (int)ctx->participant);
        ctx->is_initialized = false;
    } else {
        ctx->is_initialized = true;

        // 1. 创建四遥 Topics
        ctx->topic_yc = dds_create_topic(ctx->participant, &dlt698_Dlt698AttributeData_desc, "EdgeOS_YC_Telemetry", NULL, NULL);
        ctx->topic_yx = dds_create_topic(ctx->participant, &dlt698_Dlt698AttributeData_desc, "EdgeOS_YX_Alarms", NULL, NULL);
        ctx->topic_yk = dds_create_topic(ctx->participant, &dlt698_Dlt698AttributeData_desc, "EdgeOS_YK_Commands", NULL, NULL);

        // 2. 创建 QoS
        dds_qos_t *qos_yc = (dds_qos_t *)datahub_dds_create_qos_yc();
        dds_qos_t *qos_yx = (dds_qos_t *)datahub_dds_create_qos_yx();
        dds_qos_t *qos_yk = (dds_qos_t *)datahub_dds_create_qos_yk();

        // 3. 创建 DataWriters
        if (ctx->topic_yc > 0) ctx->writer_yc = dds_create_writer(ctx->participant, ctx->topic_yc, qos_yc, NULL);
        if (ctx->topic_yx > 0) ctx->writer_yx = dds_create_writer(ctx->participant, ctx->topic_yx, qos_yx, NULL);
        if (ctx->topic_yk > 0) ctx->writer_yk = dds_create_writer(ctx->participant, ctx->topic_yk, qos_yk, NULL);

        // 4. 创建 DataReaders
        if (ctx->topic_yc > 0) ctx->reader_yc = dds_create_reader(ctx->participant, ctx->topic_yc, qos_yc, NULL);
        if (ctx->topic_yx > 0) ctx->reader_yx = dds_create_reader(ctx->participant, ctx->topic_yx, qos_yx, NULL);
        if (ctx->topic_yk > 0) ctx->reader_yk = dds_create_reader(ctx->participant, ctx->topic_yk, qos_yk, NULL);

        if (qos_yc) dds_delete_qos(qos_yc);
        if (qos_yx) dds_delete_qos(qos_yx);
        if (qos_yk) dds_delete_qos(qos_yk);

        LOG_INFO(LOG_CAT_CORE, "Cyclone DDS Participant & Four-Remote QoS initialized on Domain %u", domain_id);
    }

    hub->backend_ctx = ctx;
    hub->backend_type = DATAHUB_BACKEND_DDS;
    return 0;
}

int datahub_dds_publish_yc(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    dds_backend_ctx_t *ctx = (dds_backend_ctx_t *)hub->backend_ctx;
    if (ctx && ctx->is_initialized && ctx->writer_yc > 0) {
        dlt698_Dlt698AttributeData sample;
        _pipe_msg_to_dlt698(msg, &sample);
        dds_write(ctx->writer_yc, &sample);
        hub->total_publishes++;
        hub->network_packets_tx++;
        return 0;
    }
    hub->total_publishes++;
    return 0;
}

int datahub_dds_publish_yx(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    dds_backend_ctx_t *ctx = (dds_backend_ctx_t *)hub->backend_ctx;
    if (ctx && ctx->is_initialized && ctx->writer_yx > 0) {
        dlt698_Dlt698AttributeData sample;
        _pipe_msg_to_dlt698(msg, &sample);
        dds_write(ctx->writer_yx, &sample);
        hub->total_publishes++;
        hub->network_packets_tx++;
        return 0;
    }
    hub->total_publishes++;
    return 0;
}

int datahub_dds_publish_yk(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    dds_backend_ctx_t *ctx = (dds_backend_ctx_t *)hub->backend_ctx;
    if (ctx && ctx->is_initialized && ctx->writer_yk > 0) {
        dlt698_Dlt698AttributeData sample;
        _pipe_msg_to_dlt698(msg, &sample);
        dds_write(ctx->writer_yk, &sample);
        hub->total_publishes++;
        hub->network_packets_tx++;
        return 0;
    }
    hub->total_publishes++;
    return 0;
}

static int dds_publish_impl(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    if (msg->type == PIPEMSG_TYPE_EVENT || (msg->tag_id >= 0x30000000 && msg->tag_id < 0x40000000)) {
        return datahub_dds_publish_yx(hub, msg);
    }
    if (msg->attr.priority >= 10 || (msg->tag_id >= 0x40000000 && msg->tag_id < 0x50000000)) {
        return datahub_dds_publish_yk(hub, msg);
    }
    return datahub_dds_publish_yc(hub, msg);
}

static int dds_write_shadow_impl(datahub_t *hub, const pipe_msg_t *msg) {
    return dds_publish_impl(hub, msg);
}

static int dds_read_shadow_impl(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (!hub || !out_msg) return -1;
    hub->total_shadow_reads++;
    (void)tag_id;
    return 0;
}

static int dds_subscribe_impl(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub) return -1;
    hub->legacy_cb = cb;
    hub->legacy_user_data = user_data;
    return 0;
}

static int dds_query_snapshot_impl(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg) {
    return dds_read_shadow_impl(hub, tag_id, out_msg);
}

static int dds_get_stats_impl(datahub_t *hub, datahub_backend_stats_t *stats) {
    if (!hub || !stats) return -1;
    stats->total_publishes = hub->total_publishes;
    stats->total_shadow_writes = hub->metrics_shadow_writes;
    stats->total_shadow_reads = hub->total_shadow_reads;
    stats->network_packets_tx = hub->network_packets_tx;
    stats->network_packets_rx = hub->network_packets_rx;
    stats->drops = hub->backend_drops;
    return 0;
}

static void dds_destroy_impl(datahub_t *hub) {
    if (!hub) return;
    dds_backend_ctx_t *ctx = (dds_backend_ctx_t *)hub->backend_ctx;
    if (ctx) {
        if (ctx->is_initialized && ctx->participant > 0) {
            dds_delete(ctx->participant);
        }
        free(ctx);
        hub->backend_ctx = NULL;
    }
}

const datahub_ops_t g_datahub_dds_ops = {
    .init = dds_init_impl,
    .publish = dds_publish_impl,
    .write_shadow = dds_write_shadow_impl,
    .read_shadow = dds_read_shadow_impl,
    .subscribe = dds_subscribe_impl,
    .query_snapshot = dds_query_snapshot_impl,
    .get_stats = dds_get_stats_impl,
    .destroy = dds_destroy_impl
};

int datahub_dds_init(datahub_t *hub, const datahub_config_t *config) {
    if (!hub) return -1;
    hub->ops = &g_datahub_dds_ops;
    return dds_init_impl(hub, config);
}
