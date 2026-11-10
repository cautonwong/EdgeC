#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "edge_log.h"
#include "edge_north_plane.h"
#include "mqtt_json_codec.h"
#include "mqtt_producer.h"
#include "northbound_mgr.h"
#include "north_source_datahub_adapter.h"
#include "smartbox_driver_adapter.h"
#include "smartbox_north_adapter.h"

static northbound_mgr_t g_north;

/* =============================================================================
 * 北向取数端口（ADR-0011 族 A / N6-5）
 *
 * SmartBox 之前**完全没有北向**：数据只在本地影子表里打转，云端看不到。
 * 这里补上最小可用的一条，分工与 DCU/IEMS 一致：
 *   值从 datahub 取、名字由产品给，驱动不认识点位模型。
 * ========================================================================== */
static int _sb_nb_peek(void *ctx, uint32_t tag_id, pipe_msg_t *out) {
    datahub_t *hub = (datahub_t *)ctx;
    if (!hub || !out) return -1;
    return datahub_read_shadow(hub, tag_id, out);
}

static const char *_sb_nb_name(void *ctx, uint32_t tag_id) {
    (void)ctx;
    const edge_tag_meta_t *meta = smartbox_get_tag_meta(tag_id);
    return meta ? meta->name : NULL;
}

/** metric 名 → tag_id（下行要用：云端只给名字）。 */
static int _sb_nb_resolve_name(void *ctx, const char *name, uint32_t *tag_id) {
    (void)ctx;
    if (!name || !tag_id) return -1;
    uint32_t n = 0;
    const edge_tag_meta_t *all = smartbox_get_tag_metadata(&n);
    if (!all) return -1;
    for (uint32_t i = 0; i < n; i++) {
        if (all[i].name && strcmp(all[i].name, name) == 0) {
            *tag_id = (uint32_t)all[i].id;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief 收一条北向下行报文。
 *
 * 解析通、**执行未接**：SmartBox 的下行是"指示灯 / 隐私模式 / 语音指令"这类动作，
 * 必须过 `smartbox_node` 的安全联锁。哪个名字对应哪个聚合根方法属于产品决策，
 * 在映射表定下来之前明确拒绝，比猜一个执行路径安全。
 */
static int _sb_nb_on_downlink(void *ctx, const char *address, const uint8_t *payload,
                              size_t len) {
    (void)ctx;
    char name[64];
    double value = 0.0;
    if (mqtt_json_decode_downlink(payload, len, name, sizeof(name), &value) != 0) {
        LOG_WARN(LOG_CAT_CORE, "SmartBox downlink on '%s': 不是 {p,v} 帧", address);
        return -1;
    }
    LOG_WARN(LOG_CAT_CORE,
             "SmartBox downlink on '%s': 解析出 %s=%.6g，但**未执行** —— "
             "缺 名字→聚合根方法 映射（须过 smartbox_node 安全联锁）",
             address, name, value);
    return -1;
}

static const edge_north_source_ops_t *smartbox_north_source_ops(void) {
    static const edge_north_source_ops_t ops = {
        .peek = _sb_nb_peek,
        .name = _sb_nb_name,
        .resolve_name = _sb_nb_resolve_name,
        .accept = NULL,
        .on_downlink = _sb_nb_on_downlink,
    };
    return &ops;
}

void smartbox_north_attach(datahub_t *hub) {
    /* 北向：数据不发出去，采集等于没做（ADR-0011）。
     * SmartBox 之前完全没有北向，这里补上最小可用的一条 MQTT 通道。
     * db 传 NULL = 没有 SnF 断网缓存（本产品还没接 sqlite 服务）；
     * 断网期间的数据只在本地，这一点与 DCU/IEMS 不同，属于已知差异。 */
    const char *mqtt_url = getenv("MQTT_BROKER");
    if (!mqtt_url || !*mqtt_url) mqtt_url = "tcp://127.0.0.1:1883";
    char sb_client_id[80];
    snprintf(sb_client_id, sizeof(sb_client_id), "SMARTBOX_%d", (int)getpid());
    nb_transport_ops_t sb_transport = mqtt_producer_create(mqtt_url, sb_client_id);
    northbound_mgr_init_ex(&g_north, northbound_source_from_datahub(hub), "SmartBox", NULL,
                           sb_transport);
    /* 主题身份在这里读一次 env 并交给北向管理器，深层模块不再自己 getenv。 */
    northbound_mgr_set_address(&g_north, NULL, getenv("MQTT_GROUP_ID"),
                               getenv("MQTT_NODE_ID"));
    northbound_mgr_set_north_source(&g_north, smartbox_north_source_ops(), hub);
    /* 协议可由部署选（E2E 用 MqttJson：载荷是 JSON，平台侧能解；
     * 现场默认仍是 Sparkplug B）。换协议时地址模板跟着换。 */
    const char *nb_proto = getenv("MQTT_PROTOCOL");
    if (nb_proto && *nb_proto && northbound_mgr_set_protocol(&g_north, nb_proto) != 0) {
        LOG_WARN(LOG_CAT_CORE, "Northbound: unknown protocol '%s', keeping default", nb_proto);
    }
    if (northbound_mgr_start(&g_north) != 0) {
        LOG_WARN(LOG_CAT_CORE, "Northbound disabled (thread start failed)");
    } else {
        LOG_INFO(LOG_CAT_CORE, "🌐 Northbound assembled: broker=%s client=%s", mqtt_url,
                 sb_client_id);
    }
}

void smartbox_north_detach(void) {
    /* 北向线程要显式停：它在自己的 loop 上跑，主 loop 退出不会带它走。 */
    northbound_mgr_stop(&g_north);
    northbound_mgr_destroy(&g_north);
}
