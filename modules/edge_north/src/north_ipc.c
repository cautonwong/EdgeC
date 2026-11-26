#include "northbound_mgr.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "edge_log.h"

/* MQTT 主题层级：'/' 拆层，'+' '#' 是通配；只接受可安全拼接的字符集。 */
bool north_ipc_topic_level_ok(const char *value) {
    if (!value || !value[0]) return false;
    for (const char *p = value; *p; p++) {
        char c = *p;
        bool legal = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                     (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!legal) return false;
    }
    return true;
}

static const char *const k_ipc_verb_names[] = {"Set", "Get", "SetResp", "GetResp"};

int north_ipc_render_topic(const char *sn, const north_ipc_endpoint_t *ep,
                           north_ipc_verb_t verb, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return -1;
    if ((unsigned)verb >= sizeof(k_ipc_verb_names) / sizeof(k_ipc_verb_names[0])) return -1;
    if (!ep || !north_ipc_topic_level_ok(sn) || !north_ipc_topic_level_ok(ep->module) ||
        !north_ipc_topic_level_ok(ep->dev_id) || !north_ipc_topic_level_ok(ep->type)) {
        return -1;
    }

    char topic[NORTH_IPC_TOPIC_MAX];
    int n = snprintf(topic, sizeof(topic), "ipc/%s/%s/%s/%s/%s/binaryFlow", sn, ep->module,
                     ep->dev_id, ep->type, k_ipc_verb_names[verb]);
    if (n < 0 || (size_t)n >= sizeof(topic) || (size_t)n >= out_sz) return -1;
    memcpy(out, topic, (size_t)n + 1);
    return 0;
}

/* 通配订阅会收进同 sn 下的所有主题（含本端自己发的 *Resp）；
 * 只有渲染后 strcmp 命中的 Set/Get 才交给产品。 */
void north_ipc_dispatch(void* user, const char* topic, const uint8_t* payload, size_t len) {
    northbound_mgr_t* mgr = (northbound_mgr_t*)user;
    if (!mgr || !topic) return;
    if (!mgr->ipc_sn[0] || !mgr->ipc_port.endpoint_count || !mgr->ipc_port.endpoint_at ||
        !mgr->ipc_port.on_frame) {
        return;
    }

    static const north_ipc_verb_t k_ipc_dispatch_verbs[] = {NORTH_IPC_SET, NORTH_IPC_GET};
    size_t count = mgr->ipc_port.endpoint_count(mgr->ipc_port.ctx);
    char rendered[NORTH_IPC_TOPIC_MAX];
    for (size_t i = 0; i < count; i++) {
        north_ipc_endpoint_t ep;
        if (mgr->ipc_port.endpoint_at(mgr->ipc_port.ctx, i, &ep) != 0) continue;
        for (size_t v = 0; v < sizeof(k_ipc_dispatch_verbs) / sizeof(k_ipc_dispatch_verbs[0]);
             v++) {
            north_ipc_verb_t verb = k_ipc_dispatch_verbs[v];
            if (north_ipc_render_topic(mgr->ipc_sn, &ep, verb, rendered, sizeof(rendered)) != 0) {
                continue;
            }
            if (strcmp(rendered, topic) != 0) continue;

            if (mgr->ipc_port.on_frame(mgr->ipc_port.ctx, &ep, verb, payload, len) < 0) {
                LOG_WARN(LOG_CAT_NB, "Northbound: IPC frame on '%s' rejected", topic);
            }
            return;
        }
    }
    LOG_DEBUG(LOG_CAT_NB, "Northbound: ignoring IPC topic '%s'", topic);
}

/* 失败不置位，下一次 poll 重试；重连后的重订归传输层。 */
void north_ipc_ensure_subscribed(northbound_mgr_t* mgr) {
    if (mgr->ipc_subscribed || !mgr->ipc_sn[0]) return;
    if (!mgr->ipc_port.endpoint_count || !mgr->ipc_port.endpoint_at ||
        !mgr->ipc_port.on_frame) {
        return;
    }
    if (mgr->ipc_port.endpoint_count(mgr->ipc_port.ctx) == 0) return;

    char topic[NORTH_IPC_TOPIC_MAX];
    int n = snprintf(topic, sizeof(topic), "ipc/%s/#", mgr->ipc_sn);
    if (n < 0 || (size_t)n >= sizeof(topic)) return;

    bool any = false;
    bool all_ok = true;
    for (uint32_t i = 0; i < mgr->transport_count; i++) {
        nb_transport_ops_t* t = &mgr->transports[i];
        if (!t->subscribe) continue;
        any = true;
        if (t->subscribe(t->ctx, topic, north_ipc_dispatch, mgr) != 0) all_ok = false;
    }

    if (any && all_ok) {
        mgr->ipc_subscribed = true;
        LOG_INFO(LOG_CAT_NB, "Northbound: IPC wildcard subscribed on '%s'", topic);
    }
}

void north_ipc_set_port(northbound_mgr_t* mgr, north_ipc_port_t port, const char* sn) {
    if (!mgr) return;

    bool valid = north_ipc_topic_level_ok(sn) && strlen(sn) < NORTH_IPC_SN_MAX &&
                 port.endpoint_count && port.endpoint_at && port.on_frame;
    if (!valid) {
        memset(&mgr->ipc_port, 0, sizeof(mgr->ipc_port));
        mgr->ipc_sn[0] = '\0';
        mgr->ipc_subscribed = false;
        LOG_WARN(LOG_CAT_NB, "Northbound: IPC port disabled (bad sn or port callbacks)");
        return;
    }

    mgr->ipc_port = port;
    snprintf(mgr->ipc_sn, sizeof(mgr->ipc_sn), "%s", sn);
    mgr->ipc_subscribed = false;
    LOG_INFO(LOG_CAT_NB, "Northbound: IPC port attached (sn=%s)", mgr->ipc_sn);
}

int north_ipc_publish(northbound_mgr_t* mgr, const north_ipc_endpoint_t* ep,
                      north_ipc_verb_t verb, const uint8_t* payload, size_t len) {
    if (!mgr || !ep) return -1;
    if (verb != NORTH_IPC_SET_RESP && verb != NORTH_IPC_GET_RESP) return -1;

    char topic[NORTH_IPC_TOPIC_MAX];
    if (north_ipc_render_topic(mgr->ipc_sn, ep, verb, topic, sizeof(topic)) != 0) return -1;
    return northbound_mgr_publish_all(mgr, topic, payload, len);
}
