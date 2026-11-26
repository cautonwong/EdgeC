#include "iems_tcp_link.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bus_orchestrator.h"
#include "edge_log.h"
#include "iems_southbound_adapter.h"

#define IEMS_TCP_LINK_BASE_MS 1000u
#define IEMS_TCP_LINK_CAP_MS 30000u
#define IEMS_TCP_LINK_MAX_SHIFT 5u
#define IEMS_TCP_LINK_HOST_MAX 64
#define IEMS_TCP_LINK_CONNECT_TIMEOUT_MS 3000u
#define IEMS_TCP_LINK_LOG_EVERY 10u

struct iems_tcp_link_s {
    uv_timer_t timer;
    uv_loop_t *loop;
    bus_orchestrator_t *orch;
    io_handle_t *io;
    iems_tcp_link_config_t cfg;
    iems_tcp_link_state_t state;
    unsigned attempt;
    int bus_id;
    uint16_t port;
    bool stopped;
    char host[IEMS_TCP_LINK_HOST_MAX];
    io_tcp_connect_t *connect_op;
};

_Static_assert(sizeof(struct iems_tcp_link_s) <= sizeof(iems_tcp_link_storage_t),
               "iems_tcp_link_s size exceeds iems_tcp_link_storage_t");

static void _attempt(iems_tcp_link_t *link);
static void _on_timer(uv_timer_t *timer);
static void _on_link_event(io_handle_t *h, io_event_t evt, void *user_data);

static void _set_state(iems_tcp_link_t *link, iems_tcp_link_state_t state) {
    if (link->state == state) return;
    link->state = state;
    if (link->cfg.on_state) link->cfg.on_state(state, link->cfg.user_data);
}

/* 退避窗口：首次 0.5..1s，第 6 次起 15..30s。attempt 只在进入 CONNECTED 时清零，
 * 这里用「当前失败次数」决定位移，再自增给日志与下一次。重复失败只保留首次与
 * 每 10 次一条摘要，避免长时间断线刷屏。 */
static void _enter_backoff(iems_tcp_link_t *link, const char *why) {
    unsigned shift =
        link->attempt < IEMS_TCP_LINK_MAX_SHIFT ? link->attempt : IEMS_TCP_LINK_MAX_SHIFT;
    uint64_t ceiling = (uint64_t)link->cfg.backoff_base_ms << shift;
    if (ceiling > link->cfg.backoff_cap_ms) ceiling = link->cfg.backoff_cap_ms;
    uint32_t half = (uint32_t)(ceiling / 2u);
    uint32_t delay = half + ((uint32_t)rand() % (half + 1u));
    link->attempt++;
    if (link->attempt == 1 || link->attempt % IEMS_TCP_LINK_LOG_EVERY == 0) {
        LOG_WARN(LOG_CAT_CORE, "TCP link bus %d: %s, retry #%u in %u ms (%s:%u)", link->bus_id, why,
                 link->attempt, delay, link->host, (unsigned)link->port);
    }
    _set_state(link, IEMS_TCP_LINK_BACKOFF);
    if (link->stopped) return;
    if (uv_timer_start(&link->timer, _on_timer, delay, 0) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "TCP link bus %d: failed to arm retry timer for %u ms",
                  link->bus_id, delay);
    }
}

static void _abort_connect(iems_tcp_link_t *link) {
    io_tcp_connect_cancel(link->connect_op);
    link->connect_op = NULL;
}

static void _on_connected(iems_tcp_link_t *link, int fd) {
    io_handle_t *h = link->cfg.adopt_fd(link->loop, fd, link->cfg.io_pool);
    if (!h) {
        close(fd);
        _enter_backoff(link, "connect failed: adopt");
        return;
    }
    /* 先挂新句柄再销毁旧句柄：BACKOFF 期间引擎仍指向旧句柄，反过来会悬垂 */
    if (bus_orchestrator_attach_io(link->orch, link->bus_id, h) != 0) {
        io_handle_destroy(h);
        _enter_backoff(link, "attach failed");
        return;
    }
    io_handle_t *old = link->io;
    link->io = h;
    io_handle_set_link_cb(h, _on_link_event, link);
    io_handle_enable_read(h, true);
    link->attempt = 0;
    uv_timer_stop(&link->timer);
    _set_state(link, IEMS_TCP_LINK_CONNECTED);
    printf("\033[32m[PHYSICS-SUCCESS] Bus %d CONNECTED via TCP: %s:%u\033[0m\n", link->bus_id,
           link->host, (unsigned)link->port);
    if (old && old != h) io_handle_destroy(old);
}

