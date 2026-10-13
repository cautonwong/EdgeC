/**
 * @file datahub.h
 * @brief EdgeOS Public SDK DataHub Shadow Bus & RingBuffer Interface
 */

#ifndef EDGEOS_SDK_DATAHUB_H
#define EDGEOS_SDK_DATAHUB_H

typedef struct uv_loop_s uv_loop_t;

#include "../../modules/edge_core/include/datahub.h"
#include "../../modules/edge_infra/include/datahub_ops.h"
#include "edgeos/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef datahub_t edgeos_datahub_t;
typedef datahub_event_cb edgeos_datahub_event_cb;
typedef datahub_sub_filter_t edgeos_datahub_sub_filter_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_SDK_DATAHUB_H */
