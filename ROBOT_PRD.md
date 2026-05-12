## Problem Statement

Traditional multi-axis robot arms and high-speed servos rely on dedicated, expensive, and tightly-coupled hardware PLCs (e.g., Beckhoff, Beckhoff TwinCAT). As industrial manufacturers seek to deploy "software-defined manufacturing," they lack an edge operating system capable of executing high-frequency (e.g., 10kHz) Field-Oriented Control (FOC) loops while simultaneously computing complex inverse kinematics and performing predictive maintenance directly at the machine's edge.

## Solution

The EdgeC Robot Controller (Robot-Ctrl) product will instantiate the `StreamEngine` in an ultra-low-latency real-time mode. It will utilize the `Algorithm Lexicon`'s 4x4 Homogeneous Matrix capabilities to compute Forward and Inverse Kinematics (FK/IK) on the fly. It will feature a dedicated FOC motor control pipeline executing sub-millisecond Clark/Park transforms. Furthermore, it will run an anomaly detection stage using Haar Wavelet Transforms (DWT) on motor vibration/current signatures to predict mechanical failures (e.g., bearing faults) before they stop the production line.

## User Stories

1. As a **motion engineer**, I want to provide target XYZ coordinates to the EdgeC API, so that the system can automatically compute the required joint angles using Inverse Kinematics (IK) matrices.
2. As a **motor drive developer**, I want the system to execute a 10kHz Field-Oriented Control (FOC) loop utilizing the `control_foc` math routines, so that it can drive a brushless DC motor with high torque and precision.
3. As a **maintenance supervisor**, I want the EdgeC node to continuously run a Discrete Wavelet Transform (DWT) anomaly detector on motor current harmonics, so that I receive an immediate alarm when a high-frequency transient indicates imminent mechanical wear.
4. As an **automation integrator**, I want the controller to support the CANopen CiA 402 drive profile over the existing CAN driver, so that I can seamlessly connect standard industrial servo drives to the EdgeC network.

## Implementation Decisions

- **`module: kinematics_solver_stage`**: A stream engine stage that intercepts desired end-effector coordinates. It will build and multiply 4x4 Denavit-Hartenberg (DH) homogeneous transformation matrices to output an array of target joint angles.
- **`module: high_speed_foc_pipeline`**: A dedicated, isolated pipeline within the `StreamEngine`. It will bypass the standard DataHub event loop, reading raw ADC values via `req_raw_io` and directly calculating Clark/Park transforms and SVPWM duty cycles to minimize latency.
- **`module: wavelet_anomaly_stage`**: A condition monitoring stage that accumulates a buffer of fast-sampled current data, applies `dsp_dwt_haar`, extracts the high-frequency detail coefficients, and triggers a `PIPEMSG_TYPE_EVENT` alarm if the energy in the detail band exceeds a learned threshold.
- **`module: canopen_cia402_driver`**: Extending the current `driver_can.c` to parse standard CiA 402 objects (Controlword, Statusword, Target Position). It will operate in Slave mode to receive commands from a higher-level MES, and Master mode to dispatch commands to downstream servos.

## Testing Decisions

- **Math Validation (AFK)**: We will write a test that inputs a known end-effector position and verifies that the output joint angles match pre-calculated analytical solutions to within 0.001 degrees.
- **DWT Synthetic Test (AFK)**: We will inject a synthetic sine wave embedded with a 10-sample high-amplitude transient. The test must assert that the `wavelet_anomaly_stage` correctly detects the transient and fires an event, while ignoring smooth sine wave variations.
- **FOC Latency Benchmark**: A performance test measuring the end-to-end execution time of the `high_speed_foc_pipeline`. It must complete its calculations in under 50 microseconds to guarantee a 10kHz control loop.

## Out of Scope

- Vision-based robotic path planning.
- Multi-robot fleet swarm coordination (this belongs in a higher-tier Orchestrator, not the edge node).