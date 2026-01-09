#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "contract_data_types.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "iec101_link.h"
#include "iec_asdu.h"
#include "net_buf.h"

extern const iec101_map_t* const g_iec101_gi_list[];
extern const uint32_t g_iec101_gi_len;

typedef struct {
    iec101_link_mgr_t link;
    iec_addr_config_t addr_conf;
    uint16_t casdu;
    datahub_t *hub;
    double *last_values;
} iec101_ctx_t;

static int iec101_v3_on_init(edge_driver_ctx_t *ctx) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(iec101_ctx_t));
    priv->addr_conf.cot_len = 1;
    priv->addr_conf.casdu_len = 1;
    priv->addr_conf.ioa_len = 2;
    priv->link.link_addr = (uint16_t)ctx->device_id;
    priv->link.addr_len = 1;
    priv->casdu = (uint16_t)ctx->device_id;
    priv->hub = (datahub_t *)ctx->user_data;

    if (ctx->role == PROTOCOL_ROLE_SLAVE) {
        priv->last_values = calloc(g_iec101_gi_len > 0 ? g_iec101_gi_len : 1, sizeof(double));
    }
    return 0;
}

static void iec101_v3_on_cleanup(edge_driver_ctx_t *ctx) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    if (priv->last_values) free(priv->last_values);
}

static int iec101_v3_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    if (ctx->role == PROTOCOL_ROLE_SLAVE) {
        net_buf_t *batch = NULL;
        int count = 0;
        for (uint32_t i = 0; i < g_iec101_gi_len; i++) {
            const iec101_map_t *map = g_iec101_gi_list[i];
            pipe_msg_t val;
            if (datahub_read_shadow(priv->hub, map->tag_id, &val) == 0) {
                if (fabs(val.payload.val.f - priv->last_values[i]) > (double)map->dead_band) {
                    priv->last_values[i] = val.payload.val.f;
                    if (!batch) {
                        batch = net_buf_alloc(NULL, 0);
                        iec_asdu_init(batch, (iec_type_id_t)map->type_id, false, 0, IEC_COT_SPONT, priv->casdu, &priv->addr_conf);
                    }
                    if (iec_asdu_add_any(batch, map->type_id, map->key, val.payload.val.f / (double)map->scaler, val.ts, map->flags, (iec_qds_t){0}, &priv->addr_conf) == 0) {
                        count++;
                    }
                    if (count >= 10) break;
                }
            }
        }
        if (batch) {
            batch->b.data[1] = (uint8_t)((batch->b.data[1] & 0x80) | (count & 0x7F));
            net_buf_t *wrapped = net_buf_alloc(NULL, 0);
            iec101_link_build_variable(&priv->link, (iec101_ctrl_t){.prm=0, .func=8}, batch, wrapped);
            net_buf_unref(batch);
            *out_tx = wrapped;
            return 0;
        }
    }
    return 0;
}

static int iec101_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs; (void)async_count;
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;
    if (b->len < 1) return EDGE_ERR_INCOMPLETE;
    if (b->data[0] == 0x68) {
        if (b->len < 4) return EDGE_ERR_INCOMPLETE;
        uint8_t len = b->data[1];
        if (b->len < (size_t)(len + 6)) return EDGE_ERR_INCOMPLETE;
        *consumed = (size_t)(len + 6);
        net_buf_simple_pull(b, (size_t)(4 + 1 + priv->link.addr_len));
        // 简化 GI 逻辑实现
        for (uint32_t i = 0; i < g_iec101_gi_len; i++) {
            const iec101_map_t *map = g_iec101_gi_list[i];
            pipe_msg_t val;
            if (datahub_read_shadow(priv->hub, map->tag_id, &val) == 0) {
                net_buf_t *df = net_buf_alloc(in->pool, 0);
                iec_asdu_init(df, (iec_type_id_t)map->type_id, false, 1, IEC_COT_INTROGEN, priv->casdu, &priv->addr_conf);
                iec_asdu_add_any(df, map->type_id, map->key, val.payload.val.f / (double)map->scaler, val.ts, map->flags, (iec_qds_t){0}, &priv->addr_conf);
                iec101_link_build_variable(&priv->link, (iec101_ctrl_t){.prm=0, .func=8}, df, resp_out);
                net_buf_unref(df);
            }
        }
    }
    return 0;
}

edge_driver_t iec101_driver = {
    .name = "IEC101",
    .ctx_size = sizeof(iec101_ctx_t),
    .on_init = iec101_v3_on_init,
    .on_cleanup = iec101_v3_on_cleanup,
    .on_tick = iec101_v3_on_tick,
    .handle_rx = iec101_v3_handle_rx,
};

int iec101_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &iec101_driver);
}