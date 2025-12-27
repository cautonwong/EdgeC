#ifndef REST_SERVER_H
#define REST_SERVER_H

#include <uv.h>
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
rest_server_t* rest_server_create(uv_loop_t *loop, service_t *svc, int port);

void rest_server_destroy(rest_server_t *srv);

#endif