static void _on_connect_done(void *user_data, int fd, int err) {
    iems_tcp_link_t *link = (iems_tcp_link_t *)user_data;
    link->connect_op = NULL;
    if (link->stopped || link->state != IEMS_TCP_LINK_CONNECTING) {
        if (fd >= 0) close(fd);
        return;
    }
    if (err != 0 || fd < 0) {
        int e = err != 0 ? -err : EIO;
        if (fd >= 0) close(fd); /* 适配器违约时仍要关掉 fd，否则泄漏 */
        char why[96];
        snprintf(why, sizeof(why), "connect failed: %s (%d)", strerror(e), e);
        _enter_backoff(link, why);
        return;
    }
    _on_connected(link, fd);
}

static void _on_timer(uv_timer_t *timer) {
    iems_tcp_link_t *link = (iems_tcp_link_t *)timer->data;
    if (!link || link->stopped) return;
    if (link->state == IEMS_TCP_LINK_CONNECTING) {
        _abort_connect(link);
        _enter_backoff(link, "connect timeout");
        return;
    }
    if (link->state == IEMS_TCP_LINK_BACKOFF) _attempt(link);
}

static void _attempt(iems_tcp_link_t *link) {
    if (link->stopped) return;
    _set_state(link, IEMS_TCP_LINK_CONNECTING);
    if (link->stopped) return;

    io_tcp_connect_opts_t opts = {
        .host = link->host,
        .port = link->port,
        .keepalive_idle_s = link->cfg.keepalive_idle_s,
        .keepalive_intvl_s = link->cfg.keepalive_intvl_s,
        .keepalive_cnt = link->cfg.keepalive_cnt,
    };
    link->connect_op = io_executor_tcp_connect_async(link->loop, &opts, _on_connect_done, link);
    if (!link->connect_op) {
        int e = errno;
        char why[96];
        snprintf(why, sizeof(why), "connect failed: %s (%d)", strerror(e), e);
        _enter_backoff(link, why);
        return;
    }
    if (uv_timer_start(&link->timer, _on_timer, link->cfg.connect_timeout_ms, 0) != 0) {
        _abort_connect(link);
        _enter_backoff(link, "connect failed");
    }
}

static void _on_link_event(io_handle_t *h, io_event_t evt, void *user_data) {
    iems_tcp_link_t *link = (iems_tcp_link_t *)user_data;
    if (!link || link->stopped) return;
    if (evt != IO_EVENT_ERROR) return;
    /* 重复错误是常态（EOF 会连报）；状态判等放最前，过期句柄的事件直接丢弃 */
    if (link->state != IEMS_TCP_LINK_CONNECTED || link->io != h) return;
    io_handle_enable_read(h, false);
    _enter_backoff(link, "connection lost");
}

/* link 的状态投影到 bus_state：BACKOFF 与 DISCONNECTED 对外都是「没连上」，
 * 重试进度只进日志。 */
static void _project_bus_state(iems_tcp_link_state_t state, void *user_data) {
    bus_conn_state_t *bus_state = (bus_conn_state_t *)user_data;
    if (!bus_state) return;
    switch (state) {
        case IEMS_TCP_LINK_CONNECTED:
            *bus_state = BUS_STATE_CONNECTED;
            break;
        case IEMS_TCP_LINK_CONNECTING:
            *bus_state = BUS_STATE_CONNECTING;
            break;
        default:
            *bus_state = BUS_STATE_DISCONNECTED;
            break;
    }
}

