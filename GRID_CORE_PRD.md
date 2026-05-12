## Problem Statement

Equipment manufacturers in the smart grid sector face a software maintenance nightmare. They must produce a vast array of specialized edge terminals: Data Concentrator Units (DCU/集中器) for reading thousands of meters, Transformer Terminal Units (TTU/配变终端) for power quality monitoring, Feeder Terminal Units (FTU/配电终端) for fault isolation, and modern Converged Terminals (智能融合终端). Historically, each device type has its own monolithic, tightly-coupled C codebase. When a provincial power grid updates a specification (e.g., a new DLT698 object or a new IEC104 security requirement), manufacturers spend months updating and re-certifying dozens of different firmware builds. Furthermore, modern grids require these devices to run dynamic "Edge Apps" (like line-loss calculation or topology identification), which legacy firmware architectures simply cannot support without full OTA updates.

## Solution

The EdgeC Universal Grid-Core Platform (Grid-Core) provides a single, unified, high-performance C11 binary that can act as *any* grid terminal. The hardware behavior is entirely dictated by the Go-compiled Semantic Contract (YAML). 
- **Deploy a DCU contract**: It instantiates a massive DLT645/698 Master polling engine mapped to an IEC104 Slave uplink.
- **Deploy a TTU contract**: It activates high-speed ADC ingestion, DSP filtering, and IEC101 Master/Slave bridging.
- **Deploy a Smart Converged Terminal contract**: It provisions sandboxed WebAssembly (WASM) execution environments to securely host third-party grid analytics apps alongside critical control loops, mediated by the zero-copy DataHub.

## User Stories

1. As a **meter manufacturer R&D director**, I want a single EdgeC software core that can be configured via YAML to behave as either a Concentrator (DCU) or a Transformer Terminal (TTU), so that I can cut my software R&D team's maintenance burden by 80%.
2. As a **DCU field gateway**, I must dynamically read a list of 1,000 sub-meters (RS485/HPLC) from a local SQLite table and asynchronously poll them via DLT698, so that I can aggregate massive residential power data without blocking the uplink connection.
3. As a **Grid Dispatch Center SCADA system**, I want to issue a standard IEC104 Total Interrogation (GI) command, and have the EdgeC terminal automatically map and return all connected DLT/Modbus meter readings seamlessly, without custom C code translating the addresses.
4. As a **Provincial Grid App Developer**, I want to compile my proprietary "Low-Voltage Topology Identification" algorithm into a WASM binary, so that it can be dynamically deployed to the EdgeC terminal and securely read voltage sag events from the DataHub without crashing the device's core protection logic.

## Implementation Decisions

- **`module: dcu_dynamic_poller`**: A specialized orchestrator extension for DCU scenarios. Unlike static Modbus maps, it must dynamically spawn `edge_req_t` tasks based on an evolving fleet of meters stored in the `SQLite` persistence layer. It manages polling intervals, retry logic, and concurrent RS485/PLC channel utilization.
- **`module: automated_protocol_bridge`**: A fast-path routing mechanism within the Control Plane. It utilizes the Semantic Compiler's `meta_list` injections to create a bidirectional mapping dictionary (e.g., Southbound DLT698 OAD `20000200` automatically maps to Northbound IEC104 IOA `1001`). When the DataHub shadow updates, it automatically triggers the Spontaneous (`IEC_COT_SPONT`) upload logic we recently perfected.
- **`module: secure_grid_app_container` (WASM)**: An enhancement of `stage_wasm.c`. It provides strict memory bounding (`k_mem_slab` quotas) and execution time limits to ensure third-party grid apps cannot starve the core `bus_orchestrator` thread, ensuring grid protection functions (like FTU fault isolation) maintain sub-millisecond priority.

## Testing Decisions

- **Massive Concurrency Simulation (AFK)**: We will simulate 1,000 virtual DLT645 meters attached to virtual serial interfaces. We will assert that the `dcu_dynamic_poller` can successfully execute a daily reading cycle for all 1,000 meters within a 15-minute window, while simultaneously maintaining an active IEC104 heartbeat with the master station at < 50ms latency.
- **Protocol Bridge Verification (AFK)**: We will inject a DLT698 response payload into the Southbound interface. The test must assert that the exact corresponding value appears in a correctly formatted IEC104 Spontaneous report on the Northbound interface, proving the compiler's mapping logic.

## Out of Scope

- Developing the proprietary hardware (HPLC modules, RF mesh chips). EdgeC will interface with these via standard Linux TTY/UART/SPI character devices.
- Developing the actual business logic for Line-Loss calculation; EdgeC only provides the WASM execution environment and data provision.