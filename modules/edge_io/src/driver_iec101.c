/**
 * @file driver_iec101.c
 * @brief IEC 60870-5-101 工业级驱动实现 (主从双模 + 接口注入版)
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "contract_data_types.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "edge_log.h"
#include "iec101_link.h"
#include "iec_asdu.h"
#include "net_buf.h"

typedef struct {
    iec101_link_mgr_t link;
    iec_addr_config_t addr_conf;
    uint16_t casdu;
    bool request_level1;
} iec101_ctx_t;

static int iec101_v3_on_init(edge_driver_ctx_t *ctx) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    memset(priv, 0, sizeof(iec101_ctx_t));

    // 动态配置应从设备私有参数获取，此处预留
    priv->addr_conf.cot_len = 1;
    priv->addr_conf.casdu_len = 1;
    priv->addr_conf.ioa_len = 2;

    priv->link.link_addr = (uint16_t)ctx->device_id;
    priv->link.addr_len = 1;
    priv->casdu = (uint16_t)ctx->device_id;

    return 0;
}

static int iec101_v3_on_tick(edge_driver_ctx_t *ctx, net_buf_t **out_tx) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;

    if (ctx->role == PROTOCOL_ROLE_MASTER && out_tx) {
        /* 主站: 周期性请求链路状态 */
        iec101_ctrl_t req_ctrl = {.prm = 1, .func = 9}; /* 请求链路状态 */
        *out_tx = net_buf_alloc(ctx->pool, 0);
        iec101_link_build_fixed(&priv->link, req_ctrl, *out_tx);
        return 0;
    }

    if (ctx->role == PROTOCOL_ROLE_SLAVE) {
        /* 从站自发上送 (通过 on_tick 输出给引擎发送) */
        for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
            const iec101_map_t *map = (const iec101_map_t *)ctx->meta_list + i;
            if (map->tag_id == 0) continue;
            (void)map;
        }
    }
    return -1;
}

static int _process_asdu(edge_driver_ctx_t *ctx, struct net_buf_simple *b, pipe_msg_t *out_msgs,
                         int *msg_count) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    iec_type_id_t type;
    bool sq;
    uint8_t count;
    iec_cot_t cot;
    uint16_t casdu;
    if (iec_asdu_parse_header(b, &type, &sq, &count, &cot, &casdu, &priv->addr_conf) != 0)
        return -1;

    uint32_t current_ioa = 0;
    int parsed = 0;

    for (int i = 0; i < count && parsed < 16; i++) {
        bool is_first = (i == 0);
        // 使用注入的 lookup_by_key 进行反查
        if (type == IEC_TYPE_M_SP_NA_1) {
            bool val;
            iec_qds_t qds;
            if (iec_asdu_parse_m_sp_na_1(b, &current_ioa, &val, &qds, sq, is_first,
                                         &priv->addr_conf) == 0) {
                const iec101_map_t *m = ctx->lookup_by_key
                                            ? (const iec101_map_t *)ctx->lookup_by_key(current_ioa)
                                            : NULL;
                if (m)
                    pipe_msg_init_scalar(&out_msgs[parsed++], m->tag_id, val ? 1.0 : 0.0,
                                         (uint64_t)time(NULL) * 1000);
            }
        } else if (type == IEC_TYPE_M_ME_NC_1) {
            float val;
            iec_qds_t qds;
            if (iec_asdu_parse_m_me_nc_1(b, &current_ioa, &val, &qds, sq, is_first,
                                         &priv->addr_conf) == 0) {
                const iec101_map_t *m = ctx->lookup_by_key
                                            ? (const iec101_map_t *)ctx->lookup_by_key(current_ioa)
                                            : NULL;
                if (m)
                    pipe_msg_init_scalar(&out_msgs[parsed++], m->tag_id, (double)val * m->scaler,
                                         (uint64_t)time(NULL) * 1000);
            }
        }
    }
    *msg_count = parsed;
    return 0;
}

static int iec101_v3_decode_response(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                                     pipe_msg_t *out_msgs, int *msg_count, size_t *consumed) {
    (void)resp_out;
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;
    *msg_count = 0;

    // 1. [流式同步] 寻找有效的 101 包头 (0x10, 0x68, 0xE5)
    size_t skip = 0;
    while (skip < b->len) {
        uint8_t h = b->data[skip];
        if (h == 0x10 || h == 0x68 || h == 0xE5) break;
        skip++;
    }
    if (skip > 0) {
        *consumed = skip;
        return 1;
    }

    if (b->len == 0) return EDGE_ERR_INCOMPLETE;
    uint8_t head = b->data[0];

    if (head == 0xE5) {
        *consumed = 1;
        priv->link.last_fcb = !priv->link.last_fcb;  // 成功确认，翻转
        return 0;
    } else if (head == 0x10) {
        if (b->len < (size_t)(3 + priv->link.addr_len)) return EDGE_ERR_INCOMPLETE;
        *consumed = (size_t)(3 + priv->link.addr_len);
        priv->link.last_fcb = !priv->link.last_fcb;  // 成功确认，翻转
        return 0;
    } else if (head == 0x68) {
        if (b->len < 4) return EDGE_ERR_INCOMPLETE;
        uint8_t len = b->data[1];
        if (b->len < (size_t)(len + 6)) return EDGE_ERR_INCOMPLETE;
        *consumed = (size_t)(len + 6);
        priv->link.last_fcb = !priv->link.last_fcb;  // 成功确认，翻转
        net_buf_simple_pull(b, (size_t)(4 + 1 + priv->link.addr_len));
        _process_asdu(ctx, b, out_msgs, msg_count);
    } else {
        *consumed = 1;
        return -1;
    }
    return 0;
}

