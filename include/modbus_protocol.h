#pragma once
#include <stdint.h>
#include <uv.h>

#include "io_executor.h"

typedef void (*modbus_read_cb)(int status, const uint16_t *registers, uint16_t num_registers,
                               void *user);

typedef struct {
    modbus_read_cb cb;
    void *user;
} modbus_ctx_t;

void modbus_protocol_init(uv_loop_t *loop);
void modbus_read_request(int meter_id);

/* New API for Integration */
io_job_t *modbus_protocol_create_read_job(int slave_id, int start_addr, int num_regs, int rx_cap,
                                          modbus_read_cb cb, void *user);

void modbus_protocol_handle_response(io_job_t *job);
