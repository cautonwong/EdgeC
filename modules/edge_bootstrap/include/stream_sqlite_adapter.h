#ifndef STREAM_SQLITE_ADAPTER_H
#define STREAM_SQLITE_ADAPTER_H

#include "stream_sqlite_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sqlite_svc_s sqlite_svc_t;

stream_sqlite_port_t stream_sqlite_port_from_sqlite(sqlite_svc_t *db);

#ifdef __cplusplus
}
#endif

#endif /* STREAM_SQLITE_ADAPTER_H */
