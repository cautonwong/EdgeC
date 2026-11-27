#include <arpa/inet.h>
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>
#include <uv.h>

#include "edge_log.h"
#include "io_executor.h"
#include "io_executor_linux.h"
#include "io_handle_priv.h"

/* --- 0. Libcurl Multi-Socket 集成 --- */

typedef struct {
    CURLM* multi;
    uv_timer_t timer;
    uv_loop_t* loop;
} curl_multi_ctx_t;

static curl_multi_ctx_t* g_curl_ctx = NULL;

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
        CURL* curl;
    } handle;
    int fd;
    net_buf_pool_t* pool;
    net_buf_t* pending_tx;
    size_t tx_off; /* pending_tx 已发出的字节数：非阻塞 fd 允许部分写 */
    io_type_t type;
    /* 产品层的链路存亡观察者：与 base.cb（引擎消费数据）分开 */
    void (*link_cb)(io_handle_t* h, io_event_t evt, void* user_data);
    void* link_user_data;

    struct sockaddr_in udp_dest;
    net_buf_t* udp_rx_nb;

    // HTTP 专用
    net_buf_t* http_rx_nb;
    struct curl_slist* http_hdrs;
} linux_io_handle_t;

static void _on_curl_timeout(uv_timer_t* handle) {
    (void)handle;
    int running_handles;
    curl_multi_socket_action(g_curl_ctx->multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
}

static int _curl_timer_cb(CURLM* multi, long timeout_ms, void* userp) {
    (void)multi;
    curl_multi_ctx_t* ctx = (curl_multi_ctx_t*)userp;
    if (timeout_ms < 0) {
        uv_timer_stop(&ctx->timer);
    } else {
        if (timeout_ms == 0) timeout_ms = 1;
        uv_timer_start(&ctx->timer, _on_curl_timeout, (uint64_t)timeout_ms, 0);
    }
    return 0;
}

static void _on_curl_poll(uv_poll_t* handle, int status, int events) {
    (void)status;
    int running_handles;
    int flags = 0;
    curl_poll_ctx_t* poll_ctx = (curl_poll_ctx_t*)handle->data;

    if (events & UV_READABLE) flags |= CURL_CSELECT_IN;
    if (events & UV_WRITABLE) flags |= CURL_CSELECT_OUT;

    curl_multi_socket_action(g_curl_ctx->multi, poll_ctx->sockfd, flags, &running_handles);

    CURLMsg* msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(g_curl_ctx->multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* easy = msg->easy_handle;
            linux_io_handle_t* impl = NULL;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &impl);

            if (impl) {
                /* 回调可能 destroy 句柄并释放 impl：先摘句柄、取走 pending，
                 * 回调后只 unref 本地保存的那份，不再碰 impl/easy。 */
                net_buf_t* pending = impl->pending_tx;
                impl->pending_tx = NULL;
                curl_multi_remove_handle(g_curl_ctx->multi, easy);
                if (msg->data.result == CURLE_OK) {
                    impl->base.cb(&impl->base, IO_EVENT_TX_DONE, NULL, impl->base.user_data);
                } else {
                    impl->base.cb(&impl->base, IO_EVENT_ERROR, (void*)(intptr_t)msg->data.result,
                                  impl->base.user_data);
                }
                if (pending) net_buf_unref(pending);
            }
        }
    }
}

static int _curl_socket_cb(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp) {
    (void)easy;
    curl_multi_ctx_t* ctx = (curl_multi_ctx_t*)userp;
    curl_poll_ctx_t* poll_ctx = (curl_poll_ctx_t*)socketp;

    if (action == CURL_POLL_IN || action == CURL_POLL_OUT || action == CURL_POLL_INOUT) {
        if (!poll_ctx) {
            poll_ctx = calloc(1, sizeof(curl_poll_ctx_t));
            if (!poll_ctx) return -1;
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
            uv_close((uv_handle_t*)&poll_ctx->poll_handle, (uv_close_cb)free);
            curl_multi_assign(ctx->multi, s, NULL);
        }
    }
    return 0;
}

static void _ensure_curl_init(uv_loop_t* loop) {
    if (g_curl_ctx) return;
    g_curl_ctx = calloc(1, sizeof(curl_multi_ctx_t));
    if (!g_curl_ctx) return;
    g_curl_ctx->loop = loop;
    g_curl_ctx->multi = curl_multi_init();
    if (!g_curl_ctx->multi) {
        free(g_curl_ctx);
        g_curl_ctx = NULL;
        return;
    }
    uv_timer_init(loop, &g_curl_ctx->timer);
    g_curl_ctx->timer.data = g_curl_ctx;
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_SOCKETFUNCTION, _curl_socket_cb);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_SOCKETDATA, g_curl_ctx);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_TIMERFUNCTION, _curl_timer_cb);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_TIMERDATA, g_curl_ctx);
}

