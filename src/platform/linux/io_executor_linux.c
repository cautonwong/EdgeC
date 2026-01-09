#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <uv.h>

// SocketCAN headers
#include <linux/can.h>
#include <linux/can/raw.h>

#include "edge_log.h"
#include "io_executor.h"

/* --- Linux 统一 IO Handle 内部实现 --- */

typedef struct {
    io_handle_t base;
    union {
        uv_poll_t poll;
        uv_tcp_t tcp;
        uv_udp_t udp;
    } handle;
    int fd;
    net_buf_pool_t *pool;
    net_buf_t *pending_tx;
    io_type_t type;

    // UDP 专用：目标地址
    struct sockaddr_in udp_dest;
    net_buf_t *udp_rx_nb;  // 替代非法的 handle->extra
} linux_io_handle_t;

/* --- 1. 基于 Poll (FD) 的通用处理 (UART, CAN, UDS) --- */

static void _on_uv_poll(uv_poll_t *handle, int status, int events) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    if (status < 0) {
        impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
        return;
    }

    if (events & UV_READABLE) {
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
            net_buf_t *buf = impl->pending_tx;
            impl->pending_tx = NULL;
            impl->base.cb(&impl->base, IO_EVENT_TX_DONE, NULL, impl->base.user_data);
            net_buf_unref(buf);
            uv_poll_start(&impl->handle.poll, UV_READABLE, _on_uv_poll);
        }
    }
}

static int _poll_enable_read(io_handle_t *h, bool enable) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (enable) return uv_poll_start(&impl->handle.poll, UV_READABLE, _on_uv_poll);
    return uv_poll_stop(&impl->handle.poll);
}

static int _poll_write_async(io_handle_t *h, net_buf_t *buf) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (impl->pending_tx) return -EBUSY;
    impl->pending_tx = net_buf_ref(buf);
    ssize_t n = write(impl->fd, buf->b.data, buf->b.len);
    if (n < 0 && errno != EAGAIN) {
        net_buf_unref(impl->pending_tx);
        impl->pending_tx = NULL;
        return -errno;
    }
    uv_poll_start(&impl->handle.poll, UV_READABLE | UV_WRITABLE, _on_uv_poll);
    return 0;
}

/* --- 2. 基于 UDP 的处理 --- */

static void _on_udp_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    if (impl->udp_rx_nb) net_buf_unref(impl->udp_rx_nb);
    impl->udp_rx_nb = net_buf_alloc(impl->pool, 0);
    if (impl->udp_rx_nb) {
        buf->base = (char *)impl->udp_rx_nb->b.data;
        buf->len = impl->udp_rx_nb->b.size;
    } else {
        buf->base = NULL;
        buf->len = 0;
    }
}

static void _on_udp_recv(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf,
                         const struct sockaddr *addr, unsigned flags) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    (void)buf;
    (void)addr;
    (void)flags;
    if (nread > 0 && impl->udp_rx_nb) {
        impl->udp_rx_nb->b.len = (uint16_t)nread;
        impl->base.cb(&impl->base, IO_EVENT_RX, impl->udp_rx_nb, impl->base.user_data);
    }
    if (impl->udp_rx_nb) {
        net_buf_unref(impl->udp_rx_nb);
        impl->udp_rx_nb = NULL;
    }
}

static void _on_udp_send_done(uv_udp_send_t *req, int status) {
    linux_io_handle_t *impl = (linux_io_handle_t *)req->data;
    (void)status;
    impl->base.cb(&impl->base, IO_EVENT_TX_DONE, NULL, impl->base.user_data);
    free(req);
}

static int _udp_enable_read(io_handle_t *h, bool enable) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (enable) return uv_udp_recv_start(&impl->handle.udp, _on_udp_alloc, _on_udp_recv);
    return uv_udp_recv_stop(&impl->handle.udp);
}

static int _udp_write_async(io_handle_t *h, net_buf_t *buf) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    uv_udp_send_t *req = malloc(sizeof(uv_udp_send_t));
    req->data = impl;
    uv_buf_t b = uv_buf_init((char *)buf->b.data, buf->b.len);
    return uv_udp_send(req, &impl->handle.udp, &b, 1, (const struct sockaddr *)&impl->udp_dest,
                       _on_udp_send_done);
}

