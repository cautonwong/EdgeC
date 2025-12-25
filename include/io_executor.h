#pragma once
#include <stdint.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IO_RS485,
    IO_HPLC,
    IO_CAN
} io_bus_t;

typedef struct {
    io_bus_t bus;
    int fd;                 /* 串口 / 设备 fd */
    uint8_t *tx;
    size_t tx_len;
    uint8_t *rx;
    size_t rx_cap;
    ssize_t rx_len;
    int result;
    void *user; /* New field for user data */
} io_job_t;

typedef void (*io_done_cb)(io_job_t *job, void *user);

void io_submit(
    uv_loop_t *loop,
    io_job_t *job,
    io_done_cb cb,
    void *user
);

#ifdef __cplusplus
}
#endif
