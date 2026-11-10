#ifndef JS_ARBITER_SERVICE_ADAPTER_H
#define JS_ARBITER_SERVICE_ADAPTER_H

#include "js_arbiter_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct service_s service_t;

js_arbiter_port_t js_arbiter_port_from_service(service_t *svc);

#ifdef __cplusplus
}
#endif

#endif /* JS_ARBITER_SERVICE_ADAPTER_H */
