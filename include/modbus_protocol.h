#pragma once
#include <stdint.h>
#include <uv.h> // For any libuv types if needed, although not directly in this API yet.
#include "io_executor.h" // For io_job_t

#ifdef __cplusplus
extern "C" {
#endif

// Modbus function codes
#define MODBUS_FC_READ_HOLDING_REGISTERS 0x03

// Modbus exception codes
#define MODBUS_EXC_ILLEGAL_FUNCTION     0x01
#define MODBUS_EXC_ILLEGAL_DATA_ADDRESS 0x02
#define MODBUS_EXC_ILLEGAL_DATA_VALUE   0x03
#define MODBUS_EXC_SLAVE_DEVICE_FAILURE 0x04

// Callback for parsed Modbus read response
// - status: 0 for success, non-zero for Modbus exception code or protocol error
// - registers: Pointer to an array of uint16_t holding the parsed register values.
//              This memory is owned by the callback caller and should be freed by the caller.
//              NULL if status is not 0.
// - num_registers: Number of registers in the array.
// - user: User-defined context.
typedef void (*modbus_read_cb)(int status, const uint16_t *registers, uint16_t num_registers, void *user);

/**
 * @brief Creates a Modbus RTU "Read Holding Registers" job.
 *        This function allocates memory for io_job_t->tx and io_job_t->rx.
 *        The caller is responsible for freeing io_job_t->tx and io_job_t->rx
 *        after the job is completed and processed.
 *
 * @param slave_id Modbus slave address (1-247).
 * @param start_addr Starting register address (0-65535).
 * @param num_regs Number of registers to read (1-125).
 * @param rx_cap_bytes Capacity for the response buffer (io_job_t->rx).
 * @param on_read_done Callback to be invoked when the Modbus response is parsed.
 * @param user User-defined context to be passed to on_read_done.
 * @return A pointer to an allocated io_job_t if successful, NULL on error.
 *         The caller is responsible for freeing this io_job_t.
 */
io_job_t *modbus_protocol_create_read_job(
    uint8_t slave_id,
    uint16_t start_addr,
    uint16_t num_regs,
    size_t rx_cap_bytes,
    modbus_read_cb on_read_done,
    void *user
);

/**
 * @brief Handles a completed io_job_t, parses the Modbus RTU response,
 *        and invokes the associated modbus_read_cb.
 *
 * @param job The completed io_job_t containing the Modbus response in job->rx.
 *            The job->user field should contain the modbus_read_cb and user context.
 */
void modbus_protocol_handle_response(io_job_t *job);

#ifdef __cplusplus
}
#endif