static int iec101_v3_handle_rx(edge_driver_ctx_t *ctx, net_buf_t *in, net_buf_t *resp_out,
                               pipe_msg_t *async_msgs, int *async_count, size_t *consumed) {
    (void)async_msgs;
    (void)async_count;
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    struct net_buf_simple *b = &in->b;
    *consumed = 0;

    if (b->len < 1) return EDGE_ERR_INCOMPLETE;
    uint8_t head = b->data[0];

    if (head == 0x10) {
        if (b->len < (size_t)(3 + priv->link.addr_len)) return EDGE_ERR_INCOMPLETE;
        *consumed = (size_t)(3 + priv->link.addr_len);
        uint8_t func = b->data[1] & 0x0F;
        if (func == 9) {
            iec101_ctrl_t resp_ctrl = {.prm = 0, .func = 11};
            iec101_link_build_fixed(&priv->link, resp_ctrl, resp_out);
        }
        return 0;
    }

    if (head == 0x68) {
        if (b->len < 4) return EDGE_ERR_INCOMPLETE;
        uint8_t len = b->data[1];
        if (b->len < (size_t)(len + 6)) return EDGE_ERR_INCOMPLETE;
        *consumed = (size_t)(len + 6);
        net_buf_simple_pull(b, (size_t)(4 + 1 + priv->link.addr_len));
        iec_type_id_t type;
        bool sq;
        uint8_t count;
        iec_cot_t cot;
        uint16_t casdu;
        if (iec_asdu_parse_header(b, &type, &sq, &count, &cot, &casdu, &priv->addr_conf) == 0) {
            if (type == IEC_TYPE_C_IC_NA_1) {
                // [注入利用] 响应总召唤
                // 使用注入的 ctx->meta_list (指向 iec101_map_t 数组)
                const iec101_map_t *maps = (const iec101_map_t *)ctx->meta_list;
                for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
                    const iec101_map_t *map = maps + i;
                    if (map->tag_id == 0) continue;

                    pipe_msg_t vm;
                    if (edge_driver_read_shadow(ctx, map->tag_id, &vm) == 0) {
                        net_buf_t *df = net_buf_alloc(in->pool, 0);
                        iec_asdu_init(df, (iec_type_id_t)map->type_id, false, 1, IEC_COT_INTROGEN,
                                      priv->casdu, &priv->addr_conf);
                        // 修复：补齐参数 (ts_ms=0, flags=map->flags)
                        iec_asdu_add_any(df, map->type_id, map->key,
                                         vm.payload.val.f / (double)map->scaler, 0, map->flags,
                                         (iec_qds_t){0}, &priv->addr_conf);
                        // 注意：iec101_link_build_var 需要实现，此处简化，假设直接发
                        // 实际应调用 iec101_link_build_variable(&priv->link, df, resp_out);
                        net_buf_simple_add_mem(&resp_out->b, df->b.data, df->b.len);
                        net_buf_unref(df);
                    }
                }
            }
        }
    }
    return 0;
}

/**
 * @brief [主站] 构建 IEC101 读/写请求 ASDU 负载
 * 注: IEC101 链路帧封装由引擎层完成
 */
static int iec101_v3_encode_request(edge_driver_ctx_t *ctx, const uint32_t *tag_ids,
                                     int tag_count, net_buf_t *out) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    if (tag_count == 0 || !ctx->meta_list) return -1;

    for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
        const iec101_map_t *map = (const iec101_map_t *)ctx->meta_list + i;
        if (map->tag_id != tag_ids[0]) continue;

        iec_asdu_init(out, (iec_type_id_t)map->type_id, false, 1, IEC_COT_REQ,
                      priv->casdu, &priv->addr_conf);
        iec_asdu_add_any(out, (uint8_t)map->type_id, map->key, 0, 0,
                         map->flags, (iec_qds_t){0}, &priv->addr_conf);
        return 0;
    }
    return -1;
}

/**
 * @brief [主站] 构建 IEC101 写请求 ASDU 负载
 */
static int iec101_v3_encode_write(edge_driver_ctx_t *ctx, uint32_t tag_id,
                                   double value, net_buf_t *out) {
    iec101_ctx_t *priv = (iec101_ctx_t *)ctx->priv;
    if (!ctx->meta_list) return -1;

    for (uint32_t i = 0; i < ctx->meta_list_len; i++) {
        const iec101_map_t *map = (const iec101_map_t *)ctx->meta_list + i;
        if (map->tag_id != tag_id) continue;

        iec_type_id_t tid = (iec_type_id_t)map->type_id;
        iec_asdu_init(out, tid, false, 1, IEC_COT_ACT, priv->casdu, &priv->addr_conf);

        if (tid == IEC_TYPE_C_SC_NA_1)
            iec_asdu_add_c_sc_na_1(out, map->key, (value > 0), false, &priv->addr_conf);
        else if (tid == IEC_TYPE_C_SE_NC_1)
            iec_asdu_add_c_se_nc_1(out, map->key, (float)value, 0, false, &priv->addr_conf);
        else
            iec_asdu_add_any(out, (uint8_t)map->type_id, map->key, value, 0,
                             map->flags, (iec_qds_t){0}, &priv->addr_conf);
        return 0;
    }
    return -1;
}

edge_driver_t iec101_driver = {.name = "IEC101",
                               .ctx_size = sizeof(iec101_ctx_t),
                               .on_init = iec101_v3_on_init,
                               .on_tick = iec101_v3_on_tick,
                               .encode_request = iec101_v3_encode_request,
                               .encode_write = iec101_v3_encode_write,
                               .decode_response = iec101_v3_decode_response,
                               .handle_rx = iec101_v3_handle_rx};

int iec101_driver_register(driver_manager_t *mgr) {
    return driver_manager_register(mgr, &iec101_driver);
}