## Problem Statement

Large-scale manufacturing facilities and older commercial buildings contain thousands of fragmented sensors communicating over legacy protocols (Modbus, IEC101, CAN). Connecting this "brownfield" equipment to modern cloud IT systems (AWS, Azure) is a massive undertaking. Network connections in these environments are often intermittent. If the edge gateway loses connection to the cloud, critical operational data is lost forever. Furthermore, mapping thousands of cryptic Modbus register addresses to meaningful cloud tags is incredibly error-prone and time-consuming.

## Solution

The EdgeC Massive IoT Hub (IoT-Hub) is a protocol juggernaut designed for brownfield digitization. It leverages the deeply polished Master/Slave driver framework to seamlessly speak Modbus, IEC, and DLT simultaneously. It solves the network reliability issue by using the `SQLite Sink` as a heavy-duty Store-and-Forward historian, utilizing backpressure to prevent memory exhaustion during long cloud outages. It solves the tag mapping nightmare via the Go-based Contract Compiler, which injects semantic `meta_list` tables directly into the C engine, transforming raw addresses into unified Sparkplug B payloads automatically.

## User Stories

1. As a **facility IT manager**, I want the edge gateway to buffer telemetry data in a local SQLite database when the internet goes down, and automatically flush the backlog to the cloud when connectivity is restored, so that I never lose a single data point for my ESG reports.
2. As a **SCADA engineer**, I want to define my equipment models in a clean YAML semantic contract and let the compiler generate the C mapping tables, so that I don't have to manually write thousands of `if/else` statements for Modbus addresses.
3. As a **cloud developer**, I want the gateway to bundle hundreds of sensor readings into a single compressed binary payload or a structured Sparkplug B message, so that my cloud ingestion costs are minimized.

## Implementation Decisions

- **`module: store_and_forward_manager`**: An extension of the `persistence_mgr.c`. It will monitor the MQTT connection status. When offline, it redirects the DataHub event stream into the `sqlite_svc_t`. When online, it spawns a low-priority background thread to sequentially read the SQLite backlog, publish it via MQTT, and delete the acknowledged rows.
- **`module: payload_compactor_stage`**: A stream stage that aggregates incoming scalar values based on their contract metadata. Once a time window expires or a size limit is reached, it compresses the array (e.g., using zlib or a simple delta-encoding scheme) before handing it to the Northbound driver.

## Testing Decisions

- **Store-and-Forward Resilience Test (AFK)**: We will simulate a 10,000 tag/second load. Midway through, the mock MQTT server will disconnect for 60 seconds. We will assert that the `store_and_forward_manager` successfully buffers all 600,000 messages in SQLite without an OOM crash, and successfully flushes them upon reconnection.

## Out of Scope
- Direct integration with proprietary cloud SDKs (e.g., AWS IoT Greengrass Core or Azure IoT Edge daemon). EdgeC will remain a pure, agnostic MQTT/Sparkplug client.