## Problem Statement

As the number of EdgeC deployments scales from dozens to tens of thousands across entire provinces or industrial parks, manual management becomes impossible. Fleet owners need a way to monitor the real-time health of mathematical pipelines (e.g., EKF divergence, MPC solver timeouts), deploy WebAssembly "Edge Apps" securely at scale, and update semantic contracts (YAML) without physical site visits.

## Solution

Edge-Ops is the centralized control plane for the EdgeC ecosystem. It consists of a high-performance backend that communicates with EdgeC nodes via a bi-directional, secure MQTT/WebSocket tunnel. It provides a "Single Pane of Glass" for device health, a "WASM Orchestrator" for remote app deployment, and an "Automatic Provisioning" system that generates C mapping tables from YAML contracts in the cloud and pushes them to the edge nodes.

## User Stories

1. As a **fleet administrator**, I want to see a map-based dashboard showing the status of all 10,000 Grid-Core terminals, so that I can dispatch repair crews only when an anomaly detector fires.
2. As a **software-defined manufacturer**, I want to push a new version of a "Vibration Analysis" WASM app to all Robot-Boxes in the field simultaneously, so that my customers get the latest features instantly.
3. As a **security officer**, I want to rotate the TLS certificates of all EdgeC nodes remotely, so that the network remains secure against compromised credentials.

## Implementation Decisions

- **`module: edge_agent_connector`**: A lightweight thread within EdgeC that maintains a persistent "Management Tunnel" back to Edge-Ops, separate from the primary data telemetry stream.
- **`module: cloud_contract_compiler`**: Moving the Go-based Contract Compiler to the cloud, allowing users to edit YAML in a web UI and receive a "Binary Contract Blob" that EdgeC can load dynamically at runtime.
- **`module: fleet_ota_manager`**: A secure update handler that utilizes A/B partitioning on the edge device to safely flash core C binaries or individual WASM applets.

## Testing Decisions

- **Massive Connection Test (AFK)**: We will simulate 5,000 concurrent EdgeC nodes connecting to a single Edge-Ops instance. We will assert that the cloud backend can broadcast an updated YAML contract to all nodes within 10 seconds without dropping connections.
