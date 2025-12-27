#include "modbus_protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "io_executor.h"
#include "shmring.h"

extern shm_ring_t *global_ring;

static uv_loop_t *g_loop;

void modbus_protocol_init(uv_loop_t *loop) {
    g_loop = loop;
}

static double parse_meter_value(void *rx) {
    if (!rx) return 0.0;
    // Mock parsing: return a value based on first byte
    return 123.45;
}

static void on_io_done(io_job_t *job, void *u) {
    (void)u;
    if (global_ring && job->result == IO_RESULT_SUCCESS) {
        PipeMsg msg = {0};
        msg.type = PIPEMSG_SCALAR_F64;
        msg.payload.val_f = parse_meter_value(job->rx_buf);
        msg.ts = uv_now(g_loop);
        shm_ring_write(global_ring, &msg);
    }
    // io_job_free 会释放 job->user_data (即 modbus_ctx_t)
    io_job_free(job);
}

void modbus_read_request(int meter_id) {
    io_job_t *job = io_job_create(IO_BUS_RS485_1, 8, 128);
    if (!job) return;

    // Fill Modbus frame
    job->tx_buf[0] = (uint8_t)meter_id;
    job->tx_buf[1] = 0x03;

    job->cb = on_io_done;
    io_executor_submit(job);
}

io_job_t *modbus_protocol_create_read_job(int slave_id, int start_addr, int num_regs, int rx_cap,
                                          modbus_read_cb cb, void *user) {
    (void)start_addr;
    (void)num_regs;
    io_job_t *job = io_job_create(IO_BUS_RS485_1, 8, (size_t)rx_cap);
    if (!job) return NULL;

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
        if (job->result == IO_RESULT_SUCCESS) {
            uint16_t regs[1] = {200};  // Mock
            ctx->cb(0, regs, 1, ctx->user);
        } else {
            ctx->cb(-1, NULL, 0, ctx->user);
        }
    }
    // Context will be freed by io_job_free or later.
    // To be safe and compliant with libuv one-shot async,
    // we manage it in io_job_free.
}