static void _on_curl_ctx_close(uv_handle_t* handle) {
    free(handle->data);
}

int io_handle_enable_read(io_handle_t *h, bool enable) {
    if (!h || !h->ops || !h->ops->enable_read) return -1;
    return h->ops->enable_read(h, enable);
}

void io_handle_destroy(io_handle_t *h) {
    if (!h || !h->ops || !h->ops->destroy) return;
    h->ops->destroy(h);
}

void io_handle_set_link_cb(io_handle_t *h,
                           void (*cb)(io_handle_t *h, io_event_t evt, void *user_data),
                           void *user_data) {
    if (!h) return;
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    impl->link_cb = cb;
    impl->link_user_data = user_data;
}

/* 引擎的 cb 先结算，链路观察者后拿事件；正常数据路径不经过这里。 */
static void _notify_link(linux_io_handle_t *impl) {
    if (impl->link_cb) impl->link_cb(&impl->base, IO_EVENT_ERROR, impl->link_user_data);
}

void io_executor_cleanup(void) {
    if (g_curl_ctx) {
        uv_timer_stop(&g_curl_ctx->timer);
        curl_multi_cleanup(g_curl_ctx->multi);
        g_curl_ctx->multi = NULL;
        uv_close((uv_handle_t*)&g_curl_ctx->timer, _on_curl_ctx_close);
        g_curl_ctx = NULL;
    }
}

/* --- 1. 基于 Poll (FD) 的通用处理 (UART, CAN, UDS) --- */

static speed_t _uart_baud_to_speed(uint32_t baud) {
    switch (baud) {
        case 2400:
            return B2400;
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
            /* 0 不是合法线速: 调用方必须拒绝, 静默退回 9600 会改错 wire speed */
            return (speed_t)0;
    }
}

bool io_executor_uart_baud_supported(uint32_t baud) {
    return _uart_baud_to_speed(baud) != (speed_t)0;
}

static void _on_uv_poll(uv_poll_t* handle, int status, int events) {
    linux_io_handle_t* impl = (linux_io_handle_t*)handle->data;
    if (status < 0) {
        /* 先停 watcher 再回调：错误条件不会自愈，继续武装会让 libuv 每轮重报；
         * 回调可能销毁句柄，之后不能再碰 impl */
        uv_poll_stop(handle);
        impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
        _notify_link(impl);
        return;
    }

    if (events & UV_READABLE) {
        uint8_t buf[256];
        ssize_t ret;
        do {
            /* tty 不是 socket，recv() 会返回 ENOTSOCK；UART fd 必须用 read() */
            ret = impl->type == IO_TYPE_UART ? read(impl->fd, buf, sizeof(buf))
                                             : recv(impl->fd, buf, sizeof(buf), MSG_DONTWAIT);
            if (ret > 0) {
                net_buf_t* nb = net_buf_alloc(impl->pool, 0);
                if (nb && (size_t)ret <= nb->b.size) {
                    memcpy(nb->b.data, buf, (size_t)ret);
                    nb->b.len = (uint16_t)ret;
                    if (impl->base.cb)
                        impl->base.cb(&impl->base, IO_EVENT_RX, nb, impl->base.user_data);
                }
                if (nb) net_buf_unref(nb);
            } else if (ret == 0 ||
                       (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
                /* 对端关闭/硬错误：EOF 会让 fd 一直可读。先停读再回调，否则每次
                 * loop 迭代都重报一次错误空转 CPU；回调后不再碰 impl */
                uv_poll_stop(handle);
                if (impl->base.cb)
                    impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
                _notify_link(impl);
                return;
            }
        } while (ret > 0);
    }

    if (events & UV_WRITABLE) {
        if (impl->pending_tx) {
            net_buf_t* buf = impl->pending_tx;
            size_t off = impl->tx_off;
            ssize_t n = write(impl->fd, buf->b.data + off, (size_t)buf->b.len - off);
            if (n > 0 && off + (size_t)n < buf->b.len) {
                /* 非阻塞 socket/tty 可能只发一部分：保留未发段、保持可写，
                 * 整帧发完才报 TX_DONE；提前结算会在线上留下截断帧 */
                impl->tx_off = off + (size_t)n;
                return;
            }
            if (n > 0) {
                fprintf(stderr, "[IO-TX] FD:%d Sent %zu bytes SUCCESS\n", impl->fd,
                        off + (size_t)n);
                impl->pending_tx = NULL;
                impl->tx_off = 0;
                // 恢复只读监听
                uv_poll_start(handle, UV_READABLE, _on_uv_poll);
                impl->base.cb(&impl->base, IO_EVENT_TX_DONE, NULL, impl->base.user_data);
                net_buf_unref(buf);
            } else if (n < 0 && errno != EAGAIN && errno != EINTR) {
                fprintf(stderr, "[IO-TX-ERR] FD:%d Write Failed: %s\n", impl->fd, strerror(errno));
                /* 硬错误后不会再有 TX_DONE:这一帧的所有权在这里结清,否则永远挂在池外 */
                net_buf_unref(impl->pending_tx);
                impl->pending_tx = NULL;
                impl->tx_off = 0;
                /* 写侧硬错误同样不会自愈：先停再回调，避免可写事件空转 */
                uv_poll_stop(handle);
                impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
                _notify_link(impl);
                return;
            }
        } else {
            uv_poll_start(handle, UV_READABLE, _on_uv_poll);
        }
    }
}

static int _poll_enable_read(io_handle_t* h, bool enable) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    if (enable) return uv_poll_start(&impl->handle.poll, UV_READABLE, _on_uv_poll);
    return uv_poll_stop(&impl->handle.poll);
}

