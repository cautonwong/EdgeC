#ifndef REST_SERVER_H
#define REST_SERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct uv_loop_s uv_loop_t;

#include "edge_service.h"

// Opaque handle
typedef struct rest_server_s rest_server_t;

#define REST_SERVER_STORAGE_SIZE (8 * 70000 + 4096)

typedef struct {
    uint64_t _align;
    uint8_t opaque[REST_SERVER_STORAGE_SIZE];
} rest_server_storage_t;

#define REST_DEFAULT_BIND_ADDR "127.0.0.1"
#define REST_DEFAULT_MAX_CLIENTS 8
#define REST_DEFAULT_MAX_HEADER_BYTES (8 * 1024)
#define REST_DEFAULT_MAX_BODY_BYTES (1024 * 1024)
#define REST_DEFAULT_READ_TIMEOUT_MS 30000

/**
 * @brief REST 管理面边界限制。零值字段走默认值，见 REST_DEFAULT_*。
 *
 * bind_addr 默认只绑回环；allow_cors_origin 为 NULL/空串时不发 CORS 头。
 * max_clients 是编译期数组上限 REST_DEFAULT_MAX_CLIENTS 的向下配置。
 */
typedef struct {
    const char *bind_addr;
    int max_clients;
    size_t max_header_bytes;
    size_t max_body_bytes;
    int read_timeout_ms;
    const char *allow_cors_origin;
} rest_limits_t;

void rest_limits_default(rest_limits_t *limits);

/**
 * @brief Init the REST API Server with explicit limits.
 *
 * @param storage Caller-allocated storage; NULL allocates on the heap (freed by destroy).
 * @param limits  NULL applies rest_limits_default().
 */
rest_server_t *rest_server_create_in_ex(rest_server_storage_t *storage, uv_loop_t *loop,
                                        service_t *svc, int port,
                                        const rest_limits_t *limits);

/**
 * @brief Init the REST API Server in caller-allocated storage with default limits.
 */
rest_server_t *rest_server_create_in(rest_server_storage_t *storage, uv_loop_t *loop, service_t *svc, int port);

/**
 * @brief Init the REST API Server (Data/Control) with default limits.
 *
 * @param loop The loop to run on.
 * @param svc The service container (for command bridge).
 * @param port The port to listen on.
 */
rest_server_t *rest_server_create(uv_loop_t *loop, service_t *svc, int port);

void rest_server_destroy(rest_server_t *srv);

/**
 * @brief Legacy /api/v1/apps/upload (raw wasm) gate. Defaults to closed; closed returns 410.
 */
void rest_server_set_legacy_upload(bool enable);

void rest_server_enable_write(bool enable);

/**
 * @brief Actual bound address, read back from the listening socket.
 *
 * @return 0 on success, -1 when the server is not bound.
 */
int rest_server_get_bound_addr(const rest_server_t *srv, char *ip, size_t ip_len, int *port);

char *rest_dispatch(service_t *svc, const char *method, const char *path, const char *body);

/**
 * @brief Dispatch with the HTTP status the handler wants to return.
 *
 * @param status Out-param, 200 when the route succeeded.
 */
char *rest_dispatch_ex(service_t *svc, const char *method, const char *path, const char *body,
                       int *status);

#endif
