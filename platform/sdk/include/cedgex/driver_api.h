#ifndef DRIVER_API_H
#define DRIVER_API_H

#include <cedgex/common_types.h>

// Forward declaration of the driver context
typedef struct DriverContext DriverContext;

// Callback function type for the driver to submit data to the SDK
typedef void (*OnDataCb)(uint32_t tag_id, const cedgex_datapoint_t *dp, void *usr_data);

typedef struct {
    const char *name;
    DriverContext* (*init)(const char *json_config, OnDataCb cb, void *usr_data);
    int (*start)(DriverContext *ctx);
    int (*stop)(DriverContext *ctx);
    int (*poll)(DriverContext *ctx);
    int (*write)(DriverContext *ctx, uint32_t tag_id, const cedgex_datapoint_t *val);
    void (*destroy)(DriverContext *ctx);
} DriverOps;

#endif /* DRIVER_API_H */