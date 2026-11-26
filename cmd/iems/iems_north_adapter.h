#ifndef IEMS_NORTH_ADAPTER_H
#define IEMS_NORTH_ADAPTER_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "datahub.h"
#include "edge_north_plane.h"
#include "edge_slave_plane.h"
#include "iems_node.h"
#include "iems_task_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 北向下行回执语义（P4-03c1，用例 tests/test_northbound_downlink_e2e.c）：
 *
 * 每次合法 {p,v} 下行发两类回执，主题都走现有地址模板（NDATA）：
 *
 * 1) accepted 回执 `<name>.ack`：提交到执行端口后发，值 1=已接受 / 0=被拒。
 *    它只表示控制写已交给执行端口/总线仲裁，不代表设备已执行。
 * 2) 结果回执 `<name>.result`：总线结算后经结果回调发，值 1=成功、0=失败、2=超时。
 *    名字为空时回落 `result.<tag_id>`。成功/失败/超时的映射：
 *    CMD_STATUS_SUCCESS→1，CMD_STATUS_TIMEOUT→2，其余非零→0。
 *
 * 线程边界：两类回执都只入队 northbound_mgr 的线程安全回执队列（uv_async 回投
 * 北向 loop），编码与发布都在北向 loop 线程完成。下行回调线程与总线完成线程都
 * 不直接 encode/publish，驱动里的非原子会话状态（如 Sparkplug seq）不会被并发写。
 *
 * 结果上下文池是文件级定长池（进程级，单 IEMS 实例假设），有自己的互斥锁：
 * 认领、reset 与完成回调的槽访问都持同一把锁；回调在锁内拷贝 (tag, name, mgr)，
 * 解锁后才投递队列，reset 返回后迟到的回调不再解引用 ctx。池满时该命令仍发
 * accepted 回执，但不再登记结果，只记 LOG_WARN 并丢弃结果回执；不阻塞、不泄漏。
 * destroy 前由组合根调用 iems_north_results_reset()：清空在途槽并把 ctx->north
 * 置 NULL，管理器停止后迟到的完成回调只会看到失效槽并安全丢弃。
 *
 * 失败回执：坏帧回 `ack.<tag_id>`（tag_id=0），未知点名/无唯一路由/提交失败带原
 * 点位名与 tag，ok=0；这些路径没有可登记的完成上下文，不会产生结果回执。
 *
 * PCS_CONTROL 残余：该点位走 iems_node_dispatch_power 的安全/功率分配，结果回执只覆盖
 * 最终这一路 execution.write_tag；dispatch 内部的多路分摊（未来多 PCS 拆分写）未聚合，
 * accepted 语义仍然有效，多路结果需后续按“命令序号 + 聚合”另立任务。
 *
 * 重复命令：{p,v} 帧没有命令 ID/序号，同一帧每收到一次就执行一次控制写并各回一次
 * accepted（与结果）回执；幂等由主站/上层负责（命令 ID 去重窗口见
 * NORTHBOUND_ARCHITECTURE N6-4）。
 *
 * 业务校验：除 PCS_CONTROL 外，{p,v} 下行按路由直接写点位，没有经过 accept()/聚合根
 * 方法，也没有 SOC 包络等业务校验；N6-2 的“字段 → 聚合根方法”映射仍待组合根补齐（见
 * docs/design/NORTHBOUND_ARCHITECTURE.md 的 N6-2 小节）。
 *
 * 写权限：下行当前没有写权限检查，任何能发布到该主题的客户端都能控点；鉴权矩阵
 * X-03a 另立，当前依赖 broker/网络 ACL。 */

#define IEMS_NORTH_RESULT_SLOTS 16
#define IEMS_NORTH_RESULT_NAME_MAX 64

typedef struct iems_north_ctx_s {
    iems_node_t *node;
    datahub_t *hub;
    /* 下行回调在传输/loop 线程读它；reset 在组合根线程置 NULL，必须原子访问。 */
    _Atomic(void *) north;
    int (*submit_control)(void *ctx, const iems_device_route_t *route, uint32_t tag_id,
                          double value, iems_write_done_fn on_done, void *on_done_user);
    void *submit_ctx;
    /* 控制写的路由：按 tag 反查目标设备，失败则拒绝，不再写死 bus 0 / 从站 1。 */
    int (*route_for_tag)(void *ctx, uint32_t tag_id, iems_device_route_t *route);
    void *route_ctx;
} iems_north_ctx_t;

/* 清空文件级在途结果槽并把 ctx->north 置 NULL。组合根在 northbound_mgr_stop 之前调用；
 * 也可用于初始化（ctx 清零后先调一次）。返回后迟到的完成回调只碰静态槽，不再解引用 ctx。 */
void iems_north_results_reset(iems_north_ctx_t *ctx);

const edge_north_source_ops_t *iems_north_source_ops(void);
const edge_slave_source_ops_t *iems_north_slave_ops(void);

/* IEC104 从站面的产品侧 ctx：取数走 datahub 影子，控制写走聚合根。
 * TCP 监听与单客户端接线在 iems_north_iec104.c。 */
typedef struct {
    datahub_t *hub;
    iems_node_t *node;
    sqlite_svc_t *db;
    int north_bus_id;
} iems_iec104_source_ctx_t;

const edge_slave_source_ops_t *iems_north_iec104_source_ops(void);

/* 控制写的产品侧接线（北向与反射链路共用）。on_done 可选，透传到执行端口。 */
int iems_control_route_for_tag(void *ctx, uint32_t tag_id, iems_device_route_t *route);
/* ctx = iems_node_t*；slave_id 由 route 给出，调用方不猜默认值。 */
int iems_control_submit(void *ctx, const iems_device_route_t *route, uint32_t tag_id, double value,
                        iems_write_done_fn on_done, void *on_done_user);

/* stream_control_cb 适配：只带 tag，设备身份从轮询路由取。 */
typedef struct {
    iems_node_t *node;
    sqlite_svc_t *db;
} iems_control_write_ctx_t;

int iems_control_submit_by_tag(void *ctx, int bus_id, uint32_t tag_id, double value,
                               const char *proto);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_NORTH_ADAPTER_H */
