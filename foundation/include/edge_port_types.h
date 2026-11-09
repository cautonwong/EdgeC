/**
 * @file edge_port_types.h
 * @brief EdgeOS consumer-owned port value types shared by foundation and components.
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 *
 * These types are pure data contracts. Adapter constructors that bind concrete
 * adapter declarations stay in foundation's edge_port.h; components that only consume the
 * port types can include this foundation header without depending on edge_core.
 */
#ifndef EDGE_PORT_TYPES_H
#define EDGE_PORT_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Consumer Port: Real-time Telemetry & State Reader
 */
typedef struct edge_reader_port {
    int (*read_tag)(void *ctx, uint32_t tag_id, double *out_val, uint64_t *out_ts_us);
    int (*read_batch)(void *ctx, const uint32_t *tag_ids, double *out_vals, size_t count);
    void *ctx;
} edge_reader_port_t;

/**
 * @brief Consumer Port: Actuation, Setpoint & Telecontrol Writer
 */
typedef struct edge_writer_port {
    int (*write_tag)(void *ctx, uint32_t tag_id, double val, uint64_t ts_us);
    int (*write_batch)(void *ctx, const uint32_t *tag_ids, const double *vals, size_t count);
    void *ctx;
} edge_writer_port_t;

/**
 * @brief Consumer Port: Deterministic Wall-Clock & Monotonic Time Provider
 */
typedef struct edge_clock_port {
    uint64_t (*now_us)(void *ctx);
    uint64_t (*now_ms)(void *ctx);
    void *ctx;
} edge_clock_port_t;

/**
 * @brief Consumer Port: Zero-Copy Preallocated Buffer / DMA-BUF Memory Allocator
 */
typedef struct edge_memory_port {
    int (*acquire_buffer)(void *ctx, size_t size, void **out_ptr, int *out_fd);
    int (*release_buffer)(void *ctx, void *ptr);
    void *ctx;
} edge_memory_port_t;

/**
 * @brief Consumer Port: Structured Telemetry & Diagnostics Logger
 */
typedef struct edge_logger_port {
    void (*log_msg)(void *ctx, int level, const char *tag, const char *msg);
    void *ctx;
} edge_logger_port_t;

/**
 * @brief Unified Injected Consumer Ports Bundle
 */
typedef struct edge_consumer_ports {
    edge_reader_port_t reader;
    edge_writer_port_t writer;
    edge_clock_port_t clock;
    edge_memory_port_t memory;
    edge_logger_port_t logger;
} edge_consumer_ports_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_PORT_TYPES_H */
