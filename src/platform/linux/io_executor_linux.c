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
    (void)handle;
    int running_handles;
    curl_multi_socket_action(g_curl_ctx->multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
}

static int _curl_timer_cb(CURLM *multi, long timeout_ms, void *userp) {
    (void)multi;
    curl_multi_ctx_t *ctx = (curl_multi_ctx_t *)userp;
    if (timeout_ms < 0) {
        uv_timer_stop(&ctx->timer);
    } else {
        if (timeout_ms == 0) timeout_ms = 1;
        uv_timer_start(&ctx->timer, _on_curl_timeout, (uint64_t)timeout_ms, 0);
    }
    return 0;
}

static void _on_curl_poll(uv_poll_t *handle, int status, int events) {
    (void)status;
    int running_handles;
    int flags = 0;
    curl_poll_ctx_t *poll_ctx = (curl_poll_ctx_t *)handle->data;

    if (events & UV_READABLE) flags |= CURL_CSELECT_IN;
    if (events & UV_WRITABLE) flags |= CURL_CSELECT_OUT;

    curl_multi_socket_action(g_curl_ctx->multi, poll_ctx->sockfd, flags, &running_handles);

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
                curl_multi_remove_handle(g_curl_ctx->multi, easy);
            }
        }
    }
}

static int _curl_socket_cb(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp) {
    (void)easy;
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
    g_curl_ctx->timer.data = g_curl_ctx;
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_SOCKETFUNCTION, _curl_socket_cb);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_SOCKETDATA, g_curl_ctx);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_TIMERFUNCTION, _curl_timer_cb);
    curl_multi_setopt(g_curl_ctx->multi, CURLMOPT_TIMERDATA, g_curl_ctx);
}

static void _on_curl_ctx_close(uv_handle_t *handle) {
    free(handle->data);
}

void io_executor_cleanup(void) {
    if (g_curl_ctx) {
        uv_timer_stop(&g_curl_ctx->timer);
        curl_multi_cleanup(g_curl_ctx->multi);
        g_curl_ctx->multi = NULL;
        uv_close((uv_handle_t *)&g_curl_ctx->timer, _on_curl_ctx_close);
        g_curl_ctx = NULL;
    }
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
                impl->base.cb(&impl->base, IO_EVENT_RX, rx, impl->base.user_data);
                net_buf_unref(rx);
            } else {
                net_buf_unref(rx);
                if (n == 0 || (errno != EAGAIN && errno != EINTR)) {
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
                fprintf(stderr, "[IO-TX] FD:%d Sent %zd bytes SUCCESS\n", impl->fd, n);
                net_buf_t *buf = impl->pending_tx;
                impl->pending_tx = NULL;
                // 恢复只读监听
                uv_poll_start(handle, UV_READABLE, _on_uv_poll);
                impl->base.cb(&impl->base, IO_EVENT_TX_DONE, NULL, impl->base.user_data);
                net_buf_unref(buf);
            } else if (n < 0 && errno != EAGAIN && errno != EINTR) {
                fprintf(stderr, "[IO-TX-ERR] FD:%d Write Failed: %s\n", impl->fd, strerror(errno));
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
    if (cmd == IO_CTL_FLUSH) {
        if (impl->pending_tx) {
            fprintf(stderr, "[IO] Forced FLUSH: Releasing pending TX buf\n");
            net_buf_unref(impl->pending_tx);
            impl->pending_tx = NULL;
        }
        return 0;
    }
    if (cmd == IO_CTL_SET_BAUD && impl->type == IO_TYPE_UART) {
        uint32_t baud = *(uint32_t *)arg;
        struct termios options;
        if (tcgetattr(impl->fd, &options) != 0) return -errno;
        speed_t speed = B9600;
        switch (baud) {
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
        return 0;
    }
    return -1;
}

/* --- 2. 基于 UDP 的处理 --- */

static void _on_udp_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    (void)suggested_size;
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
    (void)buf;
    (void)addr;
    (void)flags;
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
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
    (void)status;
    linux_io_handle_t *impl = (linux_io_handle_t *)req->data;
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
    if (!impl->http_rx_nb) impl->http_rx_nb = net_buf_alloc(impl->pool, 0);
    if (impl->http_rx_nb && net_buf_simple_tailroom(&impl->http_rx_nb->b) >= total) {
        net_buf_simple_add_mem(&impl->http_rx_nb->b, ptr, total);
        impl->base.cb(&impl->base, IO_EVENT_RX, impl->http_rx_nb, impl->base.user_data);
    }
    return total;
}

static int _http_enable_read(io_handle_t *h, bool enable) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (enable)
        curl_multi_add_handle(g_curl_ctx->multi, impl->handle.curl);
    else
        curl_multi_remove_handle(g_curl_ctx->multi, impl->handle.curl);
    return 0;
}

static int _http_write_async(io_handle_t *h, net_buf_t *buf) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    if (buf && buf->b.len > 0) {
        curl_easy_setopt(impl->handle.curl, CURLOPT_POSTFIELDS, buf->b.data);
        curl_easy_setopt(impl->handle.curl, CURLOPT_POSTFIELDSIZE, (long)buf->b.len);
    }
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
    if (g_curl_ctx && g_curl_ctx->multi) {
        curl_multi_remove_handle(g_curl_ctx->multi, impl->handle.curl);
    }
    curl_easy_cleanup(impl->handle.curl);
    if (impl->http_hdrs) curl_slist_free_all(impl->http_hdrs);
    if (impl->http_rx_nb) net_buf_unref(impl->http_rx_nb);
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
    linux_io_handle_t *impl;
} mqtt_ctx_t;

