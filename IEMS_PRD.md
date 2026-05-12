## Problem Statement

As modern microgrids become more complex, integrating renewable generation, battery storage, and dynamic grid pricing requires more than simple rule-based logic. Grid operators and site managers struggle to maximize economic return while ensuring equipment safety because traditional edge gateways lack advanced mathematical foresight (e.g., Model Predictive Control, non-linear state estimation). Furthermore, while connecting diverse field devices (Modbus, CAN, DLT) to legacy SCADA systems (IEC 104) is critical, maintenance engineers frequently face the challenge of remotely diagnosing these isolated edge devices using proprietary manufacturer software, which requires raw, transparent access to the serial or TCP ports without disrupting the ongoing edge intelligence.

## Solution

The EdgeC Intelligent Energy Management System (IEMS) leverages the highly optimized `Algorithm Lexicon V6.0` to transform the edge gateway into a mathematical brain. It introduces advanced stream-processing stages for Predictive Dispatch (MPC) and Battery State Estimation (EKF). To bridge the IT/OT gap, it adds a modern MQTT Sparkplug B north-bound driver. Crucially, it introduces a dedicated "Transparent Transmission" (Passthrough) subsystem. This allows remote maintenance tools to establish a secure tunnel through the edge device directly to the field endpoints, temporarily suspending or multiplexing with the EdgeC DataHub's polling to ensure safe, raw debugging.

## User Stories

1. As a **microgrid operator**, I want the system to respond to IEC 104 remote control commands (C_SC_NA_1/C_DC_NA_1) from the grid dispatch center, so that I can participate in grid ancillary services (e.g., peak shaving).
2. As a **site owner**, I want the system to utilize Model Predictive Control (MPC) based on upcoming electricity prices and solar forecasts, so that it can autonomously schedule battery charging/discharging to maximize daily profit.
3. As a **battery engineer**, I want the system to run an Extended Kalman Filter (EKF) stage on raw cell voltage and current data, so that I get a highly accurate SOC/SOH estimation that corrects the often-drifting BMS native reports.
4. As a **cloud IT architect**, I want the edge node to publish its telemetry using the MQTT Sparkplug B specification, so that our cloud infrastructure can automatically discover metrics without manual tag mapping.
5. As a **field maintenance technician**, I want to establish a transparent TCP-to-Serial passthrough tunnel to a specific Modbus PCS inverter, so that I can use the manufacturer's proprietary Windows software remotely as if I were plugged in locally.
6. As a **system administrator**, I want the passthrough tunnel to safely interlock with the active polling engine, so that transparent debugging does not cause protocol collisions or bus lockups.

## Implementation Decisions

- **`module: mpc_dispatcher_stage`**: A new stream engine stage. It will subscribe to forecast arrays (prices, solar generation) and current SOC. It will utilize the existing `control_mpc_formulate` and `control_qp_solve_hildreth` mathematical routines to compute a rolling optimal power setpoint for the PCS.
- **`module: ekf_soc_estimator_stage`**: A state-estimation stage utilizing `control_ekf_predict` and `control_ekf_correct`. It will maintain a local equivalent circuit model (ECM) of the battery, updating its state covariance matrix in real-time.
- **`module: sparkplug_b_driver`**: A north-bound protocol implementation conforming to the `edge_driver_t` interface, utilizing the Eclipse Tahu specification over the existing Paho MQTT client.
- **`module: passthrough_manager`**: A new service residing in the Control Plane. It will listen on dedicated TCP ports. When a connection is established, it will signal the `bus_orchestrator` to temporarily pause or interleave normal polling for the targeted physical bus. It will stream raw bytes bidirectionally between the TCP socket and the `io_handle_t` utilizing `libuv` stream endpoints.
- **Architectural Decision**: Mathematical stages will execute within the `StreamEngine` reactive topology, reading from and writing to the `DataHub` shadow table. The `passthrough_manager` operates out-of-band at the lowest IO abstraction layer to ensure raw protocol purity.

## Testing Decisions

- **Closed-Loop Simulation Testing**: We will not rely solely on physical batteries for algorithm validation. We will instantiate a `stage_transfer_function` (using the Z-transform bilinear implementation) configured as an RC-circuit plant model. The MPC stage will control this virtual plant, and we will assert that the plant's output converges to the target setpoint without violating the defined constraints (e.g., maximum charge current).
- **End-to-End Control Testing**: We will mock an IEC 104 Master dispatching a remote control command, trace it through the `DataHub`, observe the MPC re-evaluating the plan, and assert that the correct Modbus FC 16 (Write Multiple Registers) frame is encoded by the slave driver.
- **Passthrough Isolation Testing**: We will mock a continuous polling loop. A test script will initiate a passthrough TCP connection, inject malformed/proprietary bytes, and we will assert that the polling engine correctly yields the bus and resumes smoothly once the passthrough connection drops, without crashing the protocol decoders.

## Out of Scope

- Implementation of IEC 61850 (GOOSE/MMS) for microsecond-level substation protection.
- Development of the cloud-side dashboard or Sparkplug B primary host application.

## Further Notes

- The addition of the passthrough scenario requires careful handling of the `io_executor` locks. The `arbiter` must be aware of "exclusive bus holds" to prevent the polling engine from fighting with the remote engineer's diagnostic software.