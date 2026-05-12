## Problem Statement

In process automation industries such as water treatment, HVAC (Heating, Ventilation, and Air Conditioning), and chemical manufacturing, legacy PLCs are the norm. These environments deal with noisy analog sensors (liquid levels, pressures, temperatures) and require robust, continuous control loops. However, upgrading these facilities to meet modern ESG (Environmental, Social, and Governance) and carbon auditing requirements is difficult. Legacy PLCs lack the memory to buffer data during network outages, struggle with advanced filtering to prevent actuator wear, and cannot natively interface with modern REST/MQTT APIs for carbon reporting.

## Solution

The EdgeC Process Controller (Process-Ctrl) reimagines the traditional PLC. It utilizes the `Algorithm Lexicon` to apply industrial-grade DSP filters (IIR/FIR) to raw sensor data, drastically reducing noise before it reaches the control algorithms. It employs Advanced PID loops with Anti-Windup to smoothly control pumps and valves. Crucially, it leverages the `SQLite Sink` to act as an edge historian, persistently logging operational data and carbon metrics locally. This ensures zero data loss during network disruptions and provides an immutable audit trail for compliance.

## User Stories

1. As a **process engineer**, I want the controller to apply a configurable IIR low-pass filter to the raw water level sensor data, so that waves and splashing do not cause the pumps to constantly toggle on and off.
2. As a **control technician**, I want to use an Advanced PID controller with built-in Anti-Windup, so that my large HVAC valves do not overshoot their setpoints after a long period of saturation.
3. As a **facility manager**, I want the edge controller to locally log all flow meter and energy consumption data into an embedded SQLite database, so that I have a reliable, offline-capable audit trail for my monthly carbon footprint reports.
4. As an **IT auditor**, I want the system to employ Report-By-Exception (RBE) logic, so that historical data is only uploaded to the central server when significant changes occur, saving cellular data costs.

## Implementation Decisions

- **`module: dsp_filter_stage`**: Integrating the `stage_iir_filter.c` from the standard stages. It will be configurable via the semantic contract to apply specific cutoff frequencies to noisy Modbus/Analog inputs.
- **`module: advanced_pid_stage`**: A new stream stage wrapping `control_pid_compute`. It reads a process variable (e.g., filtered temperature) from the DataHub, computes the PID output, and routes the control signal to a physical actuator via a protocol driver (e.g., Modbus FC06).
- **`module: sqlite_historian_stage`**: Utilizing the existing `stage_sqlite_sink_create_v2`. It will batch-insert high-value telemetry and calculated carbon metrics into the local `.db` file for persistence.
- **`module: rbe_telemetry_stage`**: Leveraging `stage_rbe_create_v2` to filter out dead-band noise before data is pushed to the MQTT northbound driver.

## Testing Decisions

- **Filter Efficacy Test (AFK)**: Inject a noisy sine wave (simulating a turbulent tank level) into the `dsp_filter_stage`. Assert that the variance of the output signal is reduced by at least 80% without introducing unacceptable phase delay.
- **PID Anti-Windup Validation (AFK)**: Simulate a blocked valve scenario where the error remains high for a prolonged period. Release the blockage and assert that the PID output recovers smoothly without excessive overshoot, proving the integration of the integral clamping logic.
- **Historian Resilience Test (AFK)**: Force an artificial network disconnect. Verify that the `sqlite_historian_stage` continues to log data locally and that the data can be successfully queried once normal operation resumes.

## Out of Scope

- Cloud-side analytics for carbon trading. EdgeC will only provide the immutable raw data.
- Ladder Logic (IEC 61131-3) support. Control strategies will be defined using the Reactive Stream Topology, not traditional PLC ladder diagrams.