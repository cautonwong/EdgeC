/**
 * @file rest_uds_server.h
 * @brief Local management channel over a unix socket.
 *
 * Requests are length-framed and dispatched through the same routes as the
 * REST server, so appctl and HTTP share one handler set.
 *
 * Frame layout, little endian:
 *
 *   request   "CTL1" | u32 method_len | u32 path_len | u32 body_len
 *             | method | path | body
 *   response  "CTL1" | u32 status | u32 body_len | body
 *
 * One request per connection. The peer credential is checked before any byte
 * is read. Allowed uids are 0, the server's own uid, and EDGEC_APPCTL_ALLOW_UID
 * when set.
 */
#ifndef REST_UDS_SERVER_H
#define REST_UDS_SERVER_H

#include <stddef.h>

#include "edge_service.h"

typedef struct uv_loop_s uv_loop_t;

#define REST_UDS_MAGIC "CTL1"
#define REST_UDS_MAX_METHOD 16
#define REST_UDS_MAX_PATH 128
#define REST_UDS_MAX_BODY (8u * 1024u * 1024u)

typedef struct rest_uds_server_s rest_uds_server_t;

/**
 * @brief Create and listen on the management socket.
 *
 * The path is unlinked first and recreated with mode 0600. Returns NULL when
 * the directory is missing or the bind fails; the caller logs and continues.
 */
rest_uds_server_t *rest_uds_server_start(uv_loop_t *loop, service_t *svc, const char *path);

/** Close the listener and remove the socket path. Safe on NULL. */
void rest_uds_server_stop(rest_uds_server_t *srv);

#endif /* REST_UDS_SERVER_H */
