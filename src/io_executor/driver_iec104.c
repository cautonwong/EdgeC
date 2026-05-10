/**
 * @file driver_iec104.c
 * @brief 工业级 IEC 60870-5-104 驱动 (握手补强与总召唤版)
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
    bool needs_gi;      /**< 是否需要触发总召唤 (GI) */
    uint64_t last_gi_ts;
} iec104_ctx_t;

static int iec104_v4_on_init(edge_driver_ctx_t *ctx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(iec104_ctx_t));
    iec104_apci_init(&priv->apci);
    priv->casdu = (uint16_t)ctx->device_id;
    priv->hub = (datahub_t *)ctx->user_data;

    if (ctx->role == PROTOCOL_ROLE_SLAVE) {
        uint32_t count = datahub_get_metadata_count(priv->hub);
        if (count > 0) priv->last_values = calloc(count, sizeof(double));
    } else {
        priv->needs_gi = true; // 主站启动后需要 GI
    }
    return 0;
}

static void iec104_v4_on_cleanup(edge_driver_ctx_t *ctx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    if (priv->last_values) free(priv->last_values);
}

static int iec104_v4_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    uint64_t now = uv_hrtime() / 1000000;

    // 1. [主站] 启动握手机制 (STARTDT)
    if (ctx->role == PROTOCOL_ROLE_MASTER && priv->apci.state == IEC104_STATE_DISCONNECTED) {
        *out_tx = net_buf_alloc(ctx->pool, 0);
        iec104_apci_build_u(IEC104_U_STARTDT_ACT, *out_tx);
        priv->apci.state = IEC104_STATE_STARTING;
        return 0;
    }

    // 2. [主站] 触发总召唤 (GI) - 当链路刚建立时
    if (ctx->role == PROTOCOL_ROLE_MASTER && priv->apci.state == IEC104_STATE_CONNECTED && priv->needs_gi) {
        *out_tx = net_buf_alloc(ctx->pool, 0);
        iec_asdu_init(*out_tx, IEC_TYPE_C_IC_NA_1, false, 1, IEC_COT_ACT, priv->casdu, &IEC_ADDR_CONF_104);
        iec_asdu_add_c_ic_na_1(*out_tx, 0, 20, &IEC_ADDR_CONF_104); // QOI=20 (站召唤)
        iec104_apci_wrap_i(&priv->apci, *out_tx);
        priv->needs_gi = false;
        priv->last_gi_ts = now;
        LOG_INFO(LOG_CAT_IO, "IEC104: Handshake - Sent General Interrogation (GI).");
        return 0;
    }

    // 3. [从站] 变化自发上报 (Spontaneous)
    if (ctx->role == PROTOCOL_ROLE_SLAVE && priv->apci.state == IEC104_STATE_CONNECTED) {
        // ... 此处逻辑与 V3 类似，但可以结合 Contract 注入更优雅地处理 ...
    }

    // 4. APCI 状态机心跳 (t1, t2, t3)
    iec104_apci_event_t evt = iec104_apci_tick(&priv->apci, now);
    if (evt == IEC104_EVENT_SEND_S) {
        *out_tx = net_buf_alloc(ctx->pool, 0);
        iec104_apci_build_s(&priv->apci, *out_tx);
    } else if (evt == IEC104_EVENT_SEND_TESTFR) {
        *out_tx = net_buf_alloc(ctx->pool, 0);
        iec104_apci_build_u(IEC104_U_TESTFR_ACT, *out_tx);
    }
    
    return (*out_tx) ? 0 : -1;
}

