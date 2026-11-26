#include "dcu_north_adapter.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "dcu_698_axdr.h"
#include "dcu_698_appconn.h"
#include "dcu_698_object_internal.h"
#include "dcu_698_record.h"
#include "dcu_698_report.h"
#include "dcu_698_var.h"
#include "dcu_node.h"
#include "dcu_north_source.h"
#include "dcu_router.h"
#include "edge_log.h"
#include "edge_slave_plane.h"
#include "io_executor.h"
#include "mqtt_producer.h"
#include "north_driver_adapter.h"
#include "north_source_datahub_adapter.h"
#include "northbound_mgr.h"

typedef struct {
    bus_orchestrator_t* orch;
    datahub_t* hub;
} dcu_north_ctx_t;

static dcu_north_ctx_t g_dcu_north_ctx;

int dcu_north_resolve_scalar(void* ctx, uint32_t oad, const void* desc, size_t desc_len,
                             edge_point_value_t* out) {
    (void)ctx;
    (void)desc;
    (void)desc_len;
    if (!out) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    dcu_value_t v;
    int rc = dcu_698_value_for_oad(oad, &v);
    if (rc != 0) return rc;

    /* 端口 raw 是 int32；64 位类型不能截断后冒充成功，交给 resolve_axdr。 */
    if (v.width == 0 || v.width > sizeof(out->raw)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    memset(out, 0, sizeof(*out));
    out->value = v.value;
    out->raw = (int32_t)v.raw;
    out->width = v.width;
    out->type = v.type_tag;
    out->quality = v.quality;
    out->ts_ms = v.ts_ms;
    return 0;
}

int dcu_north_accept_action(void* ctx, uint32_t omd, const uint8_t* axdr, size_t axdr_len) {
    (void)ctx;
    uint16_t oi = (uint16_t)(omd >> 16);
    uint8_t method = (uint8_t)((omd >> 8) & 0xFF);
    if (oi == OI_METER_ARCHIVE) return dcu_698_accept_archive_action(omd, axdr, axdr_len);
    if (oi == OI_TASK_SCHEDULER || oi == OI_NORMAL_SCHEME || oi == OI_METER_DISCOVERY ||
        oi == OI_EVENT_SCHEME || oi == OI_TRANSPARENT_SCHEME || oi == OI_REPORT_SCHEME ||
        oi == OI_RULE_LIBRARY || (oi >= 0x5000 && oi <= 0x5007))
        return dcu_698_accept_action_axdr(omd, axdr, axdr_len);
    if (oi != OI_REMOTE_CONTROL) return dcu_698_call(omd, axdr, NULL);

    dcu_action_param_t param = {0};
    int rc = dcu_698_decode_action_param(method, axdr, axdr_len, &param);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU north ACTION OI=8000 method=%u：参数解码失败 rc=%d", method,
                 rc);
        return rc;
    }
    return dcu_698_call(omd, &param, NULL);
}

static void _on_dcu_north_connect(uv_stream_t* server, int status) {
    if (status < 0) return;
    dcu_north_ctx_t* nc = (dcu_north_ctx_t*)server->data;
    uv_pipe_t* client = malloc(sizeof(uv_pipe_t));
    if (!client) return;
    uv_pipe_init(server->loop, client, 0);
    if (uv_accept(server, (uv_stream_t*)client) == 0 && nc && nc->orch && nc->hub) {
        int fd = -1;
        uv_fileno((const uv_handle_t*)client, &fd);
        int new_fd = (fd >= 0) ? dup(fd) : -1;
        if (new_fd >= 0) {
            int flags = fcntl(new_fd, F_GETFL, 0);
            fcntl(new_fd, F_SETFL, flags | O_NONBLOCK);
            io_handle_t* h =
                io_executor_create_from_fd(server->loop, new_fd, datahub_get_io_pool(nc->hub));
            if (h) {
                bus_orchestrator_attach_io(nc->orch, DCU_NORTH_BUS, h);
                io_handle_enable_read(h, true);
            } else {
                close(new_fd);
            }
        }
    }
    uv_close((uv_handle_t*)client, (uv_close_cb)free);
}

static dcu_698_record_ctx_t g_record_ctx;

