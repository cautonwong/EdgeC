#include "modbus_protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h> // For debugging, remove later if not needed

// Internal helper to store original callback and user data
typedef struct {
    modbus_read_cb on_read_done;
    void *user;
} modbus_req_ctx_t;

// CRC16-Modbus Calculation (polynomial 0xA001)
static uint16_t modbus_crc16(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];          // XOR byte into LSB of crc
        for (int i = 8; i != 0; i--) {      // Loop over each bit
            if ((crc & 0x0001) != 0) {      // If the LSB is set
                crc >>= 1;                  // Shift right and XOR 0xA001
                crc ^= 0xA001;
            } else {                        // Else LSB is not set
                crc >>= 1;                  // Just shift right
            }
        }
    }
    return crc;
}

io_job_t *modbus_protocol_create_read_job(
    uint8_t slave_id,
    uint16_t start_addr,
    uint16_t num_regs,
    size_t rx_cap_bytes,
    modbus_read_cb on_read_done,
    void *user
) {
    if (slave_id == 0 || slave_id > 247 || num_regs == 0 || num_regs > 125) {
        fprintf(stderr, "Modbus: Invalid parameters for read job creation.\n");
        return NULL;
    }

    io_job_t *job = calloc(1, sizeof(io_job_t));
    if (!job) return NULL;

    // Modbus RTU request frame for Read Holding Registers (FC 0x03)
    // Structure: Slave ID (1 byte) + Function Code (1 byte) +
    //            Starting Address (2 bytes) + Quantity of Registers (2 bytes) +
    //            CRC (2 bytes)
    job->tx_len = 8;
    job->tx = (uint8_t *)malloc(job->tx_len);
    if (!job->tx) {
        free(job);
        return NULL;
    }

    job->tx[0] = slave_id;
    job->tx[1] = MODBUS_FC_READ_HOLDING_REGISTERS;
    job->tx[2] = (uint8_t)(start_addr >> 8);    // High byte of starting address
    job->tx[3] = (uint8_t)(start_addr & 0xFF);  // Low byte of starting address
    job->tx[4] = (uint8_t)(num_regs >> 8);      // High byte of quantity of registers
    job->tx[5] = (uint8_t)(num_regs & 0xFF);    // Low byte of quantity of registers

    uint16_t crc = modbus_crc16(job->tx, job->tx_len - 2);
    job->tx[6] = (uint8_t)(crc & 0xFF);         // Low byte of CRC
    job->tx[7] = (uint8_t)(crc >> 8);           // High byte of CRC

    // Prepare rx buffer
    job->rx_cap = rx_cap_bytes;
    if (job->rx_cap > 0) {
        job->rx = (uint8_t *)malloc(job->rx_cap);
        if (!job->rx) {
            free(job->tx);
            free(job);
            return NULL;
        }
    }

    // Store original callback and user data for later
    modbus_req_ctx_t *req_ctx = malloc(sizeof(modbus_req_ctx_t));
    if (!req_ctx) {
        free(job->tx);
        if (job->rx) free(job->rx);
        free(job);
        return NULL;
    }
    req_ctx->on_read_done = on_read_done;
    req_ctx->user = user;
    job->user = req_ctx; // Use io_job_t's user field to store our context

    return job;
}

void modbus_protocol_handle_response(io_job_t *job) {
    if (!job || !job->rx || job->rx_len < 5) { // Min response length: slave ID + FC + Byte Count + Data (1 byte) + CRC (2 bytes)
        fprintf(stderr, "Modbus: Invalid job or response data.\n");
        if (job && job->user) {
            modbus_req_ctx_t *req_ctx = (modbus_req_ctx_t *)job->user;
            if (req_ctx->on_read_done) {
                req_ctx->on_read_done(-1, NULL, 0, req_ctx->user); // -1 for generic protocol error
            }
            free(req_ctx);
            job->user = NULL;
        }
        return;
    }

    modbus_req_ctx_t *req_ctx = (modbus_req_ctx_t *)job->user;
    if (!req_ctx || !req_ctx->on_read_done) {
        fprintf(stderr, "Modbus: No callback context found.\n");
        return;
    }

    uint8_t slave_id = job->rx[0];
    uint8_t fc = job->rx[1];
    uint16_t received_crc = ((uint16_t)job->rx[job->rx_len - 1] << 8) | job->rx[job->rx_len - 2];
    uint16_t calculated_crc = modbus_crc16(job->rx, job->rx_len - 2);

    if (received_crc != calculated_crc) {
        fprintf(stderr, "Modbus: CRC mismatch. Expected 0x%04X, got 0x%04X.\n", calculated_crc, received_crc);
        req_ctx->on_read_done(-2, NULL, 0, req_ctx->user); // -2 for CRC error
        free(req_ctx);
        job->user = NULL;
        return;
    }

    // Check for Modbus exception response (Function Code + 0x80)
    if (fc & 0x80) {
        int exception_code = job->rx[2];
        fprintf(stderr, "Modbus: Exception response received. FC: 0x%02X, Code: 0x%02X.\n", fc, exception_code);
        req_ctx->on_read_done(exception_code, NULL, 0, req_ctx->user);
        free(req_ctx);
        job->user = NULL;
        return;
    }

    // Normal Read Holding Registers (FC 0x03) response
    if (fc == MODBUS_FC_READ_HOLDING_REGISTERS) {
        uint8_t byte_count = job->rx[2];
        uint16_t expected_rx_len = 3 + byte_count + 2; // Slave ID + FC + Byte Count + Data + CRC
        if (job->rx_len != expected_rx_len) {
            fprintf(stderr, "Modbus: Response length mismatch. Expected %u, got %zd.\n", expected_rx_len, job->rx_len);
            req_ctx->on_read_done(-3, NULL, 0, req_ctx->user); // -3 for length error
            free(req_ctx);
            job->user = NULL;
            return;
        }

        uint16_t num_registers = byte_count / 2;
        uint16_t *registers = (uint16_t *)malloc(sizeof(uint16_t) * num_registers);
        if (!registers) {
            fprintf(stderr, "Modbus: Failed to allocate memory for registers.\n");
            req_ctx->on_read_done(-4, NULL, 0, req_ctx->user); // -4 for memory error
            free(req_ctx);
            job->user = NULL;
            return;
        }

        for (uint16_t i = 0; i < num_registers; ++i) {
            registers[i] = ((uint16_t)job->rx[3 + i * 2] << 8) | job->rx[3 + i * 2 + 1];
        }

        req_ctx->on_read_done(0, registers, num_registers, req_ctx->user);
        free(registers); // Free the allocated registers after callback, as per API contract
    } else {
        fprintf(stderr, "Modbus: Unexpected function code 0x%02X in response.\n", fc);
        req_ctx->on_read_done(-5, NULL, 0, req_ctx->user); // -5 for unexpected FC
    }

    free(req_ctx); // Free the request context after handling
    job->user = NULL; // Clear job's user pointer
}
