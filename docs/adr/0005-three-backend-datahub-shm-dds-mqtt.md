# ADR-0005: Three-Backend Concurrent DataHub Architecture (SHM + DDS + MQTT)

* Status: Accepted
* Date: 2026-08-14
* Authors: EdgeOS Core Architecture Team

## 1. Context and Problem Statement

EdgeOS requires unified data abstraction across three distinct physical topology domains:
1. **Intra-Process (In-proc 0-Copy)**: High-speed algorithm execution (AHRS, DSP, Wasm) requiring sub-microsecond latency over POSIX Shared Memory (SHM).
2. **Inter-Process / Distributed Mesh**: Node-to-node telemetry and peer discovery across microservices via OMG DDS (Eclipse Cyclone DDS).
3. **Northbound Cloud Bridge**: Telemetry ingestion and remote command dispatch to IoT cloud platforms via MQTT (Paho MQTT).

Previously, DataHub only implemented a single-node POSIX SHM ring buffer. We need a dynamic `datahub_ops_t` VTable architecture that concurrently routes writes and reads across SHM, DDS, and MQTT backends.

## 2. Architecture & Design

```
+-------------------------------------------------------------------+
|                   DataHub Unified Client API                      |
| (datahub_write_tag, datahub_read_tag, datahub_subscribe_tag)      |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
|               DataHub Multi-Backend Router                        |
|                     (datahub_ops_t VTable)                        |
+-------------------+--------------------+--------------------------+
                    |                    |
                    v                    v
          +------------------+  +------------------+  +------------------+
          | SHM Backend      |  | DDS Backend      |  | MQTT Backend     |
          | (POSIX shm_open) |  | (Cyclone DDS)    |  | (Paho MQTT)      |
          +------------------+  +------------------+  +------------------+
          | In-Proc / Local  |  | Distributed Mesh |  | Northbound Cloud |
          +------------------+  +------------------+  +------------------+
```

### 2.1 Backend Isolation Rules
- **SHM Backend**: Manages shared memory ring buffer (`datahub_shm.c`).
- **DDS Backend**: Manages DDS Topic `EdgeOSTagData` (`datahub_dds.c`) using Cyclone DDS `dds_create_topic`, `dds_create_writer`, `dds_create_reader`.
- **MQTT Backend**: Manages MQTT publish/subscribe to `edgeos/datahub/tags/{tag_id}` (`datahub_mqtt.c`) using Paho MQTT.
- **Router**: Dispatches writes concurrently to active backends based on routing policies (`datahub_router.c`).

## 3. Implementation Phasing

1. **Phase 1**: Refactor `datahub.h` / `datahub.c` into `datahub_ops_t` VTable structure. (Done)
2. **Phase 2**: Implement `datahub_dds.c` Cyclone DDS Backend.
3. **Phase 3**: Implement `datahub_mqtt.c` Paho MQTT Backend.
4. **Phase 4**: Implement `datahub_router.c` Multi-Backend Concurrent Router.
5. **Phase 5**: Implement unit test suite `test_datahub_multi_backend.c` and verify 100% test suite pass.

## 4. Status and Consequences

- Ensures 100% backward compatibility for single-node SHM operations.
- Provides native OMG DDS interoperability with ROS 2 and industrial edge gateways.
- Enables direct cloud telemetry streaming without extra middleware.