void dcu_north_slave_attach(uv_loop_t* loop, datahub_t* hub, bus_orchestrator_t* orch,
                            dcu_node_t* node) {
    const char* path = getenv("DCU_NORTH_SOCK");
    if (!path || !path[0]) path = "/tmp/dcu_north_slave.sock";

    if (bus_orchestrator_setup_slave(orch, DCU_NORTH_BUS, "DLT698", 1) != 0) {
        LOG_ERROR(LOG_CAT_NB, "DCU northbound: 698 从站引擎建不起来，北向不可用");
        return;
    }
    if (node) {
        dcu_698_record_init(&g_record_ctx, dcu_node_get_storage(node), dcu_node_get_inventory(node));
    }
    static const edge_slave_source_ops_t src = {
        .resolve = dcu_north_resolve_scalar,
        .accept_action = dcu_north_accept_action,
        .resolve_axdr = dcu_north_resolve_axdr,
        .accept_axdr = dcu_north_accept_axdr,
        .object_list = dcu_698_appconn_resolve,
        .record = {.begin = dcu_698_record_begin,
                   .row = dcu_698_record_row,
                   .end = dcu_698_record_end},
        .report = {.next = dcu_698_report_next},
    };
    bus_orchestrator_set_slave_source(orch, DCU_NORTH_BUS, &src, &g_record_ctx);

    g_dcu_north_ctx.orch = orch;
    g_dcu_north_ctx.hub = hub;

    static uv_pipe_t srv;
    uv_pipe_init(loop, &srv, 0);
    srv.data = &g_dcu_north_ctx;
    unlink(path);
    int bind_rc = uv_pipe_bind(&srv, path);
    int listen_rc =
        (bind_rc == 0) ? uv_listen((uv_stream_t*)&srv, 16, _on_dcu_north_connect) : bind_rc;
    if (bind_rc == 0 && listen_rc == 0) {
        LOG_INFO(LOG_CAT_NB, "DCU northbound: 698 slave listening at %s", path);
    } else {
        LOG_ERROR(LOG_CAT_NB, "DCU northbound: 从站监听失败 %s (bind=%s, listen=%s)", path,
                  uv_strerror(bind_rc), uv_strerror(listen_rc));
        if (bind_rc == 0) unlink(path);
    }
}

/* 北向编排实例与传输。文件作用域：装配在 helper 里做，
 * service_new 与收尾（stop/destroy）都还要用它们。 */
static northbound_mgr_t g_north;
static north_driver_adapter_storage_t g_north_driver_storage;
static nb_transport_ops_t g_nb_transport;

/**
 * @brief 装配北向（ADR-0011 族 A）。
 *
 * 抽成独立函数不是因为 main 太长不好看，而是因为 main 已经到 200 行了 ——
 * 单一职责门禁盯的正是这个：一个函数里既装南向又装北向，读的人得同时装两套上下文。
 */
void dcu_north_cloud_attach(datahub_t* hub) {
    // 北向：数据不发出去，采集等于没做。传输层由 edge_io 提供，组合根只负责装配。
    // 主题身份在这里读一次 env 并交给北向管理器：MULTI-NODE 台架并发跑两台 DCU 不能撞主题。
    const char* mqtt_url = getenv("MQTT_BROKER");
    if (!mqtt_url || !*mqtt_url) mqtt_url = "tcp://127.0.0.1:1883";
    const char* group_id = getenv("MQTT_GROUP_ID");
    if (!group_id || !*group_id) group_id = "EdgeOS";
    const char* node_id = getenv("MQTT_NODE_ID");
    if (!node_id || !*node_id) node_id = "DCU_Node_A";
    char mqtt_client_id[80];
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%s_%d", node_id, (int)getpid());
    /* 数据源由工厂组：read_next + 事件唤醒。手拼容易漏掉 subscribe_notify，
     * 漏了不报错，只是端到端延迟悄悄回到轮询周期。 */
    nb_source_ops_t nb_source = northbound_source_from_datahub(hub);
    g_nb_transport = mqtt_producer_create(mqtt_url, mqtt_client_id);
    northbound_mgr_init_ex(&g_north, nb_source, "DCU", NULL, g_nb_transport);
    northbound_mgr_set_driver_port(&g_north, north_driver_adapter_init(&g_north_driver_storage));
    northbound_mgr_set_address(&g_north, NULL, group_id, node_id);
    /* 北向取数端口：metric 名与当前值都由产品给，驱动不反查 datahub。
     * 不注入的话驱动只能把点位编成 tag_<数字>，主站看不出是什么量。 */
    northbound_mgr_set_north_source(&g_north, dcu_north_source_ops(), hub);
    /* 协议可由部署选（E2E 用 MqttJson：载荷是 JSON，平台侧能解；
     * 现场默认仍是 Sparkplug B）。换协议时地址模板跟着换。 */
    const char* nb_proto = getenv("MQTT_PROTOCOL");
    if (nb_proto && *nb_proto && northbound_mgr_set_protocol(&g_north, nb_proto) != 0) {
        LOG_WARN(LOG_CAT_NB, "Northbound: unknown protocol '%s', keeping default", nb_proto);
    }
    LOG_INFO(LOG_CAT_NB, "🌐 Northbound assembled: broker=%s group=%s node=%s", mqtt_url, group_id,
             node_id);
}

void dcu_north_stop(void) {
    northbound_mgr_stop(&g_north);
}

northbound_mgr_t* dcu_north_mgr(void) {
    return &g_north;
}

void dcu_north_destroy(void) {
    northbound_mgr_destroy(&g_north);
    mqtt_producer_destroy(&g_nb_transport);
}
