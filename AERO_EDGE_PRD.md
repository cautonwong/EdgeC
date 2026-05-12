## Problem Statement

The commercial space industry (NewSpace) and advanced UAV (drone) sectors are constrained by strict SWaP (Size, Weight, and Power) budgets. Nanosatellites and autonomous drones require complex Attitude and Heading Reference Systems (AHRS), orbital propagators, and high-speed sensor fusion (Star Trackers, IMUs, Sun Sensors). Running heavyweight Linux operating systems or Python-based AI on these edge devices consumes too much power and introduces unpredictable latency. Traditional aerospace microcontrollers lack the modern, dynamic pipeline capabilities to easily update mission control logic over-the-air.

## Solution

The EdgeC Aerospace & Avionics Node (Aero-Edge) is an ultra-lean, deterministic build of the EdgeC framework targeting real-time operating systems (specifically Zephyr RTOS). By discarding the heavy web servers and relying purely on the `StreamEngine` and `Algorithm Lexicon`, Aero-Edge acts as a mathematically rigorous, zero-allocation avionics bus. It utilizes the Extended Kalman Filter (EKF) for 6-DOF (Degrees of Freedom) sensor fusion and leverages the C11 foundation to guarantee microsecond-level deterministic execution of flight control pipelines. 

## User Stories

1. As an **aerospace flight software engineer**, I want to compile the EdgeC engine directly onto a Zephyr-supported ARM Cortex-M7 radiation-hardened MCU, so that I have a modern, reactive pipeline architecture running in a bare-metal environment.
2. As an **attitude control specialist**, I want to utilize the `control_ekf_predict/correct` stages to fuse high-rate gyroscope data with low-rate star tracker updates, so that the satellite maintains precise pointing accuracy.
3. As a **mission control operator**, I want to update the semantic YAML contract of the satellite while it is in orbit, so that I can reconfigure telemetry aggregation logic (e.g., modifying RBE thresholds to save downlink bandwidth) without flashing a completely new firmware binary.
4. As a **systems architect**, I want to utilize the zero-copy `DataHub` as the primary avionics message bus, ensuring that critical flight control loops are physically isolated from lower-priority payload processing via Zephyr thread priorities.

## Implementation Decisions

- **`module: zephyr_hal_integration`**: A deep adaptation layer ensuring all memory allocations in the `DataHub` and `StreamEngine` are initialized statically at boot time. Removal of `libuv` dependencies in favor of native Zephyr threads and Kconfig primitives.
- **`module: ahrs_quaternion_stage`**: A new stream stage extending the `edge_linalg` library to support Quaternion mathematics. This stage will compute spatial rotations without the risk of Gimbal Lock, essential for orbital mechanics.
- **`module: ccsds_telemetry_driver`**: A protocol driver implementing the Consultative Committee for Space Data Systems (CCSDS) space packet protocol, operating on top of a raw RF serial/SPI interface for ground station downlinks.

## Testing Decisions

- **Hardware-in-the-Loop (HIL) Simulation**: Edge-Sim will be used on a host Linux machine to simulate the orbital physics and inject IMU/Star Tracker data via UART into a physical development board running Aero-Edge on Zephyr.
- **Deterministic Latency Audit**: Using Zephyr's tracing tools, we will assert that the time from IMU interrupt triggered -> EKF fusion -> actuator command dispatched does not exhibit jitter greater than 10 microseconds over a 24-hour test period.

## Out of Scope

- Designing the actual RF payload hardware or baseband signal processing.
- Full autonomous orbital rendezvous algorithms (this would reside in a specialized mission-specific WASM payload).