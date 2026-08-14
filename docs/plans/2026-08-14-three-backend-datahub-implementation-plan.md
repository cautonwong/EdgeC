# 2026-08-14 Three-Backend DataHub Implementation Plan (SHM + DDS + MQTT)

## Executive Summary
This plan details the full implementation of the Three-Backend DataHub Architecture specified in ADR-0005.

## Step-by-Step Engineering Roadmap

### Phase 1: Core VTable Foundation & Single-Node SHM (Completed)
- `modules/edge_core/include/datahub.h`: VTable definition (`datahub_ops_t`).
- `modules/edge_core/src/datahub.c`: VTable dispatcher.
- `modules/edge_core/src/datahub_shm.c`: POSIX SHM ring buffer backend implementation.

### Phase 2: Cyclone DDS Inter-Process Distributed Backend Implementation
- `modules/edge_core/include/datahub_dds.h`: DDS backend initialization and QoS parameters.
- `modules/edge_core/src/datahub_dds.c`: Cyclone DDS `dds_create_participant`, `dds_create_topic`, `dds_create_writer`, `dds_create_reader` integration.

### Phase 3: Paho MQTT Northbound Cloud Bridge Backend Implementation
- `modules/edge_core/include/datahub_mqtt.h`: MQTT backend interface.
- `modules/edge_core/src/datahub_mqtt.c`: Paho MQTT client topic publishing and payload formatting.

### Phase 4: Multi-Backend Concurrent DataHub Router
- `modules/edge_core/include/datahub_router.h`: DataHub Router manager.
- `modules/edge_core/src/datahub_router.c`: Concurrent write fan-out across SHM, DDS, and MQTT backends.

### Phase 5: Verification & Unit Test Suite
- `tests/test_datahub_multi_backend.c`: Integration test suite verifying multi-backend read/write dispatching.
- Verification command: `./build.sh && ./test.sh`.