/* --- 3. 统一工厂与配置 --- */

static void _on_handle_close(uv_handle_t *handle) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    if (impl->fd >= 0) close(impl->fd);
    if (impl->pending_tx) net_buf_unref(impl->pending_tx);
    if (impl->udp_rx_nb) net_buf_unref(impl->udp_rx_nb);
    free(impl);
}

static void _linux_destroy(io_handle_t *h) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    uv_close((uv_handle_t *)&impl->handle, _on_handle_close);
}

static const io_ops_t poll_ops = {.enable_read = _poll_enable_read,
                                  .write_async = _poll_write_async,
                                  .ioctl = NULL,
                                  .destroy = _linux_destroy};
static const io_ops_t udp_ops = {.enable_read = _udp_enable_read,
                                 .write_async = _udp_write_async,
                                 .ioctl = NULL,
                                 .destroy = _linux_destroy};

io_handle_t *io_executor_create(uv_loop_t *loop, const io_config_t *cfg, net_buf_pool_t *pool) {
    linux_io_handle_t *impl = calloc(1, sizeof(linux_io_handle_t));
    impl->pool = pool;
    impl->type = cfg->type;
    impl->fd = -1;
    impl->handle.poll.data = impl;  // 统一设置 data

    switch (cfg->type) {
        case IO_TYPE_UART: {
            int fd = open(cfg->params.uart.dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
            if (fd < 0) {
                free(impl);
                return NULL;
            }
            impl->fd = fd;
            impl->base.ops = &poll_ops;
            uv_poll_init(loop, &impl->handle.poll, fd);
            break;
        }
        case IO_TYPE_CAN: {
            int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
            if (fd < 0) {
                free(impl);
                return NULL;
            }
            struct ifreq ifr;
            strncpy(ifr.ifr_name, cfg->params.can.ifname, IFNAMSIZ - 1);
            ioctl(fd, SIOCGIFINDEX, &ifr);
            struct sockaddr_can addr = {.can_family = AF_CAN, .can_ifindex = ifr.ifr_ifindex};
            if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                close(fd);
                free(impl);
                return NULL;
            }
            impl->fd = fd;
            impl->base.ops = &poll_ops;
            uv_poll_init(loop, &impl->handle.poll, fd);
            break;
        }
        case IO_TYPE_UDP: {
            uv_udp_init(loop, &impl->handle.udp);
            struct sockaddr_in local_addr;
            local_addr.sin_family = AF_INET;
            inet_pton(AF_INET, cfg->params.udp.local_ip, &local_addr.sin_addr);
            local_addr.sin_port = htons(cfg->params.udp.local_port);
            uv_udp_bind(&impl->handle.udp, (const struct sockaddr *)&local_addr, 0);

            impl->udp_dest.sin_family = AF_INET;
            inet_pton(AF_INET, cfg->params.udp.remote_ip, &impl->udp_dest.sin_addr);
            impl->udp_dest.sin_port = htons(cfg->params.udp.remote_port);

            impl->base.ops = &udp_ops;
            break;
        }
        default:
            free(impl);
            return NULL;
    }
    return &impl->base;
}

io_handle_t *io_executor_create_from_fd(uv_loop_t *loop, int fd, net_buf_pool_t *pool) {
    if (fd < 0) return NULL;
    linux_io_handle_t *impl = calloc(1, sizeof(linux_io_handle_t));
    impl->fd = fd;
    impl->pool = pool;
    impl->base.ops = &poll_ops;
    impl->handle.poll.data = impl;
    uv_poll_init(loop, &impl->handle.poll, fd);
    return &impl->base;
}

io_handle_t *io_executor_create_uart(uv_loop_t *loop, const char *dev_path, uint32_t baud,
                                     net_buf_pool_t *pool) {
    io_config_t cfg = {.type = IO_TYPE_UART};
    strncpy(cfg.params.uart.dev, dev_path, 63);
    cfg.params.uart.baud = baud;
    return io_executor_create(loop, &cfg, pool);
}