static int _poll_write_async(io_handle_t* h, net_buf_t* buf) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    if (impl->pending_tx) return -EBUSY;
    impl->pending_tx = net_buf_ref(buf);
    impl->tx_off = 0;
    /* 写帧期间只等可写:半双工事务里,帧没写完就收到的字节属于上一条命令,
     * 这时开读会被当成这一条命令的应答(串 tag)。整帧写完后 _on_uv_poll
     * 自己切回只读。 */
    int rc = uv_poll_start(&impl->handle.poll, UV_WRITABLE, _on_uv_poll);
    if (rc != 0) {
        net_buf_unref(impl->pending_tx);
        impl->pending_tx = NULL;
        impl->tx_off = 0;
    }
    return rc;
}

/* 排空已到达内核 RX 队列、但还没被读回调取走的字节。
 * 只用于事务边界(开新帧前 / 超时收尾后):这些字节属于已经结算的事务,
 * 留给下一条命令就会被当成它的应答(RTU 读应答没有寄存器地址可校验)。 */
static void _drain_rx(linux_io_handle_t* impl) {
    if (impl->type == IO_TYPE_UART && tcflush(impl->fd, TCIFLUSH) == 0) return;
    uint8_t trash[256];
    ssize_t ret;
    do {
        /* tty 不是 socket，recv() 会返回 ENOTSOCK；UART fd 必须用 read() */
        ret = impl->type == IO_TYPE_UART ? read(impl->fd, trash, sizeof(trash))
                                         : recv(impl->fd, trash, sizeof(trash), MSG_DONTWAIT);
    } while (ret > 0);
}

static int _poll_ioctl(io_handle_t* h, int cmd, void* arg) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    if (cmd == IO_CTL_FLUSH) {
        if (impl->pending_tx) {
            fprintf(stderr, "[IO] Forced FLUSH: Releasing pending TX buf\n");
            net_buf_unref(impl->pending_tx);
            impl->pending_tx = NULL;
        }
        // Drain OS RX buffer non-blockingly to prevent sequence mismatch from delayed responses
        _drain_rx(impl);
        return 0;
    }
    if (cmd == IO_CTL_DISCARD_RX) {
        _drain_rx(impl);
        return 0;
    }
    if (cmd == IO_CTL_SET_BAUD && impl->type == IO_TYPE_UART) {
        uint32_t baud = *(uint32_t*)arg;
        struct termios options;
        if (tcgetattr(impl->fd, &options) != 0) return -errno;
        speed_t speed = _uart_baud_to_speed(baud);
        if (speed == (speed_t)0) return -EINVAL;
        cfsetispeed(&options, speed);
        cfsetospeed(&options, speed);
        if (tcsetattr(impl->fd, TCSANOW, &options) != 0) return -errno;
        return 0;
    }
    return -1;
}

/* --- 2. 基于 UDP 的处理 --- */

static void _on_udp_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    (void)suggested_size;
    linux_io_handle_t* impl = (linux_io_handle_t*)handle->data;
    if (impl->udp_rx_nb) net_buf_unref(impl->udp_rx_nb);
    impl->udp_rx_nb = net_buf_alloc(impl->pool, 0);
    if (impl->udp_rx_nb) {
        buf->base = (char*)impl->udp_rx_nb->b.data;
        buf->len = impl->udp_rx_nb->b.size;
    } else {
        buf->base = NULL;
        buf->len = 0;
    }
}

static void _on_udp_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf,
                         const struct sockaddr* addr, unsigned flags) {
    (void)buf;
    (void)addr;
    (void)flags;
    linux_io_handle_t* impl = (linux_io_handle_t*)handle->data;
    if (nread > 0 && impl->udp_rx_nb) {
        impl->udp_rx_nb->b.len = (uint16_t)nread;
        impl->base.cb(&impl->base, IO_EVENT_RX, impl->udp_rx_nb, impl->base.user_data);
    }
    if (impl->udp_rx_nb) {
        net_buf_unref(impl->udp_rx_nb);
        impl->udp_rx_nb = NULL;
    }
}

typedef struct {
    uv_udp_send_t req;
    linux_io_handle_t* impl;
    net_buf_t* buf;
} udp_send_ctx_t;

