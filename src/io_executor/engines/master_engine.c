#include <stdlib.h>
#include <string.h>

#include "bus_engine.h"
#include "driver_manager.h"
#include "edge_log.h"

typedef struct {
    bus_engine_t base;

    // Dependencies
    uv_loop_t *loop;
    arbiter_t *arb;
    datahub_t *hub;
    net_buf_pool_t *pool;
    driver_manager_t *drv_mgr;

    // State (Dynamic Context)
    edge_driver_ctx_t *drv_ctx;
    char current_proto[16];
    driver_contract_injection_t inject_data;

    io_handle_t *io;
    uv_timer_t timer;
    cmd_msg_t *current_cmd;
    net_buf_t *rx_acc;
    int bus_id;
} master_priv_t;

static void _master_on_timeout(uv_timer_t *handle) {
    master_priv_t *priv = (master_priv_t *)handle->data;
    if (priv->current_cmd) {
        LOG_WARN(LOG_CAT_BUS, "Bus %d Timeout on cmd %s [%s]", priv->bus_id,
                 priv->current_cmd->req_id, priv->current_proto);
        priv->io->ops->enable_read(priv->io, false);
        if (priv->arb)
            arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_TIMEOUT);
        priv->current_cmd = NULL;
    }
}

static void _master_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    master_priv_t *priv = (master_priv_t *)user_data;
    net_buf_t *rx = (net_buf_t *)payload;

    if (evt == IO_EVENT_ERROR) {
        if (priv->current_cmd) {
            arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_IO_ERROR);
            priv->current_cmd = NULL;
        }
        priv->io->ops->enable_read(priv->io, false);
        return;
    }

    if (evt == IO_EVENT_RX && rx) {
        fprintf(stderr, "[TRACE][MasterEngine] Received %u raw bytes from IO\n", rx->b.len);
        net_buf_append_bytes(priv->rx_acc, rx->b.data, rx->b.len);
        net_buf_unref(rx);

        bool frame_found = false;
        while (priv->rx_acc->b.len > 0) {
            if (!priv->drv_ctx) break;

            pipe_msg_t msgs[16];
            int count = 0;
            size_t consumed = 0;
            net_buf_t *resp = net_buf_alloc(priv->pool, 0);

            int rc = priv->drv_ctx->driver->decode_response(priv->drv_ctx, priv->rx_acc, resp, msgs,
                                                            &count, &consumed);

            if (resp->b.len > 0) {
                priv->io->ops->write_async(priv->io, resp);
            }
            net_buf_unref(resp);

            if (rc == 0) {
                for (int i = 0; i < count; i++) {
                    fprintf(stderr, "[DEBUG][MasterEngine] Publishing Tag %u (Val %f) to DataHub\n", 
                            msgs[i].tag_id, msgs[i].payload.val.f);
                    datahub_publish(priv->hub, &msgs[i]);
                }
                frame_found = true;
                if (consumed > 0) net_buf_pull(priv->rx_acc, consumed);
                net_buf_unref(resp); // <--- 这里一定要释放
                break;
            } else if (rc == EDGE_ERR_INCOMPLETE) {
                if (consumed > 0) net_buf_pull(priv->rx_acc, consumed);
                net_buf_unref(resp); // <--- 这里也要释放
                break;
            } else {
                if (consumed > 0)
                    net_buf_pull(priv->rx_acc, consumed);
                else
                    net_buf_simple_reset(&priv->rx_acc->b);
                net_buf_unref(resp); // <--- 这里更要释放
                break;
            }
        }

        if (frame_found) {
            uv_timer_stop(&priv->timer);
            priv->io->ops->enable_read(priv->io, false);
            if (priv->current_cmd) {
                arbiter_on_io_done(priv->arb, priv->bus_id, priv->current_cmd, CMD_STATUS_SUCCESS);
                priv->current_cmd = NULL;
            }
        }
    }
}

static void _master_attach(bus_engine_t *self, io_handle_t *io) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    priv->io = io;
    io->cb = _master_io_cb;
    io->user_data = priv;
    io->ops->enable_read(io, false);
}

