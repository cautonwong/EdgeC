# EdgeC Domain Glossary

## Core Concepts

### EdgeC Runtime
The C11-based edge computing engine that ingests, processes, and forwards industrial data. It is the single binary that, when loaded with a *Semantic Contract*, can act as any industrial edge device (grid concentrator, robot controller, medical monitor, radar processor, etc.).

### Semantic Contract
A YAML document that fully determines the runtime behavior of an EdgeC node. It defines: what devices to poll (Southbound), with which protocol (Modbus / IEC / DLT / DLMS), what mathematical processing to apply (StreamEngine Pipeline), and how to format and forward results (Northbound). A Contract is compiled into C mapping tables and injected into the engine via the *Contract Compiler*.

Synonym family: "契约", "YAML配置". Deprecated: "profile" (too generic).

### Contract Compiler
The toolchain (Go-based) that reads a Semantic Contract (YAML) and generates:
- C header files with tag metadata, protocol mappings, and pipeline topology
- Go binding code for REST API access
The compiler is what makes EdgeC "software-defined" — changing the Contract changes the device's entire behavior without modifying C source.

### DataHub
The zero-copy shared memory message bus at the core of EdgeC. It has two regions:
- **Ring Buffer**: a lock-free circular buffer carrying the live event stream (producer-consumer pattern)
- **Shadow Table**: a indexed snapshot table holding the latest value of every tag (for fast lookup by Go/UI processes)

All inter-process communication (C↔Go↔UI) goes through the DataHub via POSIX shared memory.

### StreamEngine Pipeline
A reactive, directed acyclic graph of processing *Stages* that data flows through. Each Stage is a pure function that reads from the DataHub, transforms the data, and writes back. Pipelines are defined by the Semantic Contract and can be hot-reloaded without restarting the engine.

Not: "Stage chain" (too linear — pipelines are graphs, not chains).

### Stage
A single processing unit in the StreamEngine Pipeline. Categories:
- **DSP Stage**: frequency-domain transforms (FFT, DWT, IIR/FIR filters)
- **Control Stage**: closed-loop controllers (PID, MPC, EKF, LQR)
- **ML Stage**: WASM-based model inference (TinyML)
- **Sink Stage**: persistence (SQLite) or forwarding (MQTT, Sparkplug B)
- **Custom Stage**: user-defined via WASM or QuickJS

### Edge App
A WebAssembly (WASM) binary that runs inside the EdgeC sandbox. Apps are:
- Securely isolated (memory-bounded via WAMR)
- Loaded/unloaded at runtime without restarts
- Sold/distributed through the *Algorithm Marketplace*
- Triggered by DataHub tag events

Not: "WASM App" (redundant — all Edge Apps are WASM). Not: "插件" (ambiguous).

---

## Data Flow Direction

### Southbound
Data flowing **into** EdgeC from field devices: meter readings, sensor ADC values, motor encoder positions, radar chirps. Uses Master-mode protocol drivers to poll or Slave-mode to receive.

### Northbound
Data flowing **out** of EdgeC to upstream systems: MQTT/Sparkplug B to cloud, IEC 104 to SCADA, HL7 FHIR to hospital systems, encrypted syslog to SIEM. Uses Northbound protocol drivers.

### Passthrough
An out-of-band data path that temporarily bypasses the DataHub and StreamEngine, providing raw byte-level access from a remote TCP client to a Southbound serial/RTU device. Used exclusively for field diagnostics via manufacturer proprietary software.

---

## Device Roles

### Master
The active party in a Southbound protocol session — initiates requests, manages polling schedules, processes responses. Example: a DCU acting as DLT698 Master polling 1000 meters.

### Slave
The passive party in a Southbound protocol session — waits for requests, sends responses on demand. Also the active role in Northbound protocol sessions (the edge node acts as IEC 104 Slave to the SCADA Master).