static void _on_udp_send_done(uv_udp_send_t* req, int status) {
    (void)status;
    udp_send_ctx_t* ctx = (udp_send_ctx_t*)req;
    ctx->impl->base.cb(&ctx->impl->base, IO_EVENT_TX_DONE, NULL, ctx->impl->base.user_data);
    net_buf_unref(ctx->buf);
    free(ctx);
}

static int _udp_enable_read(io_handle_t* h, bool enable) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    if (enable) return uv_udp_recv_start(&impl->handle.udp, _on_udp_alloc, _on_udp_recv);
    return uv_udp_recv_stop(&impl->handle.udp);
}

static int _udp_write_async(io_handle_t* h, net_buf_t* buf) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    udp_send_ctx_t* req = malloc(sizeof(udp_send_ctx_t));
    if (!req) return -1;
    req->impl = impl;
    req->buf = net_buf_ref(buf);
    uv_buf_t b = uv_buf_init((char*)buf->b.data, buf->b.len);
    int rc = uv_udp_send(&req->req, &impl->handle.udp, &b, 1,
                         (const struct sockaddr*)&impl->udp_dest, _on_udp_send_done);
    if (rc != 0) {
        net_buf_unref(req->buf);
        free(req);
    }
    return rc;
}

/* --- 3. 基于 HTTP (Libcurl) 的处理 --- */

static size_t _on_curl_write(void* ptr, size_t size, size_t nmemb, void* userp) {
    linux_io_handle_t* impl = (linux_io_handle_t*)userp;
    size_t total = size * nmemb;
    if (!impl->http_rx_nb) impl->http_rx_nb = net_buf_alloc(impl->pool, 0);
    if (impl->http_rx_nb && net_buf_simple_tailroom(&impl->http_rx_nb->b) >= total) {
        net_buf_simple_add_mem(&impl->http_rx_nb->b, ptr, total);
        impl->base.cb(&impl->base, IO_EVENT_RX, impl->http_rx_nb, impl->base.user_data);
    }
    return total;
}

static int _http_enable_read(io_handle_t* h, bool enable) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    if (enable)
        curl_multi_add_handle(g_curl_ctx->multi, impl->handle.curl);
    else
        curl_multi_remove_handle(g_curl_ctx->multi, impl->handle.curl);
    return 0;
}

static int _http_write_async(io_handle_t* h, net_buf_t* buf) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    if (impl->pending_tx) return -EBUSY;
    if (buf && buf->b.len > 0) {
        curl_easy_setopt(impl->handle.curl, CURLOPT_POSTFIELDS, buf->b.data);
        curl_easy_setopt(impl->handle.curl, CURLOPT_POSTFIELDSIZE, (long)buf->b.len);
    }
    if (buf) impl->pending_tx = net_buf_ref(buf);
    CURLMcode mrc = curl_multi_add_handle(g_curl_ctx->multi, impl->handle.curl);
    /* 句柄已在 multi（如 enable_read 已加入）：引用交给在途传输，完成时释放。 */
    if (mrc == CURLM_ADDED_ALREADY) return 0;
    if (mrc != CURLM_OK) {
        if (impl->pending_tx) {
            net_buf_unref(impl->pending_tx);
            impl->pending_tx = NULL;
        }
        curl_easy_setopt(impl->handle.curl, CURLOPT_POSTFIELDS, NULL);
        curl_easy_setopt(impl->handle.curl, CURLOPT_POSTFIELDSIZE, 0L);
        return -(int)mrc;
    }
    return 0;
}

static int _http_ioctl(io_handle_t* h, int cmd, void* arg) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    switch (cmd) {
        case IO_CTL_HTTP_SET_URL:
            curl_easy_setopt(impl->handle.curl, CURLOPT_URL, (const char*)arg);
            break;
        case IO_CTL_HTTP_SET_METHOD:
            curl_easy_setopt(impl->handle.curl, CURLOPT_CUSTOMREQUEST, (const char*)arg);
            break;
        case IO_CTL_HTTP_ADD_HEADER:
            impl->http_hdrs = curl_slist_append(impl->http_hdrs, (const char*)arg);
            curl_easy_setopt(impl->handle.curl, CURLOPT_HTTPHEADER, impl->http_hdrs);
            break;
        default:
            return -1;
    }
    return 0;
}

static void _http_destroy(io_handle_t* h) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    if (g_curl_ctx && g_curl_ctx->multi) {
        curl_multi_remove_handle(g_curl_ctx->multi, impl->handle.curl);
    }
    curl_easy_cleanup(impl->handle.curl);
    if (impl->http_hdrs) curl_slist_free_all(impl->http_hdrs);
    if (impl->http_rx_nb) net_buf_unref(impl->http_rx_nb);
    if (impl->pending_tx) net_buf_unref(impl->pending_tx);
    free(impl);
}

