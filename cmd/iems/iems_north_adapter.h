#ifndef IEMS_NORTH_ADAPTER_H
#define IEMS_NORTH_ADAPTER_H

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

typedef struct {
    iems_node_t *node;
    datahub_t *hub;
    void *north;
    int (*submit_control)(void *ctx, const iems_device_route_t *route, uint32_t tag_id,
                          double value);
    void *submit_ctx;
    /* 控制写的路由：按 tag 反查目标设备，失败则拒绝，不再写死 bus 0 / 从站 1。 */
    int (*route_for_tag)(void *ctx, uint32_t tag_id, iems_device_route_t *route);
    void *route_ctx;
} iems_north_ctx_t;

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

/* 控制写的产品侧接线（北向与反射链路共用）。 */
int iems_control_route_for_tag(void *ctx, uint32_t tag_id, iems_device_route_t *route);
/* ctx = iems_node_t*；slave_id 由 route 给出，调用方不猜默认值。 */
int iems_control_submit(void *ctx, const iems_device_route_t *route, uint32_t tag_id, double value);

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
