#include <arpa/inet.h>
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"
#include "io_executor.h"

/* --- 0. Libcurl Multi-Socket 集成 --- */

typedef struct {
    CURLM *multi;
    uv_timer_t timer;
    uv_loop_t *loop;
} curl_multi_ctx_t;

static curl_multi_ctx_t *g_curl_ctx = NULL;

typedef struct {
    uv_poll_t poll_handle;
    curl_socket_t sockfd;
} curl_poll_ctx_t;

/* --- Linux 统一 IO Handle 内部实现 --- */

typedef struct linux_io_handle_s {
    io_handle_t base;
    union {
        uv_poll_t poll;
        uv_tcp_t tcp;
        uv_udp_t udp;
        CURL *curl;
    } handle;
    int fd;
    net_buf_pool_t *pool;
    net_buf_t *pending_tx;
    io_type_t type;

    struct sockaddr_in udp_dest;
    net_buf_t *udp_rx_nb;

    // HTTP 专用
    net_buf_t *http_rx_nb;
    struct curl_slist *http_hdrs;
} linux_io_handle_t;

static void _on_curl_timeout(uv_timer_t *handle) {
    int running_handles;
    curl_multi_socket_action(g_curl_ctx->multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
}

static int _curl_timer_cb(CURLM *multi, long timeout_ms, void *userp) {
    curl_multi_ctx_t *ctx = (curl_multi_ctx_t *)userp;
    if (timeout_ms < 0) {
        uv_timer_stop(&ctx->timer);
    } else {
        if (timeout_ms == 0) timeout_ms = 1;
        uv_timer_start(&ctx->timer, _on_curl_timeout, timeout_ms, 0);
    }
    return 0;
}

static void _on_curl_poll(uv_poll_t *handle, int status, int events) {
    int running_handles;
    int flags = 0;
    curl_poll_ctx_t *poll_ctx = (curl_poll_ctx_t *)handle->data;

    if (events & UV_READABLE) flags |= CURL_CSELECT_IN;
    if (events & UV_WRITABLE) flags |= CURL_CSELECT_OUT;

    curl_multi_socket_action(g_curl_ctx->multi, poll_ctx->sockfd, flags, &running_handles);

    // 检查完成的句柄
    CURLMsg *msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(g_curl_ctx->multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL *easy = msg->easy_handle;
            linux_io_handle_t *impl = NULL;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &impl);

            if (impl) {
                if (msg->data.result == CURLE_OK) {
                    impl->base.cb(&impl->base, IO_EVENT_TX_DONE, NULL, impl->base.user_data);
                } else {
                    impl->base.cb(&impl->base, IO_EVENT_ERROR, (void *)(intptr_t)msg->data.result,
                                  impl->base.user_data);
                }
                // 从 multi 移除但不由这里 destroy，由 io_handle_destroy 处理
                curl_multi_remove_handle(g_curl_ctx->multi, easy);
            }
        }
    }
}

static int _curl_socket_cb(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp) {
    curl_multi_ctx_t *ctx = (curl_multi_ctx_t *)userp;
    curl_poll_ctx_t *poll_ctx = (curl_poll_ctx_t *)socketp;

    if (action == CURL_POLL_IN || action == CURL_POLL_OUT || action == CURL_POLL_INOUT) {
        if (!poll_ctx) {
            poll_ctx = calloc(1, sizeof(curl_poll_ctx_t));
            poll_ctx->sockfd = s;
            uv_poll_init(ctx->loop, &poll_ctx->poll_handle, s);
            poll_ctx->poll_handle.data = poll_ctx;
            curl_multi_assign(ctx->multi, s, poll_ctx);
        }
        int events = 0;
        if (action != CURL_POLL_OUT) events |= UV_READABLE;
        if (action != CURL_POLL_IN) events |= UV_WRITABLE;
        uv_poll_start(&poll_ctx->poll_handle, events, _on_curl_poll);
    } else if (action == CURL_POLL_REMOVE) {
        if (poll_ctx) {
            uv_poll_stop(&poll_ctx->poll_handle);
            uv_close((uv_handle_t *)&poll_ctx->poll_handle, (uv_close_cb)free);
            curl_multi_assign(ctx->multi, s, NULL);
        }
    }
    return 0;
}

