#include "iems_tcp_link.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus_orchestrator.h"
#include "edge_log.h"
#include "iems_southbound_adapter.h"

#define IEMS_TCP_LINK_BASE_MS 1000u
#define IEMS_TCP_LINK_CAP_MS 30000u
#define IEMS_TCP_LINK_MAX_SHIFT 5u
#define IEMS_TCP_LINK_HOST_MAX 64

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
};

_Static_assert(sizeof(struct iems_tcp_link_s) <= sizeof(iems_tcp_link_storage_t),
               "iems_tcp_link_s size exceeds iems_tcp_link_storage_t");

static void _attempt(iems_tcp_link_t *link);
static void _on_retry(uv_timer_t *timer);

static void _set_state(iems_tcp_link_t *link, iems_tcp_link_state_t state) {
    if (link->state == state) return;
    link->state = state;
    if (link->cfg.on_state) link->cfg.on_state(state, link->cfg.user_data);
}

/* 退避窗口：首次 0.5..1s，第 6 次起 15..30s。attempt 只在进入 CONNECTED 时清零，
 * 这里用「当前失败次数」决定位移，再自增给日志与下一次。 */
static void _enter_backoff(iems_tcp_link_t *link, const char *why) {
    unsigned shift = link->attempt < IEMS_TCP_LINK_MAX_SHIFT ? link->attempt
                                                             : IEMS_TCP_LINK_MAX_SHIFT;
    uint64_t ceiling = (uint64_t)link->cfg.backoff_base_ms << shift;
    if (ceiling > link->cfg.backoff_cap_ms) ceiling = link->cfg.backoff_cap_ms;
    uint32_t half = (uint32_t)(ceiling / 2u);
    uint32_t delay = half + ((uint32_t)rand() % (half + 1u));
    link->attempt++;
    LOG_WARN(LOG_CAT_CORE, "TCP link bus %d: %s, retry #%u in %u ms (%s:%u)", link->bus_id, why,
             link->attempt, delay, link->host, (unsigned)link->port);
    _set_state(link, IEMS_TCP_LINK_BACKOFF);
    uv_timer_start(&link->timer, _on_retry, delay, 0);
}

static void _on_retry(uv_timer_t *timer) {
    iems_tcp_link_t *link = (iems_tcp_link_t *)timer->data;
    if (!link || link->stopped) return;
    _attempt(link);
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

static void _attempt(iems_tcp_link_t *link) {
    if (link->stopped) return;
    _set_state(link, IEMS_TCP_LINK_CONNECTING);

    /* shortcut: connect 在 loop 线程内同步完成，单地址最长阻塞 1s；需要消除启动/退避期间
     * 的 loop 停顿时改为异步 connect（socket + EINPROGRESS + uv_poll WRITABLE + 独立超时定时器）。 */
    io_handle_t *h = link->cfg.open_io(link->loop, link->host, link->port, link->cfg.io_pool);
    if (!h) {
        _enter_backoff(link, "connect failed");
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
    _set_state(link, IEMS_TCP_LINK_CONNECTED);
    printf("\033[32m[PHYSICS-SUCCESS] Bus %d CONNECTED via TCP: %s:%u\033[0m\n", link->bus_id,
           link->host, (unsigned)link->port);
    if (old && old != h) io_handle_destroy(old);
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
                             iems_tcp_link_open_fn open_io, net_buf_pool_t *io_pool) {
    if (!loop || !orch || !path || !open_io) return false;
    char host[IEMS_TCP_LINK_HOST_MAX];
    unsigned port = 0;
    if (sscanf(path, "tcp://%63[^:]:%u", host, &port) != 2 || port == 0 || port > 65535) {
        LOG_WARN(LOG_CAT_CORE, "Startup: southbound bus %d (%s) has invalid TCP address %s", bus_id,
                 label ? label : "?", path);
        return false;
    }

    if (!slot) {
        /* 旧入口：只连一次，没有重连生命周期 */
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
    local.open_io = open_io;
    local.io_pool = io_pool;
    slot->link = iems_tcp_link_create_in(&slot->storage, loop, orch, bus_id, host, (uint16_t)port,
                                         &local);
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
    if (!cfg || !cfg->open_io) return NULL;

    iems_tcp_link_t *link = (iems_tcp_link_t *)storage->raw;
    memset(link, 0, sizeof(*link));
    link->loop = loop;
    link->orch = orch;
    link->bus_id = bus_id;
    link->port = port;
    snprintf(link->host, sizeof(link->host), "%s", host);
    link->cfg = *cfg;
    if (link->cfg.backoff_base_ms == 0) link->cfg.backoff_base_ms = IEMS_TCP_LINK_BASE_MS;
    if (link->cfg.backoff_cap_ms == 0) link->cfg.backoff_cap_ms = IEMS_TCP_LINK_CAP_MS;
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