static const io_ops_t http_ops = {.enable_read = _http_enable_read,
                                  .write_async = _http_write_async,
                                  .ioctl = _http_ioctl,
                                  .destroy = _http_destroy};

/* --- 4. 基于 MQTT (Paho Async) 的处理 --- */

#include "MQTTAsync.h"

typedef struct {
    MQTTAsync client;
    bool connected;
    char current_topic[128];
    linux_io_handle_t* impl;
} mqtt_ctx_t;

static void _on_mqtt_connect_success(void* context, MQTTAsync_successData* response) {
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    ctx->connected = true;
    LOG_INFO(LOG_CAT_CORE, "MQTT Connected to Broker: %s", (char*)response->alt.connect.serverURI);
    ctx->impl->base.cb(&ctx->impl->base, IO_EVENT_TX_DONE, NULL, ctx->impl->base.user_data);
}

static void _on_mqtt_connect_failure(void* context, MQTTAsync_failureData* response) {
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    LOG_ERROR(LOG_CAT_CORE, "MQTT Connection Failed, code: %d", response ? response->code : -1);
    ctx->impl->base.cb(&ctx->impl->base, IO_EVENT_ERROR, NULL, ctx->impl->base.user_data);
}

static int _on_mqtt_msg_arrived(void* context, char* topicName, int topicLen,
                                MQTTAsync_message* message) {
    (void)topicLen;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)context;
    net_buf_t* nb = net_buf_alloc(ctx->impl->pool, 0);
    if (nb) {
        net_buf_append_bytes(nb, (uint8_t*)message->payload, (size_t)message->payloadlen);
        ctx->impl->base.cb(&ctx->impl->base, IO_EVENT_RX, nb, ctx->impl->base.user_data);
        net_buf_unref(nb);
    }
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

static int _mqtt_enable_read(io_handle_t* h, bool enable) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)impl->handle.poll.data;
    if (enable && !ctx->connected) {
        MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
        conn_opts.onSuccess = _on_mqtt_connect_success;
        conn_opts.onFailure = _on_mqtt_connect_failure;
        conn_opts.context = ctx;
        conn_opts.cleansession = 1;
        return MQTTAsync_connect(ctx->client, &conn_opts);
    }
    return 0;
}

static int _mqtt_write_async(io_handle_t* h, net_buf_t* buf) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)impl->handle.poll.data;
    if (!ctx->connected) return -ENOTCONN;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.context = ctx;
    int rc = MQTTAsync_send(ctx->client, ctx->current_topic, buf->b.len, buf->b.data, 1, 0, &opts);
    return (rc == MQTTASYNC_SUCCESS) ? 0 : -1;
}

static int _mqtt_ioctl(io_handle_t* h, int cmd, void* arg) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)impl->handle.poll.data;
    switch (cmd) {
        case IO_CTL_MQTT_SET_TOPIC:
            strncpy(ctx->current_topic, (const char*)arg, 127);
            ctx->current_topic[sizeof(ctx->current_topic) - 1] = '\0';
            return 0;
        case IO_CTL_MQTT_SUBSCRIBE:
            return MQTTAsync_subscribe(ctx->client, (const char*)arg, 1, NULL);
        default:
            return -1;
    }
}

static void _mqtt_destroy(io_handle_t* h) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    mqtt_ctx_t* ctx = (mqtt_ctx_t*)impl->handle.poll.data;
    if (ctx->client) {
        MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
        MQTTAsync_disconnect(ctx->client, &disc_opts);
        MQTTAsync_destroy(&ctx->client);
    }
    free(ctx);
    free(impl);
}

static const io_ops_t mqtt_ops = {.enable_read = _mqtt_enable_read,
                                  .write_async = _mqtt_write_async,
                                  .ioctl = _mqtt_ioctl,
                                  .destroy = _mqtt_destroy};

/* --- 5. 统一入口与销毁 --- */

static void _on_handle_close(uv_handle_t* handle) {
    linux_io_handle_t* impl = (linux_io_handle_t*)handle->data;
    if (impl->fd >= 0) close(impl->fd);
    if (impl->pending_tx) net_buf_unref(impl->pending_tx);
    if (impl->udp_rx_nb) net_buf_unref(impl->udp_rx_nb);
    free(impl);
}

static void _linux_destroy(io_handle_t* h) {
    linux_io_handle_t* impl = (linux_io_handle_t*)h;
    if (impl->type == IO_TYPE_HTTP) {
        _http_destroy(h);
        return;
    }
    if (impl->type == IO_TYPE_MQTT) {
        _mqtt_destroy(h);
        return;
    }
    uv_close((uv_handle_t*)&impl->handle, _on_handle_close);
}

static const io_ops_t poll_ops = {.enable_read = _poll_enable_read,
                                  .write_async = _poll_write_async,
                                  .ioctl = _poll_ioctl,
                                  .destroy = _linux_destroy};
static const io_ops_t udp_ops = {.enable_read = _udp_enable_read,
                                 .write_async = _udp_write_async,
                                 .ioctl = NULL,
                                 .destroy = _linux_destroy};

/* keepalive 探测失败只告警：内核拒绝探测参数不能断链。 */
static void _tcp_apply_keepalive(int fd, uint32_t idle_s, uint32_t intvl_s, uint32_t cnt) {
    int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) != 0) {
        int err = errno;
        LOG_WARN(LOG_CAT_IO, "io_executor: SO_KEEPALIVE failed on FD %d: %s (%d)", fd,
                 strerror(err), err);
        return;
    }
    int idle = (int)(idle_s ? idle_s : IO_TCP_KEEPALIVE_IDLE_S);
    int intvl = (int)(intvl_s ? intvl_s : IO_TCP_KEEPALIVE_INTVL_S);
    int probes = (int)(cnt ? cnt : IO_TCP_KEEPALIVE_CNT);
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) != 0) {
        int err = errno;
        LOG_WARN(LOG_CAT_IO, "io_executor: TCP_KEEPIDLE failed on FD %d: %s (%d)", fd,
                 strerror(err), err);
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl)) != 0) {
        int err = errno;
        LOG_WARN(LOG_CAT_IO, "io_executor: TCP_KEEPINTVL failed on FD %d: %s (%d)", fd,
                 strerror(err), err);
    }
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(probes)) != 0) {
        int err = errno;
        LOG_WARN(LOG_CAT_IO, "io_executor: TCP_KEEPCNT failed on FD %d: %s (%d)", fd, strerror(err),
                 err);
    }
}

static int _tcp_connect_fd(const char* host, uint16_t port) {
    struct addrinfo hints;
    struct addrinfo* res = NULL;
    char port_str[6];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);
    if (getaddrinfo(host, port_str, &hints, &res) != 0) return -1;

    int fd = -1;
    for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(fd);
            fd = -1;
            continue;
        }
        /* SO_SNDTIMEO 不约束 connect；非阻塞 connect 加 1 秒 poll 才是真正的启动上限 */
        int rc = connect(fd, ai->ai_addr, ai->ai_addrlen);
        if (rc != 0) {
            if (errno != EINPROGRESS) {
                close(fd);
                fd = -1;
                continue;
            }
            struct pollfd pfd = {.fd = fd, .events = POLLOUT};
            int prc;
            do {
                prc = poll(&pfd, 1, 1000);
            } while (prc < 0 && errno == EINTR);
            int soerr = 0;
            socklen_t len = sizeof(soerr);
            if (prc <= 0 || getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &len) != 0 ||
                soerr != 0) {
                close(fd);
                fd = -1;
                continue;
            }
        }
        /* TCP 半开探测：对端消失无 RST 时由内核 keepalive 触发断链 */
        _tcp_apply_keepalive(fd, 0, 0, 0);
        break;
    }
    freeaddrinfo(res);
    return fd;
}

int io_executor_linux_fd(const io_handle_t* h) {
    if (!h) return -1;
    return ((const linux_io_handle_t*)h)->fd;
}

