#ifndef EDGEOS_H
#define EDGEOS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

/* 1. Base Layer */
#include "edge_base_types.h"
#include "edge_common.h"

/* 2. Generated Contracts (Crucial Order) */
#include "contract_data_types.h"

/* 3. Infrastructure & Planes */
#include "arbiter.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "edge_driver.h"
#include "edge_req.h"
#include "io_executor.h"
#include "net_buf.h"
#include "shmring.h"

/* 4. Logic & Service */
#include "edge_log.h"
#include "edge_params.h"
#include "edge_service.h"
#include "js_engine.h"
#include "northbound_mgr.h"
#include "rest_server.h"
#include "sqlite_service.h"
#include "stream_engine.h"

#ifdef __cplusplus
}
#endif

#endif