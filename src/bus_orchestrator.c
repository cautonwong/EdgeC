#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "arbiter.h"
#include "io_executor.h"
#include "edge_driver.h"
#include "shmring.h"

typedef struct {
    arbiter_t *arbiter;
    arbiter_bus_t bus_id;
    edge_driver_t *driver;
    edge_point_config_t *point;
    uint32_t slave_id;
    shm_ring_t *ring;
    io_job_t *io_job;
    bool is_write;
} job_context_t;

static void on_io_complete(io_job_t *io_job, void *user_data) {
    job_context_t *ctx = (job_context_t *)user_data;
    
    if (io_job->result == IO_RESULT_SUCCESS) {
        if (!ctx->is_write) {
            PipeMsg msg = {0};
            msg.tag_id = ctx->point->tag_id;
            msg.ts = uv_now(uv_default_loop());
            
            if (ctx->driver->parse_response(ctx->point, io_job->rx_buf, io_job->rx_len, &msg) == 0) {
                shm_ring_write(ctx->ring, &msg);
            }
        } else {
            printf("[Orchestrator] Write Job Success for tag %u\n", ctx->point->tag_id);
        }
    } else {
        fprintf(stderr, "[Orchestrator] IO Job Failed (Result: %d)\n", io_job->result);
    }

    arbiter_release(ctx->arbiter, ctx->bus_id, ctx);
    io_job_free(io_job);
    free(ctx);
}

static void on_bus_granted(arbiter_t *arb, arbiter_bus_t bus, void *user) {
    job_context_t *ctx = (job_context_t *)user;
    if (io_executor_submit(ctx->io_job) != 0) {
        arbiter_release(arb, bus, ctx);
        io_job_free(ctx->io_job);
        free(ctx);
    }
}

static int _request_submit(arbiter_t *arb, arbiter_bus_t bus, edge_driver_t *driver, 
                          edge_point_config_t *point, uint32_t slave_id,
                          shm_ring_t *ring, arbiter_prio_t prio, bool is_write) {
    job_context_t *ctx = calloc(1, sizeof(job_context_t));
    ctx->arbiter = arb;
    ctx->bus_id = bus;
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

    return arbiter_request(arb, bus, prio, ARB_SHARED, 1000, on_bus_granted, ctx);
}

int edge_request_read(arbiter_t *arb, arbiter_bus_t bus, edge_driver_t *driver, 
                     edge_point_config_t *point, uint32_t slave_id, shm_ring_t *ring) {
    return _request_submit(arb, bus, driver, point, slave_id, ring, ARB_PRIO_NORMAL, false);
}

int edge_request_write(arbiter_t *arb, arbiter_bus_t bus, edge_driver_t *driver, 
                      edge_point_config_t *point, uint32_t slave_id, shm_ring_t *ring, bool emergency) {
    arbiter_prio_t prio = emergency ? ARB_PRIO_EMERGENCY : ARB_PRIO_HIGH;
    return _request_submit(arb, bus, driver, point, slave_id, ring, prio, true);
}