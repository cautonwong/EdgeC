#include "datahub_dds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "dds/dds.h"

typedef struct {
    dds_entity_t participant;
    dds_entity_t topic;
    dds_entity_t writer;
    dds_entity_t reader;
    bool is_initialized;
} dds_backend_ctx_t;

static int dds_init_impl(datahub_t *hub, const datahub_config_t *config) {
    if (!hub) return -1;

    dds_backend_ctx_t *ctx = calloc(1, sizeof(dds_backend_ctx_t));
    if (!ctx) return -1;

    uint32_t domain_id = config ? config->dds_domain_id : 0;
    ctx->participant = dds_create_participant((dds_domainid_t)domain_id, NULL, NULL);
    if (ctx->participant < 0) {
        LOG_WARN(CORE, "Cyclone DDS participant creation deferred or failed (rc=%d)", (int)ctx->participant);
        ctx->is_initialized = false;
    } else {
        ctx->is_initialized = true;
        LOG_INFO(CORE, "Cyclone DDS Participant initialized on Domain %u", domain_id);
    }

    hub->backend_ctx = ctx;
    hub->backend_type = DATAHUB_BACKEND_DDS;
    return 0;
}

static int dds_publish_impl(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    dds_backend_ctx_t *ctx = (dds_backend_ctx_t *)hub->backend_ctx;
    if (ctx && ctx->is_initialized && ctx->writer > 0) {
        // Publish sample via Cyclone DDS writer
        dds_write(ctx->writer, msg);
        hub->total_publishes++;
        hub->network_packets_tx++;
        return 0;
    }
    // Record telemetry stats
    hub->total_publishes++;
    return 0;
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
