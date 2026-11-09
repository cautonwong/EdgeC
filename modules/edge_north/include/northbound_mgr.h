#ifndef NORTHBOUND_MGR_H
#define NORTHBOUND_MGR_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "edge_request.h"
#include "shmring.h"

// 💡 [Golang 原则] 使用者定义传输契约
typedef struct {
    void* ctx;
    int (*publish)(void* ctx, const char* topic, const uint8_t* payload, size_t len);

    /**
     * @brief 订阅下行。
     *
     * `user` 会原样回传给 `on_msg` —— 没有这个参数，订阅方只能在回调里
     * 用全局变量找回自己，多实例就废了。
     *
     * 传输层必须保证：**重连后自动重新订阅**。MQTT 的 cleansession=1 会让
     * 服务端丢掉订阅关系，重连不重订阅等于下行永久失效。
     */
    int (*subscribe)(void* ctx, const char* topic,
                     void (*on_msg)(void* user, const char* topic, const uint8_t* p, size_t len),
                     void* user);

    bool (*is_connected)(void* ctx);

    /**
     * @brief 在途消息数：已交给传输、但还没被对端确认的条数。
     *
     * `publish` 返回 0 只代表**入队成功**，不代表 broker 收到。SnF 靠这个数判定
     * "这一批是否真的送达"：为 0 才敢把记录标成 synced。
     *
     * 为 NULL 时视为"无在途"（同步型传输，比如本地文件/共享内存）。
     */
    uint32_t (*pending)(void* ctx);

    /**
     * @brief 会话代数：每次成功建立（或重建）连接 +1。
     *
     * 用来判断"是不是一个新会话"。北向驱动靠它决定要不要重发 BIRTH ——
     * 只看 is_connected() 的 false→true 跳变会漏：断连与重连可能发生在
     * 两次采样之间。代数单调递增，比较值就不会漏。
     *
     * 为 NULL 时视为"永远第 0 代"（不支持重连的传输）。
     */
    uint32_t (*session_epoch)(void* ctx);
} nb_transport_ops_t;

// 💡 [Consumer-Driven] 数据源抽象：Northbound 不再直接依赖 shm_ring_t
typedef struct {
    void* ctx;
    /** 读取下一条消息。返回 0=有数据, 1=无数据, <0=错误 */
    int (*read_next)(void* ctx, uint64_t* seq, pipe_msg_t* out);

    /**
     * @brief 注册"有新数据"通知（可选）。
     *
     * 没有它，北向只能定时轮询 ring，延迟下限就是轮询周期（原来 100ms）。
     * 有了它，数据一进来就能唤醒北向线程。
     *
     * 回调**可能在任意线程触发**（写入方的线程），实现方要自己保证线程安全；
     * 北向这边只做 `uv_async_send`，是线程安全的。
     *
     * 为 NULL 时退回轮询。
     */
    int (*subscribe_notify)(void* ctx, void (*on_data)(void* user), void* user);
} nb_source_ops_t;

/** 最多挂几路北向传输：云端 MQTT + 本地 SCADA OPC UA + 预留 */
#define NB_MAX_TRANSPORTS 4

/** 地址模板长度上限（`spBv1.0/{group}/{kind}/{node}` 这类） */
#define NB_ADDRESS_TEMPLATE_MAX 128