io_handle_t* io_executor_create(uv_loop_t* loop, const io_config_t* cfg, net_buf_pool_t* pool) {
    linux_io_handle_t* impl = calloc(1, sizeof(linux_io_handle_t));
    if (!impl) return NULL;
    impl->pool = pool;
    impl->type = cfg->type;
    impl->fd = -1;
    switch (cfg->type) {
        case IO_TYPE_UART: {
            int fd = open(cfg->params.uart.dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
            if (fd < 0) {
                free(impl);
                return NULL;
            }
            /* 默认 canonical 模式会扣住无换行的数据；创建时就把 tty 设为 raw 并应用波特率 */
            struct termios tio;
            if (tcgetattr(fd, &tio) != 0) {
                close(fd);
                free(impl);
                return NULL;
            }
            cfmakeraw(&tio);
            speed_t speed = _uart_baud_to_speed(cfg->params.uart.baud);
            if (speed == (speed_t)0) {
                LOG_ERROR(LOG_CAT_IO, "io_executor: unsupported UART baud %u for %s",
                          cfg->params.uart.baud, cfg->params.uart.dev);
                close(fd);
                free(impl);
                return NULL;
            }
            cfsetispeed(&tio, speed);
            cfsetospeed(&tio, speed);
            if (tcsetattr(fd, TCSANOW, &tio) != 0) {
                close(fd);
                free(impl);
                return NULL;
            }
            impl->fd = fd;
            impl->base.ops = &poll_ops;
            if (uv_poll_init(loop, &impl->handle.poll, fd) < 0) {
                close(fd);
                free(impl);
                return NULL;
            }
            impl->handle.poll.data = impl;
            break;
        }
        case IO_TYPE_TCP: {
            int fd = _tcp_connect_fd(cfg->params.tcp.host, cfg->params.tcp.port);
            if (fd < 0) {
                free(impl);
                return NULL;
            }
            int flags = fcntl(fd, F_GETFL, 0);
            if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
                close(fd);
                free(impl);
                return NULL;
            }
            impl->fd = fd;
            impl->base.ops = &poll_ops;
            if (uv_poll_init(loop, &impl->handle.poll, fd) < 0) {
                close(fd);
                free(impl);
                return NULL;
            }
            impl->handle.poll.data = impl;
            break;
        }
        case IO_TYPE_UDP: {
            uv_udp_init(loop, &impl->handle.udp);
            impl->handle.udp.data = impl;
            impl->udp_dest.sin_family = AF_INET;
            inet_pton(AF_INET, cfg->params.udp.remote_ip, &impl->udp_dest.sin_addr);
            impl->udp_dest.sin_port = htons(cfg->params.udp.remote_port);
            impl->base.ops = &udp_ops;
            break;
        }
        case IO_TYPE_HTTP: {
            _ensure_curl_init(loop);
            if (!g_curl_ctx) {
                free(impl);
                return NULL;
            }
            impl->handle.curl = curl_easy_init();
            if (!impl->handle.curl) {
                free(impl);
                return NULL;
            }
            curl_easy_setopt(impl->handle.curl, CURLOPT_URL, cfg->params.http.url);
            curl_easy_setopt(impl->handle.curl, CURLOPT_WRITEFUNCTION, _on_curl_write);
            curl_easy_setopt(impl->handle.curl, CURLOPT_WRITEDATA, impl);
            curl_easy_setopt(impl->handle.curl, CURLOPT_PRIVATE, impl);
            curl_easy_setopt(impl->handle.curl, CURLOPT_CONNECTTIMEOUT, 2L);
            curl_easy_setopt(impl->handle.curl, CURLOPT_TIMEOUT, 3L);
            curl_easy_setopt(impl->handle.curl, CURLOPT_NOSIGNAL, 1L);
            impl->base.ops = &http_ops;
            break;
        }
        case IO_TYPE_MQTT: {
            mqtt_ctx_t* mctx = calloc(1, sizeof(mqtt_ctx_t));
            if (!mctx) {
                free(impl);
                return NULL;
            }
            mctx->impl = impl;
            int rc =
                MQTTAsync_create(&mctx->client, cfg->params.mqtt.broker, cfg->params.mqtt.client_id,
                                 MQTTCLIENT_PERSISTENCE_NONE, NULL);
            if (rc != MQTTASYNC_SUCCESS) {
                free(mctx);
                free(impl);
                return NULL;
            }
            MQTTAsync_setCallbacks(mctx->client, mctx, NULL, _on_mqtt_msg_arrived, NULL);
            snprintf(mctx->current_topic, sizeof(mctx->current_topic), "%.*s",
                     (int)(sizeof(mctx->current_topic) - 1), cfg->params.mqtt.topic);
            impl->handle.poll.data = mctx;
            impl->base.ops = &mqtt_ops;
            break;
        }
        default:
            free(impl);
            return NULL;
    }
    return &impl->base;
}

io_handle_t* io_executor_create_from_fd(uv_loop_t* loop, int fd, net_buf_pool_t* pool) {
    if (fd < 0) return NULL;

    linux_io_handle_t* impl = calloc(1, sizeof(linux_io_handle_t));
    if (!impl) return NULL;

    impl->fd = fd;
    impl->pool = pool;
    impl->type = IO_TYPE_CUSTOM;
    impl->base.ops = &poll_ops;

    int rc = uv_poll_init(loop, &impl->handle.poll, fd);

    if (rc < 0) {
        LOG_ERROR(LOG_CAT_IO, "io_executor: Failed to init poll handle for FD %d: %s (%d)",

                  fd, uv_strerror(rc), rc);

        free(impl);

        return NULL;
    }
    impl->handle.poll.data = impl;

    return &impl->base;
}

io_handle_t* io_executor_create_uart(uv_loop_t* loop, const char* dev_path, uint32_t baud,

                                     net_buf_pool_t* pool) {
    io_config_t cfg = {.type = IO_TYPE_UART};

    strncpy(cfg.params.uart.dev, dev_path, 63);
    cfg.params.uart.dev[sizeof(cfg.params.uart.dev) - 1] = '\0';

    cfg.params.uart.baud = baud;

    return io_executor_create(loop, &cfg, pool);
}

io_handle_t* io_executor_create_tcp(uv_loop_t* loop, const char* host, uint16_t port,
                                    net_buf_pool_t* pool) {
    if (!host) return NULL;

    io_config_t cfg = {.type = IO_TYPE_TCP};

    strncpy(cfg.params.tcp.host, host, sizeof(cfg.params.tcp.host) - 1);
    cfg.params.tcp.host[sizeof(cfg.params.tcp.host) - 1] = '\0';

    cfg.params.tcp.port = port;

    return io_executor_create(loop, &cfg, pool);
}

static void _on_http_simple_cb(io_handle_t* h, io_event_t evt, void* payload, void* user_data) {
    (void)payload;
    (void)user_data;
    if (evt == IO_EVENT_TX_DONE || evt == IO_EVENT_ERROR) h->ops->destroy(h);
}

void io_executor_http_request(uv_loop_t* loop, const char* url, const char* method) {
    io_config_t cfg = {.type = IO_TYPE_HTTP};
    strncpy(cfg.params.http.url, url, 255);
    cfg.params.http.url[sizeof(cfg.params.http.url) - 1] = '\0';
    strncpy(cfg.params.http.method, method, 15);
    cfg.params.http.method[sizeof(cfg.params.http.method) - 1] = '\0';
    io_handle_t* h = io_executor_create(loop, &cfg, NULL);
    if (h) {
        h->cb = _on_http_simple_cb;
        h->ops->enable_read(h, true);
    }
}

/* --- 6. 异步 TCP 连接端口 --- */

struct io_tcp_connect_s {
    uv_poll_t poll;
    int fd;
    int connect_err; /* 0 表示按 SO_ERROR 判定 */
    uint32_t idle_s;
    uint32_t intvl_s;
    uint32_t cnt;
    io_tcp_connect_cb_t cb;
    void* user_data;
};

static void _on_connect_close(uv_handle_t* handle) {
    free(handle->data);
}

void io_tcp_connect_cancel(io_tcp_connect_t* op) {
    if (!op) return;
    int fd = op->fd;
    op->fd = -1;
    op->cb = NULL;
    if (fd >= 0) close(fd);
    if (!uv_is_closing((uv_handle_t*)&op->poll)) {
        uv_poll_stop(&op->poll);
        uv_close((uv_handle_t*)&op->poll, _on_connect_close);
    }
}

static void _on_connect_poll(uv_poll_t* handle, int status, int events) {
    io_tcp_connect_t* op = (io_tcp_connect_t*)handle->data;
    if (!op || !op->cb) return;

    int fd = op->fd;
    int err = op->connect_err;
    if (err == 0) {
        socklen_t len = sizeof(err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0) err = errno;
    }
    if (err == 0 && status != 0) err = -status; /* libuv 负数状态按 errno 上报 */
    if (err == 0 && (events & UV_DISCONNECT)) err = ECONNABORTED;

    io_tcp_connect_cb_t cb = op->cb;
    void* user_data = op->user_data;
    op->fd = -1; /* 已转移或即将关闭，cancel 不得再碰 */
    op->cb = NULL;
    /* 先摘掉连接 poll 再回调：调用方会对同一 fd 做 uv_poll_init。 */
    uv_poll_stop(&op->poll);
    uv_close((uv_handle_t*)&op->poll, _on_connect_close);
    if (err != 0) {
        close(fd);
        fd = -1;
    } else {
        _tcp_apply_keepalive(fd, op->idle_s, op->intvl_s, op->cnt);
    }
    cb(user_data, fd, err != 0 ? -err : 0);
}

io_tcp_connect_t* io_executor_tcp_connect_async(uv_loop_t* loop, const io_tcp_connect_opts_t* opts,
                                                io_tcp_connect_cb_t cb, void* user_data) {
    if (!loop || !opts || !opts->host || !cb || opts->port == 0) {
        errno = EINVAL;
        return NULL;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(opts->port);
    if (inet_pton(AF_INET, opts->host, &addr.sin_addr) != 1) {
        errno = EINVAL;
        return NULL;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return NULL;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        int err = errno;
        close(fd);
        errno = err;
        return NULL;
    }

    io_tcp_connect_t* op = calloc(1, sizeof(*op));
    if (!op) {
        int err = errno;
        close(fd);
        errno = err;
        return NULL;
    }
    op->fd = fd;
    op->cb = cb;
    op->user_data = user_data;
    op->idle_s = opts->keepalive_idle_s;
    op->intvl_s = opts->keepalive_intvl_s;
    op->cnt = opts->keepalive_cnt;
    op->poll.data = op;

    int rc = uv_poll_init(loop, &op->poll, fd);
    if (rc < 0) {
        free(op);
        close(fd);
        errno = -rc;
        return NULL;
    }
    /* 立即成功或失败都先武装 poll：完成回调始终异步，拒绝也经 cb 上报。 */
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        if (errno != EINPROGRESS && errno != EINTR) op->connect_err = errno;
    }
    rc = uv_poll_start(&op->poll, UV_WRITABLE, _on_connect_poll);
    if (rc < 0) {
        int err = -rc;
        int poll_fd = op->fd;
        op->fd = -1;
        op->cb = NULL;
        uv_close((uv_handle_t*)&op->poll, _on_connect_close);
        close(poll_fd);
        errno = err;
        return NULL;
    }
    return op;
}