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
        for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
            const iec104_map_t *map = (const iec104_map_t *)ctx->meta_list + i;
            if (map->tag_id == 0) continue;

            pipe_msg_t vm;
            if (datahub_read_shadow(priv->hub, map->tag_id, &vm) == 0) {
                // 简单的死区判断
                if (fabs(vm.payload.val.f - priv->last_values[i]) > (double)map->dead_band) {
                    if (*out_tx == NULL) *out_tx = net_buf_alloc(ctx->pool, 0);
                    
                    net_buf_t *df = net_buf_alloc(ctx->pool, 0);
                    iec_asdu_init(df, (iec_type_id_t)map->type_id, false, 1, IEC_COT_SPONT,
                                  priv->casdu, &IEC_ADDR_CONF_104);
                    iec_asdu_add_any(df, map->type_id, map->key,
                                     vm.payload.val.f / (double)map->scaler, 0, map->flags,
                                     (iec_qds_t){0}, &IEC_ADDR_CONF_104);
                    iec104_apci_wrap_i(&priv->apci, df);
                    net_buf_simple_add_mem(&(*out_tx)->b, df->b.data, df->b.len);
                    net_buf_unref(df);
                    
                    priv->last_values[i] = vm.payload.val.f;
                }
            }
        }
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
                    if (map->tag_id == 0) continue;

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
            } else if (type == IEC_TYPE_C_SC_NA_1 || type == IEC_TYPE_C_DC_NA_1) {
                // [从站] 接收遥控指令
                uint32_t ioa = 0;
                uint8_t scs = 0;
                if (iec_asdu_parse_c_sc_na_1(b, &ioa, &scs, &IEC_ADDR_CONF_104) == 0) {
                    // 1. 发送镜像确认 (ACT CON)
                    net_buf_t *con = net_buf_alloc(in->pool, 0);
                    iec_asdu_init(con, type, false, 1, IEC_COT_ACT_CON, casdu, &IEC_ADDR_CONF_104);
                    net_buf_simple_add_u8(&con->b, ioa & 0xFF);
                    net_buf_simple_add_u8(&con->b, (ioa >> 8) & 0xFF);
                    net_buf_simple_add_u8(&con->b, (ioa >> 16) & 0xFF);
                    net_buf_simple_add_u8(&con->b, scs);
                    iec104_apci_wrap_i(&priv->apci, con);
                    net_buf_simple_add_mem(&resp_out->b, con->b.data, con->b.len);
                    net_buf_unref(con);

                    // 2. 通过 DataHub 下发控制指令
                    if (ctx->lookup_by_key) {
                        const iec104_map_t *map = (const iec104_map_t *)ctx->lookup_by_key(ioa);
                        if (map) {
                            pipe_msg_t cmd_msg;
                            pipe_msg_init_scalar(&cmd_msg, map->tag_id, (double)(scs & 0x01), uv_hrtime());
                            // Since PIPEMSG_TYPE_CMD doesn't exist, just use SCALAR for now
                            // The bus orchestrator usually handles commands, but if DataHub needs to route it:
                            datahub_publish(priv->hub, &cmd_msg);
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * @brief [主站] 构建 IEC104 读请求 (遥测/遥信/总召唤)
 * 使用注入的 meta_list 查找对应测点的 type_id 和 IOA
 */
static int iec104_v4_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids,
                                     int tag_count, net_buf_t *out) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    if (tag_count == 0 || !ctx->meta_list) return -1;

    for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
        const iec104_map_t *map = (const iec104_map_t *)ctx->meta_list + i;
        if (map->tag_id != tag_ids[0]) continue;

        iec_asdu_init(out, (iec_type_id_t)map->type_id, false, 1, IEC_COT_REQ,
                      priv->casdu, &IEC_ADDR_CONF_104);

        if ((iec_type_id_t)map->type_id == IEC_TYPE_C_IC_NA_1) {
            iec_asdu_add_c_ic_na_1(out, map->key, 20, &IEC_ADDR_CONF_104);
        } else {
            iec_asdu_add_any(out, (uint8_t)map->type_id, map->key, 0, 0,
                             map->flags, (iec_qds_t){0}, &IEC_ADDR_CONF_104);
        }
        iec104_apci_wrap_i(&priv->apci, out);
        return 0;
    }
    return -1;
}

/**
 * @brief [主站] 构建 IEC104 写请求 (遥控/遥调)
 */
static int iec104_v4_encode_write(edge_driver_ctx_t *ctx, uint32_t tag_id,
                                   double value, net_buf_t *out) {
    iec104_ctx_t *priv = (iec104_ctx_t *)ctx->priv;
    if (!ctx->meta_list) return -1;

    for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
        const iec104_map_t *map = (const iec104_map_t *)ctx->meta_list + i;
        if (map->tag_id != tag_id) continue;

        iec_type_id_t tid = (iec_type_id_t)map->type_id;
        iec_asdu_init(out, tid, false, 1, IEC_COT_ACT, priv->casdu, &IEC_ADDR_CONF_104);

        if (tid == IEC_TYPE_C_SC_NA_1) {
            iec_asdu_add_c_sc_na_1(out, map->key, (value > 0), false, &IEC_ADDR_CONF_104);
        } else if (tid == IEC_TYPE_C_DC_NA_1) {
            iec_asdu_add_c_dc_na_1(out, map->key, (uint8_t)(value > 0 ? 1 : 0), false,
                                    &IEC_ADDR_CONF_104);
        } else if (tid == IEC_TYPE_C_SE_NC_1) {
            iec_asdu_add_c_se_nc_1(out, map->key, (float)value, 0, false, &IEC_ADDR_CONF_104);
        } else {
            iec_asdu_add_any(out, (uint8_t)map->type_id, map->key, value, 0,
                             map->flags, (iec_qds_t){0}, &IEC_ADDR_CONF_104);
        }
        iec104_apci_wrap_i(&priv->apci, out);
        return 0;
    }
    return -1;
}

edge_driver_t iec104_driver = {
    .name = "IEC104",
    .ctx_size = sizeof(iec104_ctx_t),
    .on_init = iec104_v4_on_init,
    .on_cleanup = iec104_v4_on_cleanup,
    .on_tick = iec104_v4_on_tick,
    .encode_request = iec104_v4_encode_request,
    .encode_write = iec104_v4_encode_write,
    .decode_response = iec104_v4_handle_rx,
    .handle_rx = iec104_v4_handle_rx,
};

int iec104_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &iec104_driver);
}