typedef struct {
    uv_thread_t thread;
    /* 线程是否已创建。`stop` 靠它决定要不要 join：
     * 驱动建不起来时线程会提前 return（loop 保持 NULL），
     * 若只看 loop 就会漏掉 join，线程句柄泄漏。 */
    bool thread_started;
    uv_loop_t* loop;
    uv_loop_t loop_storage;  // [Issue #385] Caller-Allocated loop storage (0 malloc)
    uv_timer_t poll_timer;
    uv_async_t stop_async;
    uv_async_t req_async;
    /* 数据到达唤醒：由 source 的 subscribe_notify 触发，绕过轮询周期。
     * 没有它时端到端延迟下限 = poll_timer 周期。 */
    uv_async_t data_async;

    // 💡 [Consumer-Driven] 使用抽象数据源代替直接 shm_ring_t 依赖
    nb_source_ops_t source;
    uint64_t last_seq;

    void* user_data;
    struct sqlite_svc_s* db;
    struct snf_manager_s* snf;

    char product_name[64];
    int rest_port;
    void* rest_srv;

    /* 多路传输：publish 时循环，任一路存活即算在线。
     * 工业现场"云端 MQTT + 本地 SCADA OPC UA"是常态，单传输做不到（ADR-0011）。 */
    nb_transport_ops_t transports[NB_MAX_TRANSPORTS];
    uint32_t transport_count;

    /* 地址规则：**每路传输一份**，由编排层按模板渲染。
     * 占位符 {group} {node} {kind} {point}。
     * 不给驱动，是因为 OPC UA 没有 topic（它的地址是 NodeId）——
     * 把地址塞进驱动等于逼它拼一个假 topic。 */
    char address_template[NB_ADDRESS_TEMPLATE_MAX];
    /* 地址模板是否由调用方显式配置过。没配过时，换协议会跟着换成该协议的缺省模板。 */
    bool address_explicit;
    char group_id[64];
    char node_id[64];

    /* 北向协议驱动名（"SparkplugB" / 以后的 "MqttJson" 等）。
     * 硬编码会让换协议必须改这个文件。 */
    char protocol_name[32];

    /* 上一次观察到的传输会话代数签名。UINT32_MAX = 还没采样过（首次只记基线）。 */
    uint32_t last_epoch;

    /* 下行是否已订阅。用惰性订阅（在 poll 里补）而不是在 start 里一次定死：
     * 组合根可能在 start 之后才注入北向端口，那时也该订得上。 */
    bool downlink_subscribed;

    struct driver_manager_s* drv_mgr;
    struct edge_driver_ctx_s* nb_driver;
    /* [ADR-002] 北向生产者端口：由组合根注入，驱动不碰 datahub。
     * 存指针而不是内嵌，避免根目录头文件依赖 edge_io 的 include 路径。 */
    const struct edge_north_source_ops_s* north_src;
    void* north_src_ctx;

    struct uv__queue req_queue;
    uv_mutex_t req_lock;
} northbound_mgr_t;

typedef struct {
    edge_req_t* req;
    struct uv__queue node;
} nb_req_entry_t;

struct datahub_s;

/**
 * @brief 用 datahub 组一个北向数据源（`read_next` + 事件唤醒）。
 *
 * 组合根不用自己拼 `nb_source_ops_t`：少写一次就少一次漏掉
 * `subscribe_notify` 的机会——漏了不报错，只是延迟悄悄回到 500ms。
 */
nb_source_ops_t northbound_source_from_datahub(struct datahub_s* hub);

// 兼容性聚合根初始化：注入 datahub 聚合根安全适配
int northbound_mgr_init(northbound_mgr_t* mgr, uv_loop_t* loop, struct datahub_s* hub,
                        const char* product_name, struct sqlite_svc_s* db);

// 物理闭环初始化：注入 source 和一路 transport 实现
int northbound_mgr_init_ex(northbound_mgr_t* mgr, nb_source_ops_t source, const char* product_name,
                           struct sqlite_svc_s* db, nb_transport_ops_t transport);

/**
 * @brief 再挂一路传输（多路并存）。
 *
 * `init_ex` 已经挂了第一路；要同时上云和给本地 SCADA 就再调这个。
 * 超过 `NB_MAX_TRANSPORTS` 返回 -1。
 */
int northbound_mgr_add_transport(northbound_mgr_t* mgr, nb_transport_ops_t transport);

/**
 * @brief 配地址规则与节点身份。
 *
 * @param address_template 含 `{group}` / `{node}` / `{kind}` / `{point}` 占位符，
 *                         如 `spBv1.0/{group}/{kind}/{node}`。
 *                         NULL 或空串 → 用缺省 `spBv1.0/{group}/{kind}/{node}`。
 * @param group_id / node_id 可空，空则取环境变量 `MQTT_GROUP_ID` / `MQTT_NODE_ID`。
 *
 * 模板必须**按协议配**（Sparkplug 的 `{kind}` 是 NBIRTH/NDATA，与裸 JSON 不同），
 * 但换协议不需要改 `northbound_mgr.c`。
 */
void northbound_mgr_set_address(northbound_mgr_t* mgr, const char* address_template,
                                const char* group_id, const char* node_id);

