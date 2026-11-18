#ifndef IEMS_NORTH_ADAPTER_H
#define IEMS_NORTH_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include "datahub.h"
#include "edge_north_plane.h"
#include "edge_slave_plane.h"
#include "iems_node.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    iems_node_t *node;
    datahub_t *hub;
    void *north;
    int (*submit_control)(void *ctx, int bus_id, uint32_t tag_id, double value,
                          const char *proto);
    void *submit_ctx;
    /* 控制写的路由：按 tag 反查 bus 与协议，失败则拒绝，不再写死 bus 0。 */
    int (*route_for_tag)(void *ctx, uint32_t tag_id, int *bus_id, char proto[16]);
    void *route_ctx;
} iems_north_ctx_t;

const edge_north_source_ops_t *iems_north_source_ops(void);
const edge_slave_source_ops_t *iems_north_slave_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* IEMS_NORTH_ADAPTER_H */
