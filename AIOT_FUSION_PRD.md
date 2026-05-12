## Problem Statement

In smart city infrastructure, autonomous logistics, and V2X (Vehicle-to-Everything) scenarios, edge gateways are bombarded with heterogeneous data: high-frequency IMU telemetry via CAN, GPS coordinates via serial, and metadata from smart cameras. Traditional gateways merely route this data to the cloud, resulting in massive bandwidth costs, unacceptable latency for critical safety decisions, and poor positional accuracy when GPS signals degrade (e.g., in tunnels).

## Solution

The EdgeC AIoT Sensor Fusion Gateway (AIoT-Fusion) leverages the `StreamEngine` and `Algorithm Lexicon` to perform deep sensor fusion and edge inferencing natively. By utilizing the Extended Kalman Filter (EKF) to fuse IMU and GPS data, it maintains high-precision positioning even during signal loss. Furthermore, it activates the embedded WebAssembly (WAMR) runtime within the pipeline to execute lightweight Machine Learning (TinyML) models directly on the sensor streams, sending only high-value insights (e.g., "collision imminent" or "asset degraded") to the cloud via Sparkplug B.

## User Stories

1. As a **fleet manager**, I want the gateway to fuse raw CAN-bus IMU data with GPS using an Extended Kalman Filter, so that I get accurate vehicle tracking even when traversing tunnels or urban canyons.
2. As an **AI engineer**, I want to deploy pre-trained TinyML models compiled to WebAssembly (WASM) as standard `StreamEngine` stages, so that I can run inference on high-frequency sensor data locally without writing C code.
3. As a **telecom provider**, I want the gateway to perform local data aggregation and only upload data via MQTT when the AI model detects an anomaly, so that cellular bandwidth costs are drastically reduced.
4. As a **system architect**, I want vision metadata (e.g., object bounding boxes) and physical telemetry (e.g., lidar distance) to be synchronized and correlated within the `DataHub` using the cross-correlation (`dsp_correlate`) algorithms.

## Implementation Decisions

- **`module: sensor_fusion_ekf_stage`**: A specialized stream stage wrapping `control_ekf_predict` and `control_ekf_correct`. It subscribes to high-frequency IMU tags (accelerometer, gyroscope) for the prediction step and low-frequency GPS tags for the correction step.
- **`module: tiny_ml_wasm_stage`**: An enhancement of the existing `stage_wasm.c`. It loads a WASM binary containing a lightweight neural network (e.g., compiled via TensorFlow Lite for Microcontrollers). It reads a window of sensor data from the DataHub, executes the WASM inference function, and writes the classification result back to a new tag.
- **`module: data_sync_correlator`**: A stage utilizing `dsp_correlate` to align delayed sensor streams by finding the maximum cross-correlation peak, ensuring the AI model receives temporally aligned multi-modal data.

## Testing Decisions

- **Sensor Fusion Simulation (AFK)**: We will feed recorded (mocked) IMU and GPS datasets into the pipeline. The test will simulate a GPS outage (dropping GPS updates for 10 seconds) and assert that the EKF stage continues to output a bounded, accurate trajectory.
- **WASM Inference Benchmark (AFK)**: Load a dummy WASM module that performs a matrix multiplication. Benchmark the invocation overhead from the C StreamEngine into the WAMR runtime to ensure it meets real-time constraints (< 1ms per inference).

## Out of Scope

- Processing raw video streams (pixels) within EdgeC. The gateway will only process metadata (bounding boxes, classifications) extracted by dedicated smart cameras.

## Further Notes
- WASM memory management must be strictly sandboxed to prevent a faulty AI model from crashing the core EdgeC operating system.