static void _on_mqtt_connect_success(void *context, MQTTAsync_successData *response) {
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)context;
    ctx->connected = true;
    LOG_INFO(LOG_CAT_CORE, "MQTT Connected to Broker: %s", (char *)response->alt.connect.serverURI);
    ctx->impl->base.cb(&ctx->impl->base, IO_EVENT_TX_DONE, NULL, ctx->impl->base.user_data);
}

static void _on_mqtt_connect_failure(void *context, MQTTAsync_failureData *response) {
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)context;
    LOG_ERROR(LOG_CAT_CORE, "MQTT Connection Failed, code: %d", response ? response->code : -1);
    ctx->impl->base.cb(&ctx->impl->base, IO_EVENT_ERROR, NULL, ctx->impl->base.user_data);
}

static int _on_mqtt_msg_arrived(void *context, char *topicName, int topicLen,
                                MQTTAsync_message *message) {
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)context;
    net_buf_t *nb = net_buf_alloc(ctx->impl->pool, 0);
    if (nb) {
        net_buf_append_bytes(nb, (uint8_t *)message->payload, message->payloadlen);
        ctx->impl->base.cb(&ctx->impl->base, IO_EVENT_RX, nb, ctx->impl->base.user_data);
        net_buf_unref(nb);
    }
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

static void _on_mqtt_poll(uv_poll_t *handle, int status, int events) {
    (void)events;
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
    if (status < 0) {
        impl->base.cb(&impl->base, IO_EVENT_ERROR, NULL, impl->base.user_data);
    }
}

static int _mqtt_enable_read(io_handle_t *h, bool enable) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)impl->handle.poll.data;
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

static int _mqtt_write_async(io_handle_t *h, net_buf_t *buf) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)impl->handle.poll.data;
    if (!ctx->connected) return -ENOTCONN;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.context = ctx;
    int rc = MQTTAsync_send(ctx->client, ctx->current_topic, buf->b.len, buf->b.data, 1, 0, &opts);
    return (rc == MQTTASYNC_SUCCESS) ? 0 : -1;
}

static int _mqtt_ioctl(io_handle_t *h, int cmd, void *arg) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)impl->handle.poll.data;
    switch (cmd) {
        case IO_CTL_MQTT_SET_TOPIC:
            strncpy(ctx->current_topic, (const char *)arg, 127);
            return 0;
        case IO_CTL_MQTT_SUBSCRIBE:
            return MQTTAsync_subscribe(ctx->client, (const char *)arg, 1, NULL);
        default:
            return -1;
    }
}

static void _mqtt_destroy(io_handle_t *h) {
    linux_io_handle_t *impl = (linux_io_handle_t *)h;
    mqtt_ctx_t *ctx = (mqtt_ctx_t *)impl->handle.poll.data;
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

static void _on_handle_close(uv_handle_t *handle) {
    linux_io_handle_t *impl = (linux_io_handle_t *)handle->data;
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
    if (impl->type == IO_TYPE_MQTT) {
        _mqtt_destroy(h);
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
            impl->handle.curl = curl_easy_init();
            if (!impl->handle.curl) {
                free(impl);
                return NULL;
            }
            curl_easy_setopt(impl->handle.curl, CURLOPT_URL, cfg->params.http.url);
            curl_easy_setopt(impl->handle.curl, CURLOPT_WRITEFUNCTION, _on_curl_write);
            curl_easy_setopt(impl->handle.curl, CURLOPT_WRITEDATA, impl);
            curl_easy_setopt(impl->handle.curl, CURLOPT_PRIVATE, impl);
            impl->base.ops = &http_ops;
            break;
        }
        case IO_TYPE_MQTT: {
            mqtt_ctx_t *mctx = calloc(1, sizeof(mqtt_ctx_t));
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
            strncpy(mctx->current_topic, cfg->params.mqtt.topic, 127);
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

io_handle_t *io_executor_create_from_fd(uv_loop_t *loop, int fd, net_buf_pool_t *pool) {
    if (fd < 0) return NULL;

    linux_io_handle_t *impl = calloc(1, sizeof(linux_io_handle_t));
    if (!impl) return NULL;

    impl->fd = fd;
    impl->pool = pool;
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

io_handle_t *io_executor_create_uart(uv_loop_t *loop, const char *dev_path, uint32_t baud,

                                     net_buf_pool_t *pool) {
    io_config_t cfg = {.type = IO_TYPE_UART};

    strncpy(cfg.params.uart.dev, dev_path, 63);

    cfg.params.uart.baud = baud;

    return io_executor_create(loop, &cfg, pool);
}

static void _on_http_simple_cb(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)payload;
    (void)user_data;
    if (evt == IO_EVENT_TX_DONE || evt == IO_EVENT_ERROR) h->ops->destroy(h);
}

void io_executor_http_request(uv_loop_t *loop, const char *url, const char *method) {
    io_config_t cfg = {.type = IO_TYPE_HTTP};
    strncpy(cfg.params.http.url, url, 255);
    strncpy(cfg.params.http.method, method, 15);
    io_handle_t *h = io_executor_create(loop, &cfg, NULL);
    if (h) {
        h->cb = _on_http_simple_cb;
        h->ops->enable_read(h, true);
    }
}