### Bus
A logical communication channel representing one physical interface (UART, TCP socket, CAN bus). Each Bus may carry one or more protocol sessions and is managed by the *Bus Orchestrator*.

---

## Product Lines

### Grid-Core
A product build of EdgeC targeting smart grid terminals (DCU, TTU, FTU). Characterized by massive Southbound polling (1000+ meters) and protocol bridging (DLT/IEC mapping).

### Aero-Edge
A product build targeting aerospace/avionics (satellites, UAVs). Builds on Zephyr RTOS for microsecond-level determinism. Uses CCSDS space protocol for telemetry.

### Robot-Ctrl
A product build targeting multi-axis robot arms. Features Field-Oriented Control (FOC) at 10kHz+ and Denavit-Hartenberg inverse kinematics.

### Med-Ctrl
A product build targeting medical diagnostic equipment (ECG, EEG). Features clinical-grade DSP (notch filters for mains hum removal, wavelet-based arrhythmia detection) and HL7 FHIR export.

### Vibra-Sense
A product build for predictive maintenance of rotating machinery. Converts 20kHz vibration ADC data into Remaining Useful Life (RUL) estimates via EKF-based wear tracking.

### Radar-Edge
A product build for FMCW radar signal processing. Generates Range-Doppler maps via 2D FFT and extracts targets via CFAR detection.

### IoT-Hub
A product build for massive brownfield device connectivity. Emphasizes Store-and-Forward resilience for intermittent networks and payload compaction for bandwidth efficiency.

### Process-Ctrl
A product build for process automation (water treatment, HVAC, chemical). Features advanced PID with anti-windup and SQLite-based edge historian for ESG compliance.

### VPP-Agent
A product build for Virtual Power Plant optimization. Uses MPC dispatcher to schedule battery charge/discharge against dynamic price forecasts.

### Edge-Guard
A product build functioning as an OT protocol firewall. Performs Deep Packet Inspection on industrial frames and DWT-based anomaly detection for Stuxnet-style attacks.

### Edge-Sim
A product build acting as a Digital Twin simulator. Runs physics models in WASM and exposes OpenAI Gym interface for Reinforcement Learning training.

### Nova HMI
The EdgeC local web dashboard. "Cyberspace Technical" aesthetic, Canvas/WebGL rendering for high-frequency data visualization. Served directly by the embedded web server.

### Edge-Ops
The centralized cloud control plane for fleet management. Provides device monitoring, remote Contract deployment, WASM app OTA, certificate rotation, and fleet-wide health dashboards.

---

## Licensing & Tiers

### Basic Tier
MCU-targeted build (Cortex-M4). DataHub + StreamEngine (basic filters only) + Southbound/Northbound protocol stack. No algorithmic math library. No WASM.

### Professional Tier
Application processor build (Cortex-M7/R5/A). Basic + full `edge_algo` library (linalg, DSP, control theory). Includes Edge-Guard DPI.

### Enterprise Tier
Full build (Cortex-A53/x86_64). Professional + WASM sandbox + Edge App lifecycle management + TinyML inference.

---

## Ecosystem Roles

### Algorithm Marketplace
Planned online store where third-party developers publish WASM-compiled Edge Apps. EdgeC platform takes a revenue share on app sales. Comparable to the "App Store model" for industrial edge.

### Edge App Developer
Third-party developer who writes signal-processing or analytics logic in C/Rust, compiles to WASM, and distributes via the Algorithm Marketplace. Distinction: NOT the same as a "core platform developer" (who works on the C engine itself).

### Contract Author
A system integrator or field engineer who writes Semantic Contracts (YAML) to configure EdgeC devices for specific sites. They do not write C code — they compose protocol mappings, pipeline stages, and alert rules.

### Fleet Operator
The entity managing a large number of EdgeC nodes. Uses Edge-Ops cloud console for monitoring, updates, and certificate management. The target persona for the Edge-Ops product.