static int iec104_v4_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    
    // 基础报文对齐
    size_t skip = 0;
    while (skip < b->len && b->data[skip] != 0x68) skip++;
    if (skip > 0) { *consumed = skip; return 1; }
    if (b->len < 6) return EDGE_ERR_INCOMPLETE;
    uint8_t total_len = b->data[1];
    if (b->len < (size_t)(total_len + 2)) return EDGE_ERR_INCOMPLETE;
    *consumed = (size_t)(total_len + 2);

    uint8_t c1 = b->data[2];
    
    // 处理 U 帧 STARTDT
    if (c1 == 0x07) { // STARTDT ACT
        iec104_apci_build_u(IEC104_U_STARTDT_CON, resp_out);
        priv->apci.state = IEC104_STATE_CONNECTED;
        LOG_INFO(LOG_CAT_IO, "IEC104: Link Activated (STARTDT CON sent).");
        return 0;
    }
    if (c1 == 0x0B) { // STARTDT CON (主站收到)
        priv->apci.state = IEC104_STATE_CONNECTED;
        LOG_INFO(LOG_CAT_IO, "IEC104: Master Link READY.");
        return 0;
    }

    // 处理 I 帧
    if ((c1 & 0x01) == 0) {
        uint16_t ns = (uint16_t)((b->data[2] | (b->data[3] << 8)) >> 1);
        uint16_t nr = (uint16_t)((b->data[4] | (b->data[5] << 8)) >> 1);
        priv->apci.nr = (uint16_t)((ns + 1) & 0x7FFF);
        iec104_apci_handle_nr(&priv->apci, nr);
        
        net_buf_simple_pull(b, 6);
        iec_type_id_t type; bool sq; uint8_t count; iec_cot_t cot; uint16_t casdu;
        if (iec_asdu_parse_header(b, &type, &sq, &count, &cot, &casdu, &IEC_ADDR_CONF_104) == 0) {
            // [从站] 响应总召唤
            if (type == IEC_TYPE_C_IC_NA_1 && cot == IEC_COT_ACT) {
                // 1. 发送镜像确认
                net_buf_t *con = net_buf_alloc(in->pool, 0);
                iec_asdu_init(con, type, false, 1, IEC_COT_ACT_CON, casdu, &IEC_ADDR_CONF_104);
                net_buf_simple_add_mem(&con->b, b->data, 1); // 镜像 QOI
                iec104_apci_wrap_i(&priv->apci, con);
                net_buf_simple_add_mem(&resp_out->b, con->b.data, con->b.len);
                net_buf_unref(con);
                
                // 2. 遍历注入的元数据批量上送
                for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
                    const iec104_map_t *map = (const iec104_map_t *)ctx->meta_list + i;
                    pipe_msg_t vm;
                    if (datahub_read_shadow(priv->hub, map->tag_id, &vm) == 0) {
                        net_buf_t *df = net_buf_alloc(in->pool, 0);
                        iec_asdu_init(df, (iec_type_id_t)map->type_id, false, 1, IEC_COT_INTROGEN,
                                      priv->casdu, &IEC_ADDR_CONF_104);
                        iec_asdu_add_any(df, map->type_id, map->key,
                                         vm.payload.val.f / (double)map->scaler, 0, map->flags,
                                         (iec_qds_t){0}, &IEC_ADDR_CONF_104);
                        iec104_apci_wrap_i(&priv->apci, df);
                        net_buf_simple_add_mem(&resp_out->b, df->b.data, df->b.len);
                        net_buf_unref(df);
                    }
                }
                
                // 3. 发送激活终止
                net_buf_t *term = net_buf_alloc(in->pool, 0);
                iec_asdu_init(term, type, false, 1, IEC_COT_ACT_TERM, casdu, &IEC_ADDR_CONF_104);
                net_buf_simple_add_u8(&term->b, 20); // QOI
                iec104_apci_wrap_i(&priv->apci, term);
                net_buf_simple_add_mem(&resp_out->b, term->b.data, term->b.len);
                net_buf_unref(term);
            }
        }
    }
    return 0;
}

edge_driver_t iec104_driver = {
    .name = "IEC104",
    .ctx_size = sizeof(iec104_ctx_t),
    .on_init = iec104_v4_on_init,
    .on_cleanup = iec104_v4_on_cleanup,
    .on_tick = iec104_v4_on_tick,
    .decode_response = iec104_v4_handle_rx, // 两端逻辑高度对称
    .handle_rx = iec104_v4_handle_rx,
};

int iec104_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &iec104_driver);
}
