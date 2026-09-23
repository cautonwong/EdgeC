/**
 * @file edge_port.c
 * @brief EdgeOS Consumer-Defined Ports & Standard Glue Adapters Implementation (AEP-751 / Issue #336)
 */

#include "edge_port.h"
#include "datahub.h"
#include "datahub_ops.h"
#include "dma_buf_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>
#include <unistd.h>
#endif

/* -------------------------------------------------------------------------- */
/* DataHub Glue Adapter Implementation                                        */
/* -------------------------------------------------------------------------- */

typedef struct {
    datahub_t *hub;
    uint64_t total_reads;
    uint64_t total_writes;
} datahub_adapter_ctx_t;

static int datahub_adapter_read_tag(void *ctx, uint32_t tag_id, double *out_val, uint64_t *out_ts_us) {
    if (!ctx || !out_val) return -1;
    datahub_adapter_ctx_t *ad = (datahub_adapter_ctx_t *)ctx;
    if (!ad->hub) return -2;

    PipeMsg msg;
    memset(&msg, 0, sizeof(msg));
    int rc = datahub_query_snapshot(ad->hub, tag_id, &msg);
    if (rc != 0) return rc;

    *out_val = msg.payload.val.f;
    if (out_ts_us) {
        *out_ts_us = msg.ts;
    }
    ad->total_reads++;
    return 0;
}

static int datahub_adapter_read_batch(void *ctx, const uint32_t *tag_ids, double *out_vals, size_t count) {
    if (!ctx || !tag_ids || !out_vals || count == 0) return -1;
    for (size_t i = 0; i < count; i++) {
        int rc = datahub_adapter_read_tag(ctx, tag_ids[i], &out_vals[i], NULL);
        if (rc != 0) return rc;
    }
    return 0;
}

static int datahub_adapter_write_tag(void *ctx, uint32_t tag_id, double val, uint64_t ts_us) {
    if (!ctx) return -1;
    datahub_adapter_ctx_t *ad = (datahub_adapter_ctx_t *)ctx;
    if (!ad->hub || !ad->hub->ops || !ad->hub->ops->write_shadow) return -2;

    PipeMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.tag_id = tag_id;
    msg.ts = ts_us;
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.payload.val.f = val;

    int rc = ad->hub->ops->write_shadow(ad->hub, &msg);
    if (rc == 0) {
        ad->total_writes++;
    }
    return rc;
}

static int datahub_adapter_write_batch(void *ctx, const uint32_t *tag_ids, const double *vals, size_t count) {
    if (!ctx || !tag_ids || !vals || count == 0) return -1;
    for (size_t i = 0; i < count; i++) {
        int rc = datahub_adapter_write_tag(ctx, tag_ids[i], vals[i], 0);
        if (rc != 0) return rc;
    }
    return 0;
}

static datahub_adapter_ctx_t s_default_datahub_ad = {0};