static void _master_run_command(bus_engine_t *self, cmd_msg_t *cmd) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    if (!priv->io) {
        fprintf(stderr, "[ENGINE] Bus %d error: IO not attached\n", priv->bus_id);
        if (priv->arb)
            arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_IO_ERROR);
        return;
    }

    fprintf(stderr, "[ENGINE] Bus %d executing command %s (Tag %u)\n", priv->bus_id, cmd->req_id,
            cmd->tag_id);

    // --- 核心重整：多协议动态切换 (Context Switching) ---
    if (cmd->protocol[0] != '\0' && strcmp(cmd->protocol, priv->current_proto) != 0) {
        LOG_INFO(LOG_CAT_BUS, "Bus %d: Protocol Switch [%s -> %s]", priv->bus_id,
                 priv->current_proto, cmd->protocol);

        if (priv->drv_ctx) {
            driver_instance_destroy(priv->drv_ctx, priv->current_proto);
            priv->drv_ctx = NULL;
        }

        priv->drv_ctx = driver_instance_create(priv->drv_mgr, cmd->protocol, cmd->tag_id);
        if (!priv->drv_ctx) {
            LOG_ERROR(LOG_CAT_BUS, "Bus %d: Failed to load driver '%s'", priv->bus_id,
                      cmd->protocol);
            arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
            return;
        }
        priv->drv_ctx->role = PROTOCOL_ROLE_MASTER;
        priv->drv_ctx->pool = priv->pool;  // [注入] 共享总线池
        // [注入应用] 动态切换时应用契约
        priv->drv_ctx->lookup_by_key = priv->inject_data.lookup_by_key;
        priv->drv_ctx->lookup_by_tag = priv->inject_data.lookup_by_tag;
        priv->drv_ctx->meta_list = priv->inject_data.meta_list;
        priv->drv_ctx->meta_list_len = priv->inject_data.meta_list_len;

        strncpy(priv->current_proto, cmd->protocol, 15);
        net_buf_simple_reset(&priv->rx_acc->b);
    }

    priv->current_cmd = cmd;

    if (cmd->phy_params && cmd->phy_params->baud > 0) {
        priv->io->ops->ioctl(priv->io, IO_CTL_SET_BAUD, &cmd->phy_params->baud);
    }

    uv_timer_start(&priv->timer, _master_on_timeout, cmd->timeout_ms ? cmd->timeout_ms : 1000, 0);

    net_buf_t *tx = net_buf_alloc(priv->pool, 0);
    if (tx) {
        uint32_t tags[1] = {cmd->tag_id};
        if (priv->drv_ctx &&
            priv->drv_ctx->driver->encode_request(priv->drv_ctx, tags, 1, tx) == 0) {
            fprintf(stderr, "[DEBUG][Node 3] MasterEngine sending %u bytes: ", tx->b.len);
            for (uint16_t i = 0; i < tx->b.len; i++) fprintf(stderr, "%02X ", tx->b.data[i]);
            fprintf(stderr, "\n");
            priv->io->ops->enable_read(priv->io, true);
            priv->io->ops->write_async(priv->io, tx);
        } else {
            fprintf(stderr, "[DEBUG][Node 3] MasterEngine FAILED to encode request (drv_ctx=%p)\n",
                    (void *)priv->drv_ctx);
            net_buf_unref(tx);
            uv_timer_stop(&priv->timer);
            arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_PROTO_ERROR);
        }
    }
}

static void _master_detach(bus_engine_t *self) {
    master_priv_t *priv = (master_priv_t *)self->user_data;
    uv_timer_stop(&priv->timer);
    if (!uv_is_closing((uv_handle_t *)&priv->timer)) uv_close((uv_handle_t *)&priv->timer, NULL);
    if (priv->drv_ctx) driver_instance_destroy(priv->drv_ctx, priv->current_proto);
    if (priv->rx_acc) net_buf_unref(priv->rx_acc);
    free(priv);
    free(self);
}

bus_engine_t *engine_create_master(driver_manager_t *mgr, const char *protocol, uint32_t device_id,
                                   uv_loop_t *loop, arbiter_t *arb, datahub_t *hub,
                                   net_buf_pool_t *pool,
                                   const driver_contract_injection_t *inject) {
    bus_engine_t *eng = calloc(1, sizeof(bus_engine_t));
    master_priv_t *priv = calloc(1, sizeof(master_priv_t));
    priv->loop = loop;
    priv->arb = arb;
    priv->hub = hub;
    priv->pool = pool;
    priv->drv_mgr = mgr;
    priv->bus_id = (int)device_id;
    if (inject) priv->inject_data = *inject;  // 缓存注入信息

    priv->rx_acc = net_buf_alloc(pool, 0);
    net_buf_simple_reset(&priv->rx_acc->b);

    uv_timer_init(loop, &priv->timer);
    priv->timer.data = priv;

    if (protocol && protocol[0] != '\0') {
        priv->drv_ctx = driver_instance_create(mgr, protocol, device_id);
        if (priv->drv_ctx) {
            priv->drv_ctx->role = PROTOCOL_ROLE_MASTER;
            priv->drv_ctx->pool = priv->pool;  // [注入] 共享总线池
            // [注入应用] 初始应用
            if (inject) {
                priv->drv_ctx->lookup_by_key = inject->lookup_by_key;
                priv->drv_ctx->lookup_by_tag = inject->lookup_by_tag;
                priv->drv_ctx->meta_list = inject->meta_list;
                priv->drv_ctx->meta_list_len = inject->meta_list_len;
            }
            strncpy(priv->current_proto, protocol, 15);
        }
    }

    eng->user_data = priv;
    eng->attach = _master_attach;
    eng->run_command = _master_run_command;
    eng->detach = _master_detach;
    return eng;
}