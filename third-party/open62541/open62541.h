#ifndef OPEN62541_H
#define OPEN62541_H

typedef struct UA_Client UA_Client;
typedef struct UA_ClientConfig UA_ClientConfig;

typedef unsigned int UA_StatusCode;
#define UA_STATUSCODE_GOOD 0x00000000u

UA_Client *UA_Client_new(void);
UA_ClientConfig *UA_Client_getConfig(UA_Client *client);
void UA_ClientConfig_setDefault(UA_ClientConfig *config);
UA_StatusCode UA_Client_connect(UA_Client *client, const char *endpointUrl);
void UA_Client_run_iterate(UA_Client *client, int timeout);
void UA_Client_disconnect(UA_Client *client);
void UA_Client_delete(UA_Client *client);

#endif