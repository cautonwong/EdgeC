#include <stdlib.h>
#include <string.h>

#include "bus_engine.h"
#include "driver_manager.h"
#include "edge_log.h"

#define MAX_SLAVE_PROTOCOLS 4

typedef struct {
    edge_driver_ctx_t *ctx;
    char name[16];
} slave_proto_slot_t;

typedef struct {
    bus_engine_t base;
    uv_loop_t *loop;
    arbiter_t *arb;
    datahub_t *hub;
    net_buf_pool_t *pool;
    driver_manager_t *drv_mgr;
    driver_contract_injection_t inject_data;  // Persist injection

    slave_proto_slot_t protocols[MAX_SLAVE_PROTOCOLS];
    int proto_count;
    int last_hit_idx;

    net_buf_t *rx_acc;
    io_handle_t *io;
    int bus_id;
} slave_priv_t;

static void _slave_io_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    slave_priv_t *priv = (slave_priv_t *)user_data;
    net_buf_t *rx = (net_buf_t *)payload;

    if (evt == IO_EVENT_ERROR) {
        printf("\033[31m[IEMS-SLAVE] IO ERROR on Bus %d: %s\033[0m\n", priv->bus_id, strerror(errno));
        return;
    }

    if (evt == IO_EVENT_RX && rx) {
        // [Nexus Debug] 明确打印接收日志
        printf("\033[33m[IEMS-SLAVE] Bus %d RX: %u bytes\033[0m\n", priv->bus_id, (uint32_t)rx->b.len);

        // [Nexus] 内存空间预检与归位：如果当前缓冲区为空，重置指针到头部

        if (priv->rx_acc->b.len == 0) {
            net_buf_simple_reset(&priv->rx_acc->b);
        }

        // [Nexus] 防止脏数据无限积压

        if (priv->rx_acc->b.len > 1024) {
            LOG_WARN(LOG_CAT_BUS, "Slave Bus %d: Accumulator overflow, clearing buffer",
                     priv->bus_id);

            net_buf_simple_reset(&priv->rx_acc->b);
        }

        net_buf_append_bytes(priv->rx_acc, rx->b.data, rx->b.len);

        net_buf_unref(rx);

                // 尝试解析

                while (priv->rx_acc->b.len > 0) {

                    bool processed = false;

                    bool any_incomplete = false;

                    size_t initial_len = priv->rx_acc->b.len;

        

                    for (int i = 0; i < priv->proto_count; i++) {

                        int idx = (i + priv->last_hit_idx) % priv->proto_count;

                        slave_proto_slot_t *slot = &priv->protocols[idx];

        

                        net_buf_t *resp = net_buf_alloc(priv->pool, 0);

                        pipe_msg_t async_msgs[16];

                        int async_count = 0;

                        size_t consumed = 0;

        

                        int rc = slot->ctx->driver->handle_rx(slot->ctx, priv->rx_acc, resp, async_msgs,

                                                              &async_count, &consumed);

        

                        if (rc == 0) {

                            if (resp->b.len > 0) {

                                printf("\033[32m[IEMS-SLAVE] Sending Response: %u bytes\033[0m\n",

                                       (uint32_t)resp->b.len);

                                priv->io->ops->write_async(priv->io, resp);

                            }

                            net_buf_unref(resp);

        

                            for (int j = 0; j < async_count; j++)

                                datahub_publish(priv->hub, &async_msgs[j]);

        

                            if (consumed > 0) net_buf_pull(priv->rx_acc, consumed);

                            priv->last_hit_idx = idx;

                            processed = true;

                            break;

                        } else {

                            net_buf_unref(resp);

                            if (rc == EDGE_ERR_INCOMPLETE) {

                                any_incomplete = true;

                                continue;

                            } else if (consumed > 0) {

                                net_buf_pull(priv->rx_acc, consumed);

                                processed = true;

                                break;

                            }

                        }

                    }

        

                    if (!processed) {

                        // [Nexus Fix] 更加稳健的滑动窗口：

                        // 如果没有任何协议匹配，且没有任何协议报告“报文未收全”，说明头部确实是垃圾数据

                        if (!any_incomplete && priv->rx_acc->b.len > 0) {

                            net_buf_pull(priv->rx_acc, 1);

                            continue;

                        }

                        break; // 等待更多数据

                    }

                    if (priv->rx_acc->b.len >= initial_len) break;

                }
    }
}

