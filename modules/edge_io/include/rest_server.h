#ifndef REST_SERVER_H
#define REST_SERVER_H

typedef struct uv_loop_s uv_loop_t;

#include "edge_service.h"

// Opaque handle
typedef struct rest_server_s rest_server_t;

/**
 * @brief Init the REST API Server (Data/Control).
 *
 * @param loop The loop to run on.
 * @param svc The service container (for command bridge).
 * @param port The port to listen on.
 */
rest_server_t *rest_server_create(uv_loop_t *loop, service_t *svc, int port);

void rest_server_destroy(rest_server_t *srv);

void rest_server_enable_write(bool enable);

char *rest_dispatch(service_t *svc, const char *method, const char *path, const char *body);

#endif
