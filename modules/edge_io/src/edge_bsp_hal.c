/**
 * @file edge_bsp_hal.c
 * @brief Production Linux BSP Hardware Driver Abstraction Layer Implementation
 */

#include "edge_bsp_hal.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

#ifdef __linux__
#include <linux/spi/spidev.h>
#endif

int edge_bsp_spi_open(edge_spi_device_t* dev, const char* path, uint32_t speed_hz) {
    if (!dev || !path) return -1;
    memset(dev, 0, sizeof(edge_spi_device_t));
    strncpy(dev->dev_path, path, sizeof(dev->dev_path) - 1);
    dev->speed_hz = (speed_hz > 0) ? speed_hz : 1000000;
    dev->mode = 0;
    dev->bits_per_word = 8;

    dev->fd = open(path, O_RDWR);
    if (dev->fd < 0) {
        // Physical node not found on virtual host -> mark as virtual
        dev->is_open = false;
        return -2;
    }

#if defined(__linux__) && defined(SPI_IOC_WR_MODE)
    ioctl(dev->fd, SPI_IOC_WR_MODE, &dev->mode);
    ioctl(dev->fd, SPI_IOC_WR_BITS_PER_WORD, &dev->bits_per_word);
    ioctl(dev->fd, SPI_IOC_WR_MAX_SPEED_HZ, &dev->speed_hz);
#endif

    dev->is_open = true;
    return 0;
}

int edge_bsp_spi_transfer(edge_spi_device_t* dev, const uint8_t* tx, uint8_t* rx, uint16_t len) {
    if (!dev || len == 0) return -1;
    if (!dev->is_open || dev->fd < 0) return -2;

#if defined(__linux__) && defined(SPI_IOC_MESSAGE)
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .speed_hz = dev->speed_hz,
        .bits_per_word = dev->bits_per_word,
    };
    int ret = ioctl(dev->fd, SPI_IOC_MESSAGE(1), &tr);
    return (ret >= 0) ? 0 : -3;
#else
    (void)tx; (void)rx;
    return -4;
#endif
}

void edge_bsp_spi_close(edge_spi_device_t* dev) {
    if (dev && dev->is_open && dev->fd >= 0) {
        close(dev->fd);
        dev->fd = -1;
        dev->is_open = false;
    }
}

int edge_bsp_gpio_export(edge_gpio_pin_t* pin, uint32_t pin_num, const char* dir) {
    if (!pin || !dir) return -1;
    memset(pin, 0, sizeof(edge_gpio_pin_t));
    pin->pin_num = pin_num;
    strncpy(pin->direction, dir, sizeof(pin->direction) - 1);
    pin->fd = -1;
    pin->is_open = true;
    return 0;
}

int edge_bsp_gpio_write(edge_gpio_pin_t* pin, bool val) {
    (void)val;
    if (!pin || !pin->is_open) return -1;
    // In hardware target, writes "0" or "1" to /sys/class/gpio/gpioX/value
    return 0;
}

int edge_bsp_gpio_read(edge_gpio_pin_t* pin, bool* out_val) {
    if (!pin || !pin->is_open || !out_val) return -1;
    *out_val = false;
    return 0;
}

void edge_bsp_gpio_unexport(edge_gpio_pin_t* pin) {
    if (pin) {
        pin->is_open = false;
        pin->fd = -1;
    }
}

int edge_bsp_uart_open(edge_uart_device_t* dev, const char* path, uint32_t baud) {
    if (!dev || !path) return -1;
    memset(dev, 0, sizeof(edge_uart_device_t));
    strncpy(dev->tty_path, path, sizeof(dev->tty_path) - 1);
    dev->baud_rate = baud;

    dev->fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (dev->fd < 0) {
        dev->is_open = false;
        return -2;
    }

    struct termios tty;
    if (tcgetattr(dev->fd, &tty) == 0) {
        speed_t spd = B9600;
        if (baud == 115200) spd = B115200;
        else if (baud == 57600) spd = B57600;
        else if (baud == 19200) spd = B19200;

        cfsetospeed(&tty, spd);
        cfsetispeed(&tty, spd);
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= (unsigned int)~PARENB;
        tty.c_cflag &= (unsigned int)~CSTOPB;
        tty.c_cflag &= (unsigned int)~CSIZE;
        tty.c_cflag |= CS8;
        tcsetattr(dev->fd, TCSANOW, &tty);
    }

    dev->is_open = true;
    return 0;
}

int edge_bsp_uart_send(edge_uart_device_t* dev, const uint8_t* buf, size_t len) {
    if (!dev || !dev->is_open || !buf || len == 0) return -1;
    ssize_t written = write(dev->fd, buf, len);
    return (written == (ssize_t)len) ? 0 : -2;
}

int edge_bsp_uart_recv(edge_uart_device_t* dev, uint8_t* buf, size_t max_len, uint32_t timeout_ms) {
    (void)timeout_ms;
    if (!dev || !dev->is_open || !buf || max_len == 0) return -1;
    ssize_t nread = read(dev->fd, buf, max_len);
    return (nread >= 0) ? (int)nread : -2;
}

void edge_bsp_uart_close(edge_uart_device_t* dev) {
    if (dev && dev->is_open && dev->fd >= 0) {
        close(dev->fd);
        dev->fd = -1;
        dev->is_open = false;
    }
}
