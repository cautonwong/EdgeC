#include "north_source_datahub_adapter.h"
#include "datahub.h"

/* ------------------------------------------------------------------------------
 * datahub 的"有新数据"通知适配
 *
 * `datahub_subscribe(hub, cb, user_data)` 只回传一个 `user_data`，而我们的
 * `subscribe_notify` 要 (函数, 上下文) 两个。用一对静态变量桥接。
 *
 * **一个进程只有一个 northbound_mgr**（DCU/IEMS/SmartBox 各自是独立进程），
 * 所以静态变量是安全的。如果将来同进程要跑多个，这里得换成按 hub 索引的表。
 * -------------------------------------------------------------------------- */
static struct {
    void (*fn)(void*);
    void* user;
} g_hub_notify;

static void _hub_on_data(datahub_t* hub, void* user_data) {
    (void)hub;
    (void)user_data;
    if (g_hub_notify.fn) g_hub_notify.fn(g_hub_notify.user);
}

static int _hub_subscribe_notify(void* ctx, void (*on_data)(void* user), void* user) {
    if (!ctx || !on_data) return -1;
    g_hub_notify.fn = on_data;
    g_hub_notify.user = user;
    datahub_subscribe((datahub_t*)ctx, _hub_on_data, NULL);
    return 0;
}

nb_source_ops_t northbound_source_from_datahub(datahub_t* hub) {
    nb_source_ops_t src = {
        .ctx = hub,
        .read_next = (int (*)(void*, uint64_t*, pipe_msg_t*))datahub_read_ring,
        .subscribe_notify = _hub_subscribe_notify,
    };
    return src;
}

