#ifndef DRIVER_API_H
#define DRIVER_API_H

#include "../include/cedgex/common_types.h"

// Forward declare the driver context, making it an opaque pointer for the SDK
typedef struct DriverInternalContext DriverContext;

// Callback function type for the driver to submit data to the SDK's data hub
typedef void (*OnDataCb)(uint32_t tag_id, const cedgex_datapoint_t *dp,
                         void *usr_data);

typedef struct {
  const char *name;  // e.g., "modbus-tcp", "iec104"

  // --- Lifecycle Functions ---
  DriverContext *(*init)(const char *json_config, OnDataCb cb, void *usr_data);
  int (*start)(DriverContext *ctx);
  int (*stop)(DriverContext *ctx);
  void (*destroy)(DriverContext *ctx);

  // --- Data Functions ---
  int (*poll)(DriverContext *ctx);  // Trigger a read cycle
  int (*write_tag)(DriverContext *ctx, uint32_t tag_id,
                   const cedgex_datapoint_t *val);

} DriverOps;

// The one and only function a .so plugin MUST export
#define DRIVER_ENTRY_POINT "driver_entry_point"
typedef DriverOps *(*DriverEntryPointFunc)(void);

#endif  // DRIVER_API_H