int engine_slave_add_protocol(bus_engine_t *self, const char *protocol, uint32_t device_id,
                              const driver_contract_injection_t *inject) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    if (priv->proto_count >= MAX_SLAVE_PROTOCOLS) return -1;

    edge_driver_ctx_t *ctx = driver_instance_create(priv->drv_mgr, protocol, device_id);
    if (!ctx) {
        printf("\033[31m[SLAVE-INIT] Failed to create driver instance for %s\033[0m\n", protocol);
        return -1;
    }

    printf("\033[32m[SLAVE-INIT] Protocol %s added to Bus %d (SlaveID: %u)\033[0m\n", protocol,
           priv->bus_id, device_id);

    ctx->role = PROTOCOL_ROLE_SLAVE;
    ctx->pool = priv->pool;      // [注入] 共享总线池
    ctx->user_data = priv->hub;  // [注入] 打通影子表访问路径

    // [注入应用] 填充驱动上下文
    if (inject) {
        ctx->lookup_by_key = inject->lookup_by_key;
        ctx->lookup_by_tag = inject->lookup_by_tag;
        ctx->meta_list = inject->meta_list;
        ctx->meta_list_len = inject->meta_list_len;
    } else {
        // Fallback or inheritance?
        ctx->lookup_by_key = priv->inject_data.lookup_by_key;
        ctx->lookup_by_tag = priv->inject_data.lookup_by_tag;
        ctx->meta_list = priv->inject_data.meta_list;
        ctx->meta_list_len = priv->inject_data.meta_list_len;
    }

    priv->protocols[priv->proto_count].ctx = ctx;
    strncpy(priv->protocols[priv->proto_count].name, protocol, 15);
    priv->proto_count++;

    return 0;
}

static void _slave_attach(bus_engine_t *self, io_handle_t *io) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    priv->io = io;
    io->cb = _slave_io_cb;
    io->user_data = priv;
    io->ops->enable_read(io, true);
}

static void _slave_detach(bus_engine_t *self) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    if (priv->io) priv->io->ops->enable_read(priv->io, false);
    for (int i = 0; i < priv->proto_count; i++) {
        driver_instance_destroy(priv->protocols[i].ctx, priv->protocols[i].name);
    }
    if (priv->rx_acc) net_buf_unref(priv->rx_acc);
    free(priv);
    free(self);
}

static void _slave_run_command(bus_engine_t *self, cmd_msg_t *cmd) {
    slave_priv_t *priv = (slave_priv_t *)self->user_data;
    // Slave 模式目前不通过仲裁器主动发起指令，如果收到，直接报错/忽略
    arbiter_on_io_done(priv->arb, priv->bus_id, cmd, CMD_STATUS_IO_ERROR);
}

bus_engine_t *engine_create_slave(driver_manager_t *mgr, int bus_id, const char *protocol,
                                  uint32_t device_id, uv_loop_t *loop, arbiter_t *arb,
                                  datahub_t *hub, net_buf_pool_t *pool,
                                  const driver_contract_injection_t *inject) {
    bus_engine_t *eng = calloc(1, sizeof(bus_engine_t));
    slave_priv_t *priv = calloc(1, sizeof(slave_priv_t));
    priv->loop = loop;
    priv->arb = arb;
    priv->hub = hub;
    priv->pool = pool;
    priv->drv_mgr = mgr;
    priv->bus_id = bus_id;
    if (inject) priv->inject_data = *inject;

    priv->rx_acc = net_buf_alloc(pool, 0);
    net_buf_simple_reset(&priv->rx_acc->b);

    eng->user_data = priv;
    eng->attach = _slave_attach;
    eng->detach = _slave_detach;
    eng->run_command = _slave_run_command;

    if (protocol && protocol[0] != '\0') {
        engine_slave_add_protocol(eng, protocol, device_id, inject);
    }

    return eng;
}
