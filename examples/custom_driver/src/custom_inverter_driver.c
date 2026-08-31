/**
 * @file custom_inverter_driver.c
 * @brief Secondary Developer Example: Custom Inverter Driver Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "custom_inverter_driver.h"

typedef struct {
    char device_uri[64];
    uint32_t slave_addr;
    custom_inverter_telemetry_t current_data;
    bool is_online;
} custom_inverter_priv_t;

static int custom_inverter_on_init(edgeos_driver_ctx_t *ctx) {
    if (!ctx) return -1;
    custom_inverter_priv_t *priv = (custom_inverter_priv_t *)calloc(1, sizeof(custom_inverter_priv_t));
    if (!priv) return -1;

    priv->slave_addr = 1;
    priv->is_online = true;
    priv->current_data.voltage_ac = 220.0f;
    priv->current_data.current_ac = 10.0f;
    priv->current_data.active_power_w = 2200.0f;
    priv->current_data.grid_freq_hz = 50.0f;

    ctx->priv = priv;
    return 0;
}

static void custom_inverter_on_cleanup(edgeos_driver_ctx_t *ctx) {
    if (!ctx) return;
    if (ctx->priv) {
        free(ctx->priv);
        ctx->priv = NULL;
    }
}

static bool custom_inverter_is_ready(edgeos_driver_ctx_t *ctx) {
    if (!ctx || !ctx->priv) return false;
    custom_inverter_priv_t *priv = (custom_inverter_priv_t *)ctx->priv;
    return priv->is_online;
}

static int custom_inverter_on_tick(edgeos_driver_ctx_t *ctx, net_buf_t **out_tx) {
    (void)ctx;
    if (out_tx) *out_tx = NULL;
    return 0;
}

const edgeos_driver_t custom_inverter_driver = {
    .name = "custom_inverter_driver",
    .ctx_size = sizeof(custom_inverter_priv_t),
    .on_init = (int (*)(edge_driver_ctx_t *))custom_inverter_on_init,
    .on_cleanup = (void (*)(edge_driver_ctx_t *))custom_inverter_on_cleanup,
    .is_ready = (bool (*)(edge_driver_ctx_t *))custom_inverter_is_ready,
    .on_tick = (int (*)(edge_driver_ctx_t *, net_buf_t **))custom_inverter_on_tick,
};

int custom_inverter_simulate_poll(edgeos_driver_ctx_t *ctx, edgeos_datahub_t *hub) {
    if (!ctx || !ctx->priv || !hub) return -1;
    custom_inverter_priv_t *priv = (custom_inverter_priv_t *)ctx->priv;
    if (!priv->is_online) return -1;

    // Simulate physical inverter reading and DataHub shadow bus writeback
    edgeos_pipe_msg_t msg_u;
    pipe_msg_init_scalar(&msg_u, 1001, (double)priv->current_data.voltage_ac, 1000);
    datahub_write_shadow(hub, &msg_u);

    edgeos_pipe_msg_t msg_i;
    pipe_msg_init_scalar(&msg_i, 1002, (double)priv->current_data.current_ac, 1000);
    datahub_write_shadow(hub, &msg_i);

    edgeos_pipe_msg_t msg_p;
    pipe_msg_init_scalar(&msg_p, 1003, (double)priv->current_data.active_power_w, 1000);
    datahub_write_shadow(hub, &msg_p);

    edgeos_pipe_msg_t msg_f;
    pipe_msg_init_scalar(&msg_f, 1004, (double)priv->current_data.grid_freq_hz, 1000);
    datahub_write_shadow(hub, &msg_f);

    return 0;
}
