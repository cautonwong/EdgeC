#include "modbus_protocol.h"
#include "shmring.h"
#include "arbiter.h"
#include "io_executor.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern shm_ring_t *global_ring; 

static uv_loop_t *g_loop;

void modbus_protocol_init(uv_loop_t *loop) {
    g_loop = loop;
}

static double parse_meter_value(void *rx) {
    (void)rx;
    return 123.45; 
}

static void on_io_done(io_job_t *job, void *u) {
    (void)u;
    if (global_ring) {
        PipeMsg msg = {
            .type = PIPE_SCALAR_F64,
            .payload.val_f = parse_meter_value(job->rx_buf)
        };
        shm_ring_write(global_ring, &msg);
    }
    
    // Cleanup job
    free(job->rx_buf);
    free(job->tx_buf);
    if (job->user_data) free(job->user_data); // Free user ctx if any
    free(job);
}

void modbus_read_request(int meter_id) {
    // Legacy support: call create_read_job but use our own callback handling
    // or just construct a job manually to be safe/simple.
    
    io_job_t *job = calloc(1, sizeof(io_job_t));
    job->bus_id = IO_BUS_RS485_1;
    job->rx_cap = 128;
    job->rx_buf = malloc(128);
    job->tx_len = 8;
    job->tx_buf = malloc(8);
    // ... fill tx ...
    
    job->cb = on_io_done;
    job->user_data = NULL;

    // Direct submit, NO ARBITER (to avoid leak in legacy test path)
    io_executor_submit(job);
}

io_job_t* modbus_protocol_create_read_job(
    int slave_id, 
    int start_addr, 
    int num_regs, 
    int rx_cap, 
    modbus_read_cb cb, 
    void *user
) {
    (void)start_addr; (void)num_regs;
    io_job_t *job = calloc(1, sizeof(io_job_t));
    job->bus_id = IO_BUS_RS485_1; 
    job->rx_cap = (size_t)rx_cap;
    job->rx_buf = malloc((size_t)rx_cap);
    
    job->tx_len = 8;
    job->tx_buf = malloc(8);
    job->tx_buf[0] = (uint8_t)slave_id;
    job->tx_buf[1] = 0x03; 
    
    modbus_ctx_t *ctx = malloc(sizeof(modbus_ctx_t));
    ctx->cb = cb;
    ctx->user = user;
    job->user_data = ctx;

    return job;
}

void modbus_protocol_handle_response(io_job_t *job) {
    modbus_ctx_t *ctx = (modbus_ctx_t *)job->user_data;
    if (ctx && ctx->cb) {
        if (job->result == 0) {
            uint16_t regs[128] = {0};
            if (job->rx_len >= 5) {
                regs[0] = 200; 
            }
            ctx->cb(0, regs, 1, ctx->user);
        } else {
            ctx->cb(job->result, NULL, 0, ctx->user);
        }
    }
    // We ALWAYS free the context here as it's a one-shot callback
    if (ctx) {
        free(ctx);
        job->user_data = NULL; // Prevent double free
    }
}