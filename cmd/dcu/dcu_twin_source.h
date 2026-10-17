#ifndef DCU_TWIN_SOURCE_H
#define DCU_TWIN_SOURCE_H

#include <stdint.h>

#include "dcu_poller.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double base_voltage;
    double base_current;
} dcu_twin_source_t;

int dcu_twin_source_init(dcu_twin_source_t *source, double base_voltage, double base_current);
int dcu_twin_source_read(void *ctx, uint32_t meter_idx, uint32_t tag_id, double *out_val);
poller_source_ops_t dcu_twin_source_get_ops(dcu_twin_source_t *source);

#ifdef __cplusplus
}
#endif

#endif /* DCU_TWIN_SOURCE_H */