static void _ensure_curl_init(uv_loop_t *loop) {
    if (g_curl_ctx) return;
    g_curl_ctx = calloc(1, sizeof(curl_multi_ctx_t));
    g_curl_ctx->loop = loop;
    g_curl_ctx->multi = curl_multi_init();
    uv_timer_init(loop, &g_curl_ctx->timer);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_SOCKETFUNCTION, _curl_socket_cb);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_SOCKETDATA, g_curl_ctx);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_TIMERFUNCTION, _curl_timer_cb);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_TIMERDATA, g_curl_ctx);
}

/* --- 1. 基于 Poll (FD) 的通用处理 (UART, CAN, UDS) --- */

static void _on_uv_poll(uv_poll_t *handle, int status, int events) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    if (status < 0) {
        impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
        return;
    }

    if (events & UV_READABLE) {
        while (1) {
            net_buf_t *rx = net_buf_alloc(impl->pool, 0);
            if (!rx) break;
            ssize_t n = read(impl->fd, rx->b.data, rx->b.size);
            if (n > 0) {
                rx->b.len = (uint16_t)n;
                fprintf(stderr, "[IO] FD %d READ %zd bytes\n", impl->fd, n);
                impl->base.cb(&impl->base, IO_EVENT_RX, rx, impl->base.user_data);
                net_buf_unref(rx);
            } else {
                net_buf_unref(rx);
                if (n == 0) {
                    fprintf(stderr, "[IO] FD %d peer closed\n", impl->fd);
                    impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
                } else if (errno != EAGAIN && errno != EINTR) {
                    fprintf(stderr, "[IO] FD %d read error: %s\n", impl->fd, strerror(errno));
                    impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
                }
                break;
            }
        }
    }

    if (events & UV_WRITABLE) {
        if (impl->pending_tx) {
            ssize_t n = write(impl->fd, impl->pending_tx->b.data, impl->pending_tx->b.len);
            if (n > 0) {
                fprintf(stderr, "[IO] FD %d WRITE DONE\n", impl->fd);
                net_buf_t *buf = impl->pending_tx;
                impl->pending_tx = NULL;
                uv_poll_start(handle, UV_READABLE, _on_uv_poll);
                impl->base.cb(&impl->base, IO_EVENT_TX_DONE, NULL, impl->base.user_data);
                net_buf_unref(buf);
            } else if (errno != EAGAIN && errno != EINTR) {
                fprintf(stderr, "[IO] FD %d write error: %s\n", impl->fd, strerror(errno));
                impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
            }
        } else {
            uv_poll_start(handle, UV_READABLE, _on_uv_poll);
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
    return uv_poll_start(&impl->handle.poll, UV_READABLE | UV_WRITABLE, _on_uv_poll);
}

static int _poll_ioctl(io_handle_t *h, int cmd, void *arg) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (cmd == IO_CTL_SET_BAUD && impl->type == IO_TYPE_UART) {
        uint32_t baud = *(uint32_t *)arg;
        struct termios options;
        if (tcgetattr(impl->fd, &options) != 0) return -errno;
        speed_t speed = B9600;
        switch (baud) {
            case 9600:
                speed = B9600;
                break;
            case 19200:
                speed = B19200;
                break;
            case 38400:
                speed = B38400;
                break;
            case 57600:
                speed = B57600;
                break;
            case 115200:
                speed = B115200;
                break;
            default:
                speed = B9600;
                break;
        }
        cfsetispeed(&options, speed);
        cfsetospeed(&options, speed);
        if (tcsetattr(impl->fd, TCSANOW, &options) != 0) return -errno;
        LOG_INFO(LOG_CAT_IO, "IO: Baud rate switched to %u", baud);
        return 0;
    }
    return -1;
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

/* --- 3. 基于 HTTP (Libcurl) 的处理 --- */

static size_t _on_curl_write(void *ptr, size_t size, size_t nmemb, void *userp) {
    linux_io_handle_t *impl = (linux_io_handle_t *)userp;
    size_t total = size * nmemb;

    // 如果没有分配 rx_nb，则分配一个 (按需)
    if (!impl->http_rx_nb) {
        impl->http_rx_nb = net_buf_alloc(impl->pool, 0);
    }

    if (impl->http_rx_nb && net_buf_simple_tailroom(&impl->http_rx_nb->b) >= total) {
        net_buf_simple_add_mem(&impl->http_rx_nb->b, ptr, total);
        // 通知上层：收到数据片段
        impl->base.cb(&impl->base, IO_EVENT_RX, impl->http_rx_nb, impl->base.user_data);
    }

    return total;
}

static int _http_enable_read(io_handle_t *h, bool enable) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (enable) {
        curl_multi_add_handle(g_curl_ctx->multi, impl->handle.curl);
    } else {
        curl_multi_remove_handle(g_curl_ctx->multi, impl->handle.curl);
    }
    return 0;
}

