#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "contract_data_types.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "iec104_apci.h"
#include "iec_asdu.h"
#include "net_buf.h"

// 外部契约符号声明 (由链接器在各产品构建时匹配)
extern const iec104_map_t* const g_iec104_gi_list[];
extern const uint32_t g_iec104_gi_len;

typedef struct {
    iec104_apci_mgr_t apci;
    uint16_t casdu;
    datahub_t *hub;
    uint32_t gi_cursor;
    double *last_values;
} iec104_ctx_t;

static int iec104_v3_on_init(edge_driver_ctx_t *ctx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(iec104_ctx_t));
    iec104_apci_init(&priv->apci);
    priv->casdu = (uint16_t)ctx->device_id;
    priv->hub = (datahub_t *)ctx->user_data;
    priv->gi_cursor = 0xFFFFFFFF; 

    if (ctx->role == PROTOCOL_ROLE_SLAVE) {
        priv->last_values = calloc(g_iec104_gi_len > 0 ? g_iec104_gi_len : 1, sizeof(double));
    }
    return 0;
}

static void iec104_v3_on_cleanup(edge_driver_ctx_t *ctx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    if (priv->last_values) free(priv->last_values);
}

static int iec104_v3_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    
    // GI 状态机
    if (ctx->role == PROTOCOL_ROLE_SLAVE && priv->gi_cursor < g_iec104_gi_len) {
        const iec104_map_t *map = g_iec104_gi_list[priv->gi_cursor++];
        pipe_msg_t val;
        if (datahub_read_shadow(priv->hub, map->tag_id, &val) == 0) {
            *out_tx = net_buf_alloc(NULL, 0);
            iec_asdu_init(*out_tx, (iec_type_id_t)map->type_id, false, 1, IEC_COT_INTROGEN, priv->casdu, &IEC_ADDR_CONF_104);
            iec_asdu_add_any(*out_tx, map->type_id, map->key, val.payload.val.f / (double)map->scaler, 
                             val.ts, map->flags, (iec_qds_t){0}, &IEC_ADDR_CONF_104);
            iec104_apci_wrap_i(&priv->apci, *out_tx);
            return 0;
        }
    }

    // 突发上报聚合
    if (ctx->role == PROTOCOL_ROLE_SLAVE && priv->apci.state == IEC104_STATE_CONNECTED && priv->gi_cursor >= g_iec104_gi_len) {
        net_buf_t *batch = NULL;
        int count = 0;
        for (uint32_t i = 0; i < g_iec104_gi_len; i++) {
            const iec104_map_t *map = g_iec104_gi_list[i];
            pipe_msg_t val;
            if (datahub_read_shadow(priv->hub, map->tag_id, &val) == 0) {
                if (fabs(val.payload.val.f - priv->last_values[i]) > (double)map->dead_band) {
                    priv->last_values[i] = val.payload.val.f;
                    if (!batch) {
                        batch = net_buf_alloc(NULL, 0);
                        iec_asdu_init(batch, (iec_type_id_t)map->type_id, false, 0, IEC_COT_SPONT, priv->casdu, &IEC_ADDR_CONF_104);
                    }
                    if (iec_asdu_add_any(batch, map->type_id, map->key, val.payload.val.f / (double)map->scaler, val.ts, map->flags, (iec_qds_t){0}, &IEC_ADDR_CONF_104) == 0) {
                        count++;
                    }
                    if (count >= 10) break;
                }
            }
        }
        if (batch) {
            batch->b.data[1] = (uint8_t)((batch->b.data[1] & 0x80) | (count & 0x7F));
            iec104_apci_wrap_i(&priv->apci, batch);
            *out_tx = batch;
            return 0;
        }
    }
    return 0;
}

static int iec104_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs; (void)async_count;
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;
    if (b->len < 6) return EDGE_ERR_INCOMPLETE;
    if (b->data[0] != 0x68) { *consumed = 1; return 1; }
    uint8_t len = b->data[1];
    if (b->len < (size_t)(len + 2)) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)(len + 2);

    if (b->data[2] == 0x07) { // STARTDT ACT
        iec104_apci_build_u(IEC104_U_STARTDT_CON, resp_out);
        priv->apci.state = IEC104_STATE_CONNECTED;
    } else if ((b->data[2] & 0x01) == 0) { // I 帧
        net_buf_simple_pull(b, 6);
        iec_type_id_t type; bool sq; uint8_t count; iec_cot_t cot; uint16_t casdu;
        if (iec_asdu_parse_header(b, &type, &sq, &count, &cot, &casdu, &IEC_ADDR_CONF_104) == 0) {
            if (type == IEC_TYPE_C_IC_NA_1) {
                net_buf_t *ack = net_buf_alloc(in->pool, 0);
                iec_asdu_init(ack, IEC_TYPE_C_IC_NA_1, false, 1, IEC_COT_ACT_CON, priv->casdu, &IEC_ADDR_CONF_104);
                iec_asdu_add_c_ic_na_1(ack, 0, 20, &IEC_ADDR_CONF_104);
                iec104_apci_wrap_i(&priv->apci, ack);
                net_buf_simple_add_mem(&resp_out->b, ack->b.data, ack->b.len);
                net_buf_unref(ack);
                priv->gi_cursor = 0;
            }
        }
    }
    return 0;
}

edge_driver_t iec104_driver = {
    .name = "IEC104",
    .ctx_size = sizeof(iec104_ctx_t),
    .on_init = iec104_v3_on_init,
    .on_cleanup = iec104_v3_on_cleanup,
    .on_tick = iec104_v3_on_tick,
    .handle_rx = iec104_v3_handle_rx,
};

int iec104_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &iec104_driver);
}
