#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"
#include "io_executor.h"

/* --- Linux UART 内部实现 --- */

typedef struct {
    io_handle_t base;
    uv_poll_t poll;
    int fd;
    net_buf_pool_t *pool;
    net_buf_t *pending_tx;  // 当前正在发送的 buffer
} linux_io_handle_t;

static void _on_uv_poll(uv_poll_t *handle, int status, int events) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    if (status < 0) {
        LOG_ERROR(LOG_CAT_IO, "IO Poll Error: %s", uv_strerror(status));
        impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
        return;
    }

    if (events & UV_READABLE) {
        // 遵循 0108.md：实现层预读到 net_buf
        net_buf_t *rx = net_buf_alloc(impl->pool, 0);
        if (rx) {
            ssize_t n = read(impl->fd, rx->b.data, rx->b.size);
            if (n > 0) {
                rx->b.len = (uint16_t)n;
                impl->base.cb(&impl->base, IO_EVENT_RX, rx, impl->base.user_data);
            }
            net_buf_unref(rx);
        }
    }

    if (events & UV_WRITABLE) {
        if (impl->pending_tx) {
            // 在回调之前解引用并清除，防止回调逻辑中再次访问
            net_buf_t *buf = impl->pending_tx;
            impl->pending_tx = NULL;
            impl->base.cb(&impl->base, IO_EVENT_TX_DONE, NULL, impl->base.user_data);
            net_buf_unref(buf);

            // 检查句柄是否在回调中被关闭
            if (!uv_is_closing((uv_handle_t *)&impl->poll)) {
                uv_poll_stop(&impl->poll);
                uv_poll_start(&impl->poll, UV_READABLE, _on_uv_poll);
            }
        }
    }
}

static int _linux_enable_read(io_handle_t *h, bool enable) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (uv_is_closing((uv_handle_t *)&impl->poll)) return -EBADF;
    if (enable) {
        return uv_poll_start(&impl->poll, UV_READABLE, _on_uv_poll);
    } else {
        return uv_poll_stop(&impl->poll);
    }
}

static int _linux_write_async(io_handle_t *h, net_buf_t *buf) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (impl->pending_tx) return -EBUSY;

    impl->pending_tx = net_buf_ref(buf);

    ssize_t n = write(impl->fd, buf->b.data, buf->b.len);
    if (n < 0 && errno != EAGAIN) {
        net_buf_unref(impl->pending_tx);
        impl->pending_tx = NULL;
        return -errno;
    }

    uv_poll_start(&impl->poll, UV_READABLE | UV_WRITABLE, _on_uv_poll);
    return 0;
}

static int _speed_to_baud(uint32_t speed) {
    switch (speed) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
            return B115200;
    }
}

static int _linux_ioctl(io_handle_t *h, int cmd, void *arg) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (cmd == IO_CTL_SET_BAUD) {
        uint32_t baud = *(uint32_t *)arg;
        struct termios options;
        if (tcgetattr(impl->fd, &options) != 0) return -errno;
        int speed = _speed_to_baud(baud);
        cfsetispeed(&options, speed);
        cfsetospeed(&options, speed);
        if (tcsetattr(impl->fd, TCSANOW, &options) != 0) return -errno;
        LOG_INFO(LOG_CAT_IO, "IO: Baud rate switched to %u", baud);
        return 0;
    }
    return -ENOTSUP;
}

static void _on_io_handle_close(uv_handle_t *handle) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    if (impl->fd >= 0) close(impl->fd);
    if (impl->pending_tx) net_buf_unref(impl->pending_tx);
    free(impl);
}

static void _linux_destroy(io_handle_t *h) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (!uv_is_closing((uv_handle_t *)&impl->poll)) {
        uv_close((uv_handle_t *)&impl->poll, _on_io_handle_close);
    }
}

static const io_ops_t linux_uart_ops = {.enable_read = _linux_enable_read,
                                        .write_async = _linux_write_async,
                                        .ioctl = _linux_ioctl,
                                        .destroy = _linux_destroy};

io_handle_t *io_executor_create_from_fd(uv_loop_t *loop, int fd, net_buf_pool_t *pool) {
    if (fd < 0) return NULL;

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    linux_io_handle_t *impl = calloc(1, sizeof(linux_io_handle_t));
    impl->base.ops = &linux_uart_ops;
    impl->fd = fd;
    impl->pool = pool;

    uv_poll_init(loop, &impl->poll, fd);
    impl->poll.data = impl;

    return &impl->base;
}

io_handle_t *io_executor_create_uart(uv_loop_t *loop, const char *dev_path, uint32_t baud,
                                     net_buf_pool_t *pool) {
    int fd = open(dev_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) return NULL;

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD);
    tcsetattr(fd, TCSANOW, &options);

    linux_io_handle_t *impl = calloc(1, sizeof(linux_io_handle_t));
    impl->base.ops = &linux_uart_ops;
    impl->fd = fd;
    impl->pool = pool;

    uv_poll_init(loop, &impl->poll, fd);
    impl->poll.data = impl;

    return &impl->base;
}