/**
 * @brief 选北向协议驱动（缺省 "SparkplugB"）。未注册的名字返回 -1。
 *
 * 必须在 `northbound_mgr_start` 之前调用（驱动实例在 start 时创建）。
 */
int northbound_mgr_set_protocol(northbound_mgr_t* mgr, const char* protocol_name);

/**
 * @brief 按当前地址模板渲染一个地址（诊断与测试用）。
 *
 * 单独暴露是为了让模板逻辑可单测：模板写错时主题"看起来正常但不对"，
 * 主站收不到数据却不报错——这类问题必须能在单元测试里抓到。
 *
 * @param kind  NBIRTH / NDATA / NDEATH 等
 * @param point metric 名，可为 NULL（NBIRTH 不带点位）
 * @return 0 成功；-1 参数非法
 */
int northbound_mgr_render_address(northbound_mgr_t* mgr, const char* kind, const char* point,
                                  char* out, size_t out_sz);

/**
 * @brief 把协议编解码器注册进组合根建好的驱动管理器。
 *
 * 组件只保留「协议名对应哪个编解码器」这条策略；驱动的创建在组合根。
 */
int northbound_mgr_register_codec(const char* protocol_name, struct driver_manager_s* drv_mgr);

/** @brief 当前协议名（组合根建驱动实例时要用同一个名字）。 */
const char* northbound_mgr_get_protocol(const northbound_mgr_t* mgr);

/**
 * @brief 接管组合根建好的驱动。所有权转给管理器，destroy 时一并释放。
 *
 * 必须在 northbound_mgr_start() 之前调用；没接管驱动时线程不会运行。
 */
int northbound_mgr_attach_driver(northbound_mgr_t* mgr, struct driver_manager_s* drv_mgr,
                                 struct edge_driver_ctx_s* driver);

int northbound_mgr_start(northbound_mgr_t* mgr);
void northbound_mgr_stop(northbound_mgr_t* mgr);
void northbound_mgr_destroy(northbound_mgr_t* mgr);

int northbound_mgr_submit_req(northbound_mgr_t* mgr, edge_req_t* req);
void northbound_mgr_set_rest_port(northbound_mgr_t* mgr, int port);
void northbound_mgr_set_rest_server(northbound_mgr_t* mgr, void* rest_srv);

/**
 * @brief 注入北向取数端口（组合根调用）。
 *
 * 没有这个端口，驱动拿不到 metric 名，只能回落成 `tag_%u`；
 * `req_tick` 也拿不到点位列表，NBIRTH 里没有清单。
 *
 * 端口**不存进 `edge_driver_ctx_t`**：进了 ctx，驱动就又变成"自己取数"，
 * 等于把已删除的 `read_shadow` 换个名字请回来。这里存的是编排层的指针，
 * 每次调用时现传给驱动。
 *
 * @param ops 由产品实现（只有产品认识自己的点位模型）；NULL 表示不提供
 * @param ctx 传给 ops 的上下文
 */
/**
 * @brief 发布一条下行回执：metric 名 `<name>.ack`，值 1=执行成功 / 0=失败。
 *
 * 为什么必须有：主站发了命令，如果只有"设备没回应"和"设备执行了但没吭声"两种可能，
 * 现场就只能靠猜。回执把"收到了但拒绝了"与"根本没收到"分开。
 *
 * 两个刻意的选择：
 *   - **不走 SnF**：断网时缓存一条三天前的"执行成功"回执照样发出去，比不发更糟。
 *     发不出去就是发不出去，主站按超时处理。
 *   - **走驱动的 native 编码**（不是拼 JSON）：回执和遥测走同一条编码路径，
 *     Sparkplug 与裸 JSON 各自编成自己该有的样子，编排层不认识协议。
 *
 * @param tag_id 该命令对应的点位（用于回落命名，可为 0）
 * @param name   点位名（NULL 时回执名为 `ack.<tag_id>`）
 * @return 0 已交给传输；<0 未连接或编码失败
 */
int northbound_mgr_publish_ack(northbound_mgr_t* mgr, uint32_t tag_id, const char* name, bool ok);

void northbound_mgr_set_north_source(northbound_mgr_t* mgr,
                                     const struct edge_north_source_ops_s* ops, void* ctx);

#endif
