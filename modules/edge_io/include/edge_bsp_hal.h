/**
 * @file edge_bsp_hal.h
 * @brief Production Linux BSP Hardware Driver Abstraction Layer (SPI, GPIO, UART)
 */

#ifndef EDGE_BSP_HAL_H
#define EDGE_BSP_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char dev_path[64]; // e.g. "/dev/spidev0.0"
    int fd;
    uint32_t speed_hz;
    uint8_t mode;
    uint8_t bits_per_word;
    bool is_open;
} edge_spi_device_t;

typedef struct {
    uint32_t pin_num;
    char direction[8]; // "in" or "out"
    int fd;
    bool is_open;
} edge_gpio_pin_t;

typedef struct {
    char tty_path[64]; // e.g. "/dev/ttyS1"
    int fd;
    uint32_t baud_rate;
    bool is_open;
} edge_uart_device_t;

/**
 * @brief 初始化并打开 Linux 字符设备 SPI 接口 (支持 ESAM 硬件加密芯片)
 */
int edge_bsp_spi_open(edge_spi_device_t* dev, const char* path, uint32_t speed_hz);
int edge_bsp_spi_transfer(edge_spi_device_t* dev, const uint8_t* tx, uint8_t* rx, uint16_t len);
void edge_bsp_spi_close(edge_spi_device_t* dev);

/**
 * @brief 初始化并操作 Linux GPIO 节点 (支持 16-DI 遥信输入与 8-DO 继电器输出)
 */
int edge_bsp_gpio_export(edge_gpio_pin_t* pin, uint32_t pin_num, const char* dir);
int edge_bsp_gpio_write(edge_gpio_pin_t* pin, bool val);
int edge_bsp_gpio_read(edge_gpio_pin_t* pin, bool* out_val);
void edge_bsp_gpio_unexport(edge_gpio_pin_t* pin);

/**
 * @brief 初始化并打开 Linux TTY 串口 (支持 HPLC 载波模组与 RS485 总线)
 */
int edge_bsp_uart_open(edge_uart_device_t* dev, const char* path, uint32_t baud);
int edge_bsp_uart_send(edge_uart_device_t* dev, const uint8_t* buf, size_t len);
int edge_bsp_uart_recv(edge_uart_device_t* dev, uint8_t* buf, size_t max_len, uint32_t timeout_ms);
void edge_bsp_uart_close(edge_uart_device_t* dev);

#ifdef __cplusplus
}
#endif

#endif // EDGE_BSP_HAL_H