static int _http_write_async(io_handle_t *h, net_buf_t *buf) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    // 如果有 Payload (POST/PUT)
    if (buf && buf->b.len > 0) {
        curl_easy_setopt(impl->handle.curl, CURLOPT_POSTFIELDS, buf->b.data);
        curl_easy_setopt(impl->handle.curl, CURLOPT_POSTFIELDSIZE, (long)buf->b.len);
    }
    // 启动传输
    curl_multi_add_handle(g_curl_ctx->multi, impl->handle.curl);
    return 0;
}

static int _http_ioctl(io_handle_t *h, int cmd, void *arg) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    switch (cmd) {
        case IO_CTL_HTTP_SET_URL:
            curl_easy_setopt(impl->handle.curl, CURLOPT_URL, (const char *)arg);
            break;
        case IO_CTL_HTTP_SET_METHOD:
            curl_easy_setopt(impl->handle.curl, CURLOPT_CUSTOMREQUEST, (const char *)arg);
            break;
        case IO_CTL_HTTP_ADD_HEADER:
            impl->http_hdrs = curl_slist_append(impl->http_hdrs, (const char *)arg);
            curl_easy_setopt(impl->handle.curl, CURLOPT_HTTPHEADER, impl->http_hdrs);
            break;
        default:
            return -1;
    }
    return 0;
}

static void _http_destroy(io_handle_t *h) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    curl_multi_remove_handle(g_curl_ctx->multi, impl->handle.curl);
    curl_easy_cleanup(impl->handle.curl);
    if (impl->http_hdrs) curl_slist_free_all(impl->http_hdrs);
    if (impl->http_rx_nb) net_buf_unref(impl->http_rx_nb);
    free(impl);
}

static const io_ops_t http_ops = {.enable_read = _http_enable_read,
                                  .write_async = _http_write_async,
                                  .ioctl = _http_ioctl,
                                  .destroy = _http_destroy};

static void _on_handle_close(uv_handle_t *handle) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    if (impl->type == IO_TYPE_HTTP) {
        // HTTP 的销毁通过 ops->destroy 直接完成
        return;
    }
    if (impl->fd >= 0) close(impl->fd);
    if (impl->pending_tx) net_buf_unref(impl->pending_tx);
    if (impl->udp_rx_nb) net_buf_unref(impl->udp_rx_nb);
    free(impl);
}

static void _linux_destroy(io_handle_t *h) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (impl->type == IO_TYPE_HTTP) {
        _http_destroy(h);
        return;
    }
    uv_close((uv_handle_t *)&impl->handle, _on_handle_close);
}

static const io_ops_t poll_ops = {.enable_read = _poll_enable_read,
                                  .write_async = _poll_write_async,
                                  .ioctl = _poll_ioctl,
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
    impl->handle.poll.data = impl;

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
        case IO_TYPE_UDS: {
            int fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) {
                free(impl);
                return NULL;
            }
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, cfg->params.uds.path, sizeof(addr.sun_path) - 1);
            unlink(addr.sun_path);
            if (bind(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(fd, 5) < 0) {
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
        case IO_TYPE_HTTP: {
            _ensure_curl_init(loop);
            impl->handle.curl = curl_easy_init();
            if (!impl->handle.curl) {
                free(impl);
                return NULL;
            }
            curl_easy_setopt(impl->handle.curl, CURLOPT_URL, cfg->params.http.url);
            if (strlen(cfg->params.http.method) > 0) {
                curl_easy_setopt(impl->handle.curl, CURLOPT_CUSTOMREQUEST, cfg->params.http.method);
            }
            curl_easy_setopt(impl->handle.curl, CURLOPT_WRITEFUNCTION, _on_curl_write);
            curl_easy_setopt(impl->handle.curl, CURLOPT_WRITEDATA, impl);
            curl_easy_setopt(impl->handle.curl, CURLOPT_PRIVATE, impl);
            curl_easy_setopt(impl->handle.curl, CURLOPT_FOLLOWLOCATION, 1L);
            impl->base.ops = &http_ops;
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
