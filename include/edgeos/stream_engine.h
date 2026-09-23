/**
 * @file stream_engine.h
 * @brief EdgeOS Public SDK Declarative Stream DAG Engine Interface (ARCH-DEEP-04)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#ifndef EDGEOS_SDK_STREAM_ENGINE_H
#define EDGEOS_SDK_STREAM_ENGINE_H

#include "../../modules/edge_stream/include/stream_engine.h"
#include "edgeos/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef stream_engine_t edgeos_stream_engine_t;
typedef pipeline_t edgeos_pipeline_t;
typedef pipeline_stage_t edgeos_pipeline_stage_t;
typedef pipeline_loader_ctx_t edgeos_pipeline_loader_ctx_t;
typedef stage_factory_create_fn edgeos_stage_factory_create_fn;

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_SDK_STREAM_ENGINE_H */
