#include "north_protocol.h"

#include <stdio.h>
#include <string.h>

#include "edge_log.h"
#include "north_driver_port.h"

/* ==============================================================================
 * 北向协议注册表（ADR-0011：换协议不改编排层）
 *
 * 加一个新协议 = 在这里加一行 + 写一个驱动。管理器只调用 attach。
 * ========================================================================== */
typedef struct {
    const char* name;
    /* 该协议缺省的地址模板。换协议时地址规则必须跟着换 ——
     * Sparkplug 的主题里要 kind，JSON 的 topic 树完全不同。 */
    const char* default_address;
} nb_protocol_entry_t;

static const nb_protocol_entry_t NB_PROTOCOLS[] = {
    {"SparkplugB", "spBv1.0/{group}/{kind}/{node}"},
    /* 第二个协议是**对照样本**：地址规则、载荷格式、BIRTH 语义都与 Sparkplug 不同，
     * 用来证明换协议不需要改编排层。只有一个协议时"可插拔"没法验证。 */
    {"MqttJson", "edge/{node}/{kind}"},
};

static const nb_protocol_entry_t* _find_protocol(const char* name) {
    if (!name) return NULL;
    for (size_t i = 0; i < sizeof(NB_PROTOCOLS) / sizeof(NB_PROTOCOLS[0]); i++) {
        if (strcmp(NB_PROTOCOLS[i].name, name) == 0) return &NB_PROTOCOLS[i];
    }
    return NULL;
}

int north_protocol_attach(northbound_mgr_t* mgr) {
    const nb_protocol_entry_t* p = _find_protocol(mgr ? mgr->protocol_name : NULL);
    if (!p) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: unknown protocol '%s'",
                  mgr ? mgr->protocol_name : "");
        return -1;
    }
    if (!mgr->driver_port.attach) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: driver port is not injected");
        return -1;
    }
    if (mgr->driver_port.attach(mgr->driver_port.ctx, p->name, &mgr->nb_driver) != 0) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: driver port cannot create '%s'", p->name);
        mgr->nb_driver = NULL;
        return -1;
    }
    return 0;
}

int northbound_mgr_set_protocol(northbound_mgr_t* mgr, const char* protocol_name) {
    if (!mgr || !protocol_name || !protocol_name[0]) return -1;
    if (!_find_protocol(protocol_name)) {
        LOG_ERROR(LOG_CAT_NB, "Northbound: unknown protocol '%s'", protocol_name);
        return -1;
    }
    snprintf(mgr->protocol_name, sizeof(mgr->protocol_name), "%s", protocol_name);

    /* 换协议就把地址模板一起换掉（除非调用方显式配过）。
     * 不换的话：选了 JSON 驱动、却还在往 spBv1.0/... 上发 —— 主题和载荷对不上，
     * 主站两边都解析不了，而日志里一切正常。 */
    if (!mgr->address_explicit) {
        const nb_protocol_entry_t* p = _find_protocol(protocol_name);
        if (p && p->default_address) {
            snprintf(mgr->address_template, sizeof(mgr->address_template), "%s",
                     p->default_address);
        }
    }
    return 0;
}
