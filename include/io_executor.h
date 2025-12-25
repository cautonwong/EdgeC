#ifndef IO_EXECUTOR_H
#define IO_EXECUTOR_H

#include <uv.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct io_job_s io_job_t;
typedef struct io_executor_s io_executor_t;

// IO Bus Types
typedef enum {
    IO_BUS_RS485_1 = 0,
    IO_BUS_RS485_2,
    IO_BUS_CAN_1,
    IO_BUS_MAX
} io_bus_id_t;

// Job Result Codes
typedef enum {
    IO_RESULT_SUCCESS = 0,
    IO_RESULT_TIMEOUT,
    IO_RESULT_ERROR,
    IO_RESULT_CANCELLED
} io_result_t;

// IO Operation Types
typedef enum {
    IO_OP_READ_WRITE,   // Standard write then read
    IO_OP_WRITE_ONLY,   // Broadcast
    IO_OP_READ_ONLY     // Listen mode (uncommon for master)
} io_op_t;

// Callback type for job completion
// This callback MUST be executed on the MAIN LOOP
typedef void (*io_complete_cb)(io_job_t *job, void *user_data);

// Job Structure
struct io_job_s {
    // Identity
    uint32_t id;            // Unique Job ID
    io_bus_id_t bus_id;
    io_op_t op;
    
    // Data Buffers
    uint8_t *tx_buf;        // Data to send
    size_t tx_len;
    uint8_t *rx_buf;        // Buffer to receive data
    size_t rx_cap;          // Capacity of rx_buf
    size_t rx_len;          // Actual received length
    
    // Timing & Control
    uint64_t timeout_ms;    // IO timeout
    
    // Result
    io_result_t result;
    int system_errno;       // Errno if failed
    
    // Callback info
    io_complete_cb cb;
    void *user_data;
    
    // Internal use (List pointers, etc.)
    struct io_job_s *next;
};

// Executor Management
// Initialize the executor system (creates threads per bus)
int io_executor_init(uv_loop_t *main_loop);

// Shutdown all IO threads
void io_executor_shutdown(void);

// Submit a job to the appropriate IO thread
// Returns 0 on success, < 0 on failure
int io_executor_submit(io_job_t *job);

// Cancel a pending job
void io_executor_cancel(io_job_t *job);

// Helper: Allocate a job
io_job_t* io_job_create(io_bus_id_t bus, size_t tx_size, size_t rx_size);

// Helper: Free a job
void io_job_free(io_job_t *job);

#ifdef __cplusplus
}
#endif

#endif // IO_EXECUTOR_H