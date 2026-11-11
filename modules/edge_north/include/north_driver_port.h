/**
 * @file north_driver_port.h
 * @brief 北向协议驱动的消费者定义端口。
 *
 * edge_north 只认识"把一条点位值编码成北向帧"这件事，不认识
 * driver_manager、edge_driver_ctx_t 或具体协议实现。实现由 adapters/io_drivers
 * 提供，组合根在 start 之前注入。
 */

#ifndef NORTH_DRIVER_PORT_H
#define NORTH_DRIVER_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "edge_north_plane.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct north_driver_s north_driver_t;

typedef struct {
    void *ctx;

    int (*attach)(void *ctx, const char *protocol, north_driver_t **out_driver);
    void (*detach)(void *ctx, north_driver_t *driver);

    /* 编码一条点位值。返回的指针由端口持有，有效期到下一次 encode 或
     * detach；调用方必须在下一次编码前完成 publish。 */
    int (*encode_value)(void *ctx, north_driver_t *driver, uint32_t tag_id, double value,
                        const char *name, const uint8_t **out_data, size_t *out_len);

    /* 编码周期帧（NBIRTH/状态帧）。语义同上。 */
    int (*encode_tick)(void *ctx, north_driver_t *driver,
                       const edge_north_source_ops_t *north, void *north_ctx,
                       const uint8_t **out_data, size_t *out_len);

    /* 传输会话重建后清掉驱动的会话级状态。可为 NULL。 */
    void (*reset_session)(void *ctx, north_driver_t *driver);
} north_driver_port_t;

#ifdef __cplusplus
}
#endif

#endif /* NORTH_DRIVER_PORT_H */