int edge_port_adapter_from_datahub(edge_reader_port_t *out_reader,
                                  edge_writer_port_t *out_writer,
                                  datahub_t *hub) {
    if (!hub) return -1;

    s_default_datahub_ad.hub = hub;

    if (out_reader) {
        out_reader->read_tag = datahub_adapter_read_tag;
        out_reader->read_batch = datahub_adapter_read_batch;
        out_reader->ctx = &s_default_datahub_ad;
    }

    if (out_writer) {
        out_writer->write_tag = datahub_adapter_write_tag;
        out_writer->write_batch = datahub_adapter_write_batch;
        out_writer->ctx = &s_default_datahub_ad;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* System Clock Glue Adapter Implementation                                   */
/* -------------------------------------------------------------------------- */

static uint64_t clock_adapter_now_us(void *ctx) {
    (void)ctx;
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
#endif
}

static uint64_t clock_adapter_now_ms(void *ctx) {
    return clock_adapter_now_us(ctx) / 1000ULL;
}

int edge_port_adapter_from_system_clock(edge_clock_port_t *out_clock) {
    if (!out_clock) return -1;
    out_clock->now_us = clock_adapter_now_us;
    out_clock->now_ms = clock_adapter_now_ms;
    out_clock->ctx = NULL;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* DMA-BUF Memory Pool Glue Adapter Implementation                            */
/* -------------------------------------------------------------------------- */

typedef struct {
    dma_buf_pool_t *pool;
    uint64_t total_acquires;
    uint64_t total_releases;
} dma_adapter_ctx_t;

static int dma_adapter_acquire(void *ctx, size_t size, void **out_ptr, int *out_fd) {
    if (!ctx || !out_ptr) return -1;
    (void)size;
    dma_adapter_ctx_t *ad = (dma_adapter_ctx_t *)ctx;
    if (!ad->pool) return -2;

    dma_buf_desc_t *desc = NULL;
    int rc = dma_buf_pool_acquire(ad->pool, &desc);
    if (rc != 0 || !desc) return rc ? rc : -3;

    *out_ptr = desc->vaddr;
    if (out_fd) *out_fd = desc->fd;
    ad->total_acquires++;
    return 0;
}

static int dma_adapter_release(void *ctx, void *ptr) {
    if (!ctx || !ptr) return -1;
    dma_adapter_ctx_t *ad = (dma_adapter_ctx_t *)ctx;
    if (!ad->pool) return -2;

    /* Search matching buffer descriptor by virtual address */
    dma_buf_desc_t temp_desc;
    memset(&temp_desc, 0, sizeof(temp_desc));
    temp_desc.vaddr = ptr;
    temp_desc.is_in_use = true;
    temp_desc.ref_count = 1;

    int rc = dma_buf_unref(ad->pool, &temp_desc);
    if (rc == 0) {
        ad->total_releases++;
    }
    return rc;
}

static dma_adapter_ctx_t s_default_dma_ad = {0};

int edge_port_adapter_from_dma_pool(edge_memory_port_t *out_mem, dma_buf_pool_t *pool) {
    if (!out_mem || !pool) return -1;
    s_default_dma_ad.pool = pool;
    out_mem->acquire_buffer = dma_adapter_acquire;
    out_mem->release_buffer = dma_adapter_release;
    out_mem->ctx = &s_default_dma_ad;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Console Logger Glue Adapter Implementation                                 */
/* -------------------------------------------------------------------------- */

static void console_logger_msg(void *ctx, int level, const char *tag, const char *msg) {
    (void)ctx;
    const char *lvl_str = "INFO";
    if (level <= 1) lvl_str = "DEBUG";
    else if (level == 3) lvl_str = "WARN";
    else if (level >= 4) lvl_str = "ERROR";

    printf("[%s] [%s] %s\n", lvl_str, tag ? tag : "EdgeOS", msg ? msg : "");
}

int edge_port_adapter_from_console_logger(edge_logger_port_t *out_logger) {
    if (!out_logger) return -1;
    out_logger->log_msg = console_logger_msg;
    out_logger->ctx = NULL;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Mock / Fake Ports for Pure Unit Testing (Zero OS/HAL Dependency)           */
/* -------------------------------------------------------------------------- */

#define MOCK_TAG_TABLE_SIZE 128

typedef struct {
    double tag_values[MOCK_TAG_TABLE_SIZE];
    bool tag_set[MOCK_TAG_TABLE_SIZE];
    uint64_t simulated_time_us;
    uint8_t prealloc_buffer[4096];
    bool buffer_in_use;
    pthread_mutex_t lock;
} mock_ports_state_t;

static mock_ports_state_t s_mock_state;
static bool s_mock_init = false;

static int mock_read_tag(void *ctx, uint32_t tag_id, double *out_val, uint64_t *out_ts_us) {
    mock_ports_state_t *st = (mock_ports_state_t *)ctx;
    if (!st || !out_val) return -1;

    uint32_t idx = tag_id % MOCK_TAG_TABLE_SIZE;
    *out_val = st->tag_values[idx];
    if (out_ts_us) *out_ts_us = st->simulated_time_us;
    return 0;
}

static int mock_read_batch(void *ctx, const uint32_t *tag_ids, double *out_vals, size_t count) {
    if (!ctx || !tag_ids || !out_vals) return -1;
    for (size_t i = 0; i < count; i++) {
        mock_read_tag(ctx, tag_ids[i], &out_vals[i], NULL);
    }
    return 0;
}

static int mock_write_tag(void *ctx, uint32_t tag_id, double val, uint64_t ts_us) {
    mock_ports_state_t *st = (mock_ports_state_t *)ctx;
    if (!st) return -1;

    uint32_t idx = tag_id % MOCK_TAG_TABLE_SIZE;
    st->tag_values[idx] = val;
    st->tag_set[idx] = true;
    if (ts_us > 0) st->simulated_time_us = ts_us;
    return 0;
}

static int mock_write_batch(void *ctx, const uint32_t *tag_ids, const double *vals, size_t count) {
    if (!ctx || !tag_ids || !vals) return -1;
    for (size_t i = 0; i < count; i++) {
        mock_write_tag(ctx, tag_ids[i], vals[i], 0);
    }
    return 0;
}

static uint64_t mock_now_us(void *ctx) {
    mock_ports_state_t *st = (mock_ports_state_t *)ctx;
    if (st) {
        st->simulated_time_us += 1000; /* Step 1ms forward */
        return st->simulated_time_us;
    }
    return 1000000ULL;
}

static uint64_t mock_now_ms(void *ctx) {
    return mock_now_us(ctx) / 1000ULL;
}

static int mock_acquire_buffer(void *ctx, size_t size, void **out_ptr, int *out_fd) {
    mock_ports_state_t *st = (mock_ports_state_t *)ctx;
    if (!st || !out_ptr || size > sizeof(st->prealloc_buffer)) return -1;
    if (st->buffer_in_use) return -2;

    st->buffer_in_use = true;
    *out_ptr = st->prealloc_buffer;
    if (out_fd) *out_fd = 999;
    return 0;
}

static int mock_release_buffer(void *ctx, void *ptr) {
    mock_ports_state_t *st = (mock_ports_state_t *)ctx;
    if (!st || !ptr) return -1;
    st->buffer_in_use = false;
    return 0;
}

static void mock_log_msg(void *ctx, int level, const char *tag, const char *msg) {
    (void)ctx; (void)level; (void)tag; (void)msg;
    /* Silent mock logger */
}

int edge_port_create_mock_ports(edge_consumer_ports_t *out_ports, double initial_tag_val) {
    if (!out_ports) return -1;

    if (!s_mock_init) {
        memset(&s_mock_state, 0, sizeof(s_mock_state));
        pthread_mutex_init(&s_mock_state.lock, NULL);
        s_mock_init = true;
    }

    for (int i = 0; i < MOCK_TAG_TABLE_SIZE; i++) {
        s_mock_state.tag_values[i] = initial_tag_val;
        s_mock_state.tag_set[i] = true;
    }
    s_mock_state.simulated_time_us = 10000000ULL;
    s_mock_state.buffer_in_use = false;

    out_ports->reader.read_tag = mock_read_tag;
    out_ports->reader.read_batch = mock_read_batch;
    out_ports->reader.ctx = &s_mock_state;

    out_ports->writer.write_tag = mock_write_tag;
    out_ports->writer.write_batch = mock_write_batch;
    out_ports->writer.ctx = &s_mock_state;

    out_ports->clock.now_us = mock_now_us;
    out_ports->clock.now_ms = mock_now_ms;
    out_ports->clock.ctx = &s_mock_state;

    out_ports->memory.acquire_buffer = mock_acquire_buffer;
    out_ports->memory.release_buffer = mock_release_buffer;
    out_ports->memory.ctx = &s_mock_state;

    out_ports->logger.log_msg = mock_log_msg;
    out_ports->logger.ctx = &s_mock_state;

    return 0;
}
