#include "open62541.h"
#include <stdlib.h>

struct UA_Client {
    int dummy;
};

struct UA_ClientConfig {
    int dummy;
};

UA_Client *UA_Client_new(void) {
    return calloc(1, sizeof(UA_Client));
}

UA_ClientConfig *UA_Client_getConfig(UA_Client *client) {
    (void)client;
    static UA_ClientConfig config;
    return &config;
}

void UA_ClientConfig_setDefault(UA_ClientConfig *config) {
    (void)config;
}

UA_StatusCode UA_Client_connect(UA_Client *client, const char *endpointUrl) {
    (void)client;
    (void)endpointUrl;
    return UA_STATUSCODE_GOOD;
}

void UA_Client_run_iterate(UA_Client *client, int timeout) {
    (void)client;
    (void)timeout;
}

void UA_Client_disconnect(UA_Client *client) {
    (void)client;
}

void UA_Client_delete(UA_Client *client) {
    free(client);
}