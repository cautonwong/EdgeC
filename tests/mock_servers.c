#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <string.h>

#include "rest_server.h"
#include "web_server.h"

// Mock Web Server
struct web_server_s {
    int dummy;
};
web_server_t* web_server_create(uv_loop_t* loop, int port, const char* doc_root) {
    (void)loop;
    (void)port;
    (void)doc_root;
    return malloc(sizeof(web_server_t));
}
void web_server_destroy(web_server_t* srv) {
    free(srv);
}
void web_server_set_api_handler(web_server_t* srv, web_api_handler_pt handler, void* user_data) {
    (void)srv;
    (void)handler;
    (void)user_data;
}

// Mock REST Server
struct rest_server_s {
    int dummy;
};
rest_server_t* rest_server_create(uv_loop_t* loop, service_t* svc, int port) {
    (void)loop;
    (void)svc;
    (void)port;
    return malloc(sizeof(rest_server_t));
}
void rest_server_destroy(rest_server_t* srv) {
    free(srv);
}

char* rest_dispatch(service_t* svc, const char* path, const char* body) {
    (void)svc;
    (void)path;
    (void)body;
    return strdup("{}");
}