bool iems_tcp_link_start_bus(uv_loop_t *loop, bus_orchestrator_t *orch, int bus_id,
                             const char *label, const char *path, bus_conn_state_t *bus_state,
                             iems_tcp_link_slot_t *slot, const iems_tcp_link_config_t *cfg,
                             iems_tcp_link_open_fn open_io, iems_tcp_link_adopt_fn adopt_fd,
                             net_buf_pool_t *io_pool) {
    if (!loop || !orch || !path) return false;
    char host[IEMS_TCP_LINK_HOST_MAX];
    unsigned port = 0;
    if (sscanf(path, "tcp://%63[^:]:%u", host, &port) != 2 || port == 0 || port > 65535) {
        LOG_WARN(LOG_CAT_CORE, "Startup: southbound bus %d (%s) has invalid TCP address %s", bus_id,
                 label ? label : "?", path);
        return false;
    }
    struct in_addr addr4;
    if (inet_pton(AF_INET, host, &addr4) != 1) {
        LOG_WARN(LOG_CAT_CORE,
                 "Startup: southbound bus %d (%s) TCP host %s is not a numeric IPv4 address",
                 bus_id, label ? label : "?", host);
        return false;
    }

    if (!slot) {
        /* 旧入口：只连一次，没有重连生命周期 */
        if (!open_io) return false;
        io_handle_t *h = open_io(loop, host, (uint16_t)port, io_pool);
        if (!h) return false;
        if (bus_orchestrator_attach_io(orch, bus_id, h) != 0) {
            io_handle_destroy(h);
            return false;
        }
        io_handle_enable_read(h, true);
        if (bus_state) *bus_state = BUS_STATE_CONNECTED;
        printf("\033[32m[PHYSICS-SUCCESS] Bus %d CONNECTED via TCP: %s:%u\033[0m\n", bus_id, host,
               port);
        return true;
    }

    iems_tcp_link_config_t local = {0};
    if (cfg) local = *cfg;
    local.on_state = _project_bus_state;
    local.user_data = bus_state;
    local.adopt_fd = adopt_fd;
    local.io_pool = io_pool;
    slot->link =
        iems_tcp_link_create_in(&slot->storage, loop, orch, bus_id, host, (uint16_t)port, &local);
    return slot->link != NULL;
}

void iems_tcp_link_close_all(iems_tcp_link_slot_t *slots, int count) {
    if (!slots) return;
    for (int i = 0; i < count; i++) iems_tcp_link_close(slots[i].link);
}

iems_tcp_link_t *iems_tcp_link_create_in(iems_tcp_link_storage_t *storage, uv_loop_t *loop,
                                         bus_orchestrator_t *orch, int bus_id, const char *host,
                                         uint16_t port, const iems_tcp_link_config_t *cfg) {
    if (!storage || !loop || !orch || !host || !host[0] || port == 0) return NULL;
    if (!cfg || !cfg->adopt_fd) return NULL;

    iems_tcp_link_t *link = (iems_tcp_link_t *)storage->raw;
    memset(link, 0, sizeof(*link));
    link->loop = loop;
    link->orch = orch;
    link->bus_id = bus_id;
    link->port = port;
    snprintf(link->host, sizeof(link->host), "%s", host);
    if (cfg) link->cfg = *cfg;
    if (link->cfg.backoff_base_ms == 0) link->cfg.backoff_base_ms = IEMS_TCP_LINK_BASE_MS;
    if (link->cfg.backoff_cap_ms == 0) link->cfg.backoff_cap_ms = IEMS_TCP_LINK_CAP_MS;
    if (link->cfg.connect_timeout_ms == 0) {
        link->cfg.connect_timeout_ms = IEMS_TCP_LINK_CONNECT_TIMEOUT_MS;
    }
    if (link->cfg.keepalive_idle_s == 0) link->cfg.keepalive_idle_s = IO_TCP_KEEPALIVE_IDLE_S;
    if (link->cfg.keepalive_intvl_s == 0) link->cfg.keepalive_intvl_s = IO_TCP_KEEPALIVE_INTVL_S;
    if (link->cfg.keepalive_cnt == 0) link->cfg.keepalive_cnt = IO_TCP_KEEPALIVE_CNT;
    if (uv_timer_init(loop, &link->timer) != 0) return NULL;
    link->timer.data = link;
    _attempt(link);
    return link;
}

void iems_tcp_link_close(iems_tcp_link_t *link) {
    if (!link || link->stopped) return;
    link->stopped = true;
    uv_timer_stop(&link->timer);
    uv_close((uv_handle_t *)&link->timer, NULL);
    _abort_connect(link);
    if (link->io) {
        io_handle_enable_read(link->io, false);
        io_handle_destroy(link->io);
        link->io = NULL;
    }
    link->state = IEMS_TCP_LINK_DISCONNECTED;
    if (link->cfg.on_state) link->cfg.on_state(IEMS_TCP_LINK_DISCONNECTED, link->cfg.user_data);
}

iems_tcp_link_state_t iems_tcp_link_state(const iems_tcp_link_t *link) {
    return link ? link->state : IEMS_TCP_LINK_DISCONNECTED;
}
