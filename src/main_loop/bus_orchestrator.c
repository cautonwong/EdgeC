#include "bus_orchestrator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "edge_driver.h"
#include "io_executor.h"
#include "sched_time.h"
#include "shmring.h"

typedef struct {
    arbiter_t *arbiter;
    int bus_id;
    edge_driver_t *driver;
    edge_point_config_t *point;
    uint32_t slave_id;
    shm_ring_t *ring;
    io_job_t *io_job;
    cmd_msg_t cmd;
    bool is_write;
} job_context_t;

static void on_io_complete(io_job_t *io_job, void *user_data) {
    job_context_t *ctx = (job_context_t *)user_data;

    if (io_job->result == IO_RESULT_SUCCESS) {
        if (!ctx->is_write) {
            PipeMsg msg = {0};
            msg.tag_id = ctx->point->tag_id;
            msg.ts = (uint64_t)sched_wall_now();
            msg.type = PIPEMSG_SCALAR_F64;

            if (ctx->driver->parse_response(ctx->point, io_job->rx_buf, io_job->rx_len, &msg) ==
                0) {
                shm_ring_write(ctx->ring, &msg);
            }
        }
    }

    arbiter_on_io_done(ctx->arbiter, ctx->bus_id, &ctx->cmd);
    io_job_free(io_job);
    free(ctx);
}

static void on_bus_granted(void *user) {
    job_context_t *ctx = (job_context_t *)user;
    if (io_executor_submit(ctx->io_job) != 0) {
        arbiter_on_io_done(ctx->arbiter, ctx->bus_id, &ctx->cmd);
        io_job_free(ctx->io_job);
        free(ctx);
    }
}

static int _request_submit(arbiter_t *arb, arbiter_bus_t bus, edge_driver_t *driver,
                           edge_point_config_t *point, uint32_t slave_id, shm_ring_t *ring,
                           cmd_priority_t prio, bool is_write) {
    job_context_t *ctx = calloc(1, sizeof(job_context_t));
    ctx->arbiter = arb;
    ctx->bus_id = (int)bus;
    ctx->driver = driver;
    ctx->point = point;
    ctx->slave_id = slave_id;
    ctx->ring = ring;
    ctx->is_write = is_write;

    ctx->io_job = io_job_create((io_bus_id_t)bus, 32, 32);
    ctx->io_job->cb = on_io_complete;
    ctx->io_job->user_data = ctx;

    if (driver->build_request(slave_id, point, ctx->io_job->tx_buf, &ctx->io_job->tx_len) != 0) {
        io_job_free(ctx->io_job);
        free(ctx);
        return -1;
    }

    snprintf(ctx->cmd.req_id, 32, "req-%u", point->tag_id);
    ctx->cmd.priority = prio;
    ctx->cmd.action = is_write ? CMD_ACTION_WRITE : CMD_ACTION_READ;
    ctx->cmd.on_start = on_bus_granted;
    ctx->cmd.user_data = ctx;

    return arbiter_submit_cmd(arb, (int)bus, &ctx->cmd);
}

int edge_request_read(arbiter_t *arb, arbiter_bus_t bus, edge_driver_t *driver,
                      edge_point_config_t *point, uint32_t slave_id, shm_ring_t *ring) {
    return _request_submit(arb, bus, driver, point, slave_id, ring, CMD_PRIO_NORMAL, false);
}

int edge_request_write(arbiter_t *arb, arbiter_bus_t bus, edge_driver_t *driver,
                       edge_point_config_t *point, uint32_t slave_id, shm_ring_t *ring,
                       bool emergency) {
    cmd_priority_t prio = emergency ? CMD_PRIO_EMERGENCY : CMD_PRIO_HIGH;
    return _request_submit(arb, bus, driver, point, slave_id, ring, prio, true);
}
