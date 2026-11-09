#include "dcu_north_adapter.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "dcu_node.h"
#include "dcu_north_source.h"
#include "dcu_router.h"
#include "edge_log.h"
#include "edge_slave_plane.h"
#include "io_executor.h"
#include "mqtt_producer.h"
#include "northbound_driver_adapter.h"
#include "northbound_mgr.h"

typedef struct {
    bus_orchestrator_t* orch;
    datahub_t* hub;
} dcu_north_ctx_t;

static dcu_north_ctx_t g_dcu_north_ctx;

/**
 * @brief OAD → 当前值。类型标签由这里给（`width=2` 是 long 还是 long-unsigned
 * 只有点位模型知道，驱动不该猜 —— ADR-002）。
 */
static int _dcu_nb_resolve(void* ctx, uint32_t oad, const void* desc, size_t desc_len,
                           edge_point_value_t* out) {
    (void)ctx;
    (void)desc;
    (void)desc_len;
    if (!out) return -1;
    memset(out, 0, sizeof(*out));

    uint16_t oi = (uint16_t)(oad >> 16);
    if (oi == 0x2000 || oi == 0x2001 || oi == 0x2004) {
        /* 实时参量（电压/电流/功率）：DCU 侧是 float，698 用 float32 */
        float v = 0.0f;
        if (dcu_router_get_attr(oad, &v) != 0) return -1;
        out->value = (double)v;
        out->width = 4;
        out->type = 0x17; /* float32 */
    } else if (oi == 0x6000) {
        /* 档案类计数：uint16 */
        uint16_t v = 0;
        if (dcu_router_get_attr(oad, &v) != 0) return -1;
        out->value = (double)v;
        out->raw = (int32_t)v;
        out->width = 2;
        out->type = 0x12; /* long-unsigned */
    } else {
        /* 其它 OI 的属性类型没在本文件里定义过，猜一个类型标签会让主站解出错值
         * 且不报错。宁可不应答（主站超时），也不发一个"看起来对"的报文。 */
        return -1;
    }
    out->ts_ms = (uint64_t)(uv_hrtime() / 1000000ULL);
    return 0;
}

/**
 * @brief 收一次对象方法操作（OI 8000 跳闸/合闸等）。
 *
 * **当前明确返回失败**：标准表 A.9 规定方法 129/130 的参数是
 * `array structure{继电器 OAD, 告警延时, 限电时间, 自动合闸}`，解它需要按 698
 * 第 7 章把结构拆出来，再映射到"哪只表的继电器"。语义没确认前不猜 ——
 * 驱动会把非 0 变成 DAR=2（暂时失效），主站看到的是"设备暂时不能执行"，
 * 而不是"执行成功"。
 */
static int _dcu_nb_accept_action(void* ctx, uint32_t omd, const uint8_t* axdr, size_t axdr_len) {
    (void)ctx;
    (void)axdr;
    (void)axdr_len;
    uint16_t oi = (uint16_t)(omd >> 16);
    uint8_t method = (uint8_t)((omd >> 8) & 0xFF);
    LOG_WARN(LOG_CAT_NB,
             "DCU north ACTION OI=0x%04X method=%u：参数结构未实现（见标准表 A.9），回 DAR=2",
             oi, method);
    return -1;
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

void dcu_north_slave_attach(uv_loop_t* loop, datahub_t* hub, bus_orchestrator_t* orch) {
    const char* path = getenv("DCU_NORTH_SOCK");
    if (!path || !path[0]) path = "/tmp/dcu_north_slave.sock";

    if (bus_orchestrator_setup_slave(orch, DCU_NORTH_BUS, "DLT698", 1) != 0) {
        LOG_ERROR(LOG_CAT_NB, "DCU northbound: 698 从站引擎建不起来，北向不可用");
        return;
    }
    static const edge_slave_source_ops_t src = {
        .resolve = _dcu_nb_resolve,
        .accept_action = _dcu_nb_accept_action,
    };
    bus_orchestrator_set_slave_source(orch, DCU_NORTH_BUS, &src, NULL);

    g_dcu_north_ctx.orch = orch;
    g_dcu_north_ctx.hub = hub;

    static uv_pipe_t srv;
    uv_pipe_init(loop, &srv, 0);
    srv.data = &g_dcu_north_ctx;
    unlink(path);
    int bind_rc = uv_pipe_bind(&srv, path);
    int listen_rc = (bind_rc == 0) ? uv_listen((uv_stream_t*)&srv, 16, _on_dcu_north_connect)
                                   : bind_rc;
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
    if (northbound_attach_default_driver(&g_north) != 0) {
        LOG_WARN(LOG_CAT_NB, "Northbound: driver not attached, channel stays idle");
    }
    LOG_INFO(LOG_CAT_NB, "🌐 Northbound assembled: broker=%s group=%s node=%s", mqtt_url,
             group_id, node_id);
}


void dcu_north_stop(void) {
    northbound_mgr_stop(&g_north);
}

northbound_mgr_t *dcu_north_mgr(void) {
    return &g_north;
}

void dcu_north_destroy(void) {
    northbound_mgr_destroy(&g_north);
    mqtt_producer_destroy(&g_nb_transport);
}
