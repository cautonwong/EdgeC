/**
 * @file driver_iec104.c
 * @brief 工业级 IEC 60870-5-104 驱动 (注入驱动版)
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "contract_data_types.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "iec104_apci.h"
#include "iec_asdu.h"
#include "net_buf.h"

typedef struct {
    iec104_apci_mgr_t apci;
    uint16_t casdu;
    datahub_t *hub;
    double *last_values;
} iec104_ctx_t;

static int iec104_v3_on_init(edge_driver_ctx_t *ctx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(iec104_ctx_t));
    iec104_apci_init(&priv->apci);
    priv->casdu = (uint16_t)ctx->device_id;
    priv->hub = (datahub_t *)ctx->user_data;

    if (ctx->role == PROTOCOL_ROLE_SLAVE) {
        uint32_t count = datahub_get_metadata_count(priv->hub);
        if (count > 0) priv->last_values = calloc(count, sizeof(double));
    }
    return 0;
}

static void iec104_v3_on_cleanup(edge_driver_ctx_t *ctx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    if (priv->last_values) free(priv->last_values);
}

static int iec104_v3_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    uint64_t now = uv_hrtime() / 1000000;

    // 1. [从站] 变化监测 (完全通过注入的 meta_list 驱动)
    if (ctx->role == PROTOCOL_ROLE_SLAVE && priv->apci.state == IEC104_STATE_CONNECTED) {
        uint32_t count = datahub_get_metadata_count(priv->hub);
        for (uint32_t i = 0; i < count; i++) {
            const edge_tag_meta_t *meta = datahub_get_metadata_by_index(priv->hub, i);
            if (!meta) continue;

            const iec104_map_t *map =
                ctx->lookup_by_tag ? (const iec104_map_t *)ctx->lookup_by_tag(meta->id) : NULL;
            if (!map) continue;

            pipe_msg_t val;
            if (datahub_read_shadow(priv->hub, map->tag_id, &val) == 0) {
                double diff = fabs(val.payload.val.f - priv->last_values[i]);
                if (diff > (double)map->dead_band) {
                    priv->last_values[i] = val.payload.val.f;
                    *out_tx = net_buf_alloc(ctx->pool, 0);
                    if (!*out_tx) continue;
                    iec_asdu_init(*out_tx, (iec_type_id_t)map->type_id, false, 1, IEC_COT_SPONT,
                                  priv->casdu, &IEC_ADDR_CONF_104);
                    // 修复：补齐 ts_ms (0表示让底层取当前时间) 和 flags
                    iec_asdu_add_any(*out_tx, map->type_id, map->key,
                                     val.payload.val.f / (double)map->scaler, 0, map->flags,
                                     (iec_qds_t){0}, &IEC_ADDR_CONF_104);
                    iec104_apci_wrap_i(&priv->apci, *out_tx);
                    return 0;
                }
            }
        }
    }

    // 2. [主站] 启动与心跳
    if (ctx->role == PROTOCOL_ROLE_MASTER && priv->apci.state == IEC104_STATE_DISCONNECTED) {
        *out_tx = net_buf_alloc(ctx->pool, 0);
        if (!*out_tx) return -1;
        iec104_apci_build_u(IEC104_U_STARTDT_ACT, *out_tx);
        priv->apci.state = IEC104_STATE_STARTING;
        return 0;
    }

    iec104_apci_event_t evt = iec104_apci_tick(&priv->apci, now);
    if (evt == IEC104_EVENT_SEND_S) {
        *out_tx = net_buf_alloc(ctx->pool, 0);
        if (!*out_tx) return -1;
        iec104_apci_build_s(&priv->apci, *out_tx);
    } else if (evt == IEC104_EVENT_SEND_TESTFR) {
        *out_tx = net_buf_alloc(ctx->pool, 0);
        if (!*out_tx) return -1;
        iec104_apci_build_u(IEC104_U_TESTFR_ACT, *out_tx);
    }
    return 0;
}

static int iec104_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs;
    (void)async_count;
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;

    size_t skip = 0;
    while (skip < b->len && b->data[skip] != 0x68) skip++;
    if (skip > 0) {
        *consumed = skip;
        return 1;
    }

    if (b->len < 6) return EDGE_ERR_INCOMPLETE;
    uint8_t total_len = b->data[1];
    if (b->len < (size_t)(total_len + 2)) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)(total_len + 2);

    uint8_t c1 = b->data[2];
    if (c1 == 0x07) {
        iec104_apci_build_u(IEC104_U_STARTDT_CON, resp_out);
        priv->apci.state = IEC104_STATE_CONNECTED;
        return 0;
    }

    if ((c1 & 0x01) == 0) {  // I 帧
        net_buf_simple_pull(b, 6);
        iec_type_id_t type;
        bool sq;
        uint8_t count;
        iec_cot_t cot;
        uint16_t casdu;
        if (iec_asdu_parse_header(b, &type, &sq, &count, &cot, &casdu, &IEC_ADDR_CONF_104) == 0) {
            if (type == IEC_TYPE_C_IC_NA_1) {
                // [注入利用] 响应总召唤：使用注入的 meta_list
                for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
                    const iec104_map_t *map = (const iec104_map_t *)ctx->meta_list[i];
                    pipe_msg_t vm;
                    if (datahub_read_shadow(priv->hub, map->tag_id, &vm) == 0) {
                        net_buf_t *df = net_buf_alloc(in->pool, 0);
                        iec_asdu_init(df, (iec_type_id_t)map->type_id, false, 1, IEC_COT_INTROGEN,
                                      priv->casdu, &IEC_ADDR_CONF_104);
                        // 修复：补齐参数
                        iec_asdu_add_any(df, map->type_id, map->key,
                                         vm.payload.val.f / (double)map->scaler, 0, map->flags,
                                         (iec_qds_t){0}, &IEC_ADDR_CONF_104);
                        iec104_apci_wrap_i(&priv->apci, df);
                        net_buf_simple_add_mem(&resp_out->b, df->b.data, df->b.len);
                        net_buf_unref(df);
                    }
                }
            }
        }
    }
    return 0;
}

static int iec104_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                     pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    (void)out_msgs;
    (void)msg_count;
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    size_t skip = 0;
    while (skip < b->len && b->data[skip] != 0x68) skip++;
    if (skip > 0) {
        *consumed = skip;
        return 1;
    }
    if (b->len < 6) return EDGE_ERR_INCOMPLETE;
    uint8_t total_len = b->data[1];
    if (b->len < (size_t)(total_len + 2)) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)(total_len + 2);

    uint8_t c1 = b->data[2];
    if ((c1 & 0x01) == 0) {  // I 帧
        uint16_t ns = (uint16_t)((b->data[2] | (b->data[3] << 8)) >> 1);
        uint16_t nr = (uint16_t)((b->data[4] | (b->data[5] << 8)) >> 1);
        priv->apci.nr = (uint16_t)((ns + 1) & 0x7FFF);
        iec104_apci_handle_nr(&priv->apci, nr);
    }
    return 0;
}

edge_driver_t iec104_driver = {
    .name = "IEC104",
    .ctx_size = sizeof(iec104_ctx_t),
    .on_init = iec104_v3_on_init,
    .on_cleanup = iec104_v3_on_cleanup,
    .on_tick = iec104_v3_on_tick,
    .decode_response = iec104_v3_decode_response,
    .handle_rx = iec104_v3_handle_rx,
};

int iec104_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &iec104_driver);
}
