# EdgeC SDK Tiering & Monetization Strategy

EdgeC provides a scalable licensing model designed to meet the diverse needs of industrial manufacturers, robotics companies, and medical device OEMs. Our tiered approach ensures that customers only pay for the computational power and algorithmic depth they require.

## Feature Comparison Matrix

| Feature Module | **Basic Tier** | **Professional Tier** | **Enterprise Tier** |
| :--- | :---: | :---: | :---: |
| **Core Framework** | | | |
| Event-Driven DataHub | ✅ | ✅ | ✅ |
| StreamEngine (Basic Filters) | ✅ | ✅ | ✅ |
| Bus Orchestrator & Arbiter | ✅ | ✅ | ✅ |
| **Industrial Protocols** | | | |
| Modbus RTU/TCP | ✅ | ✅ | ✅ |
| IEC 60870-5-104 / 101 | ✅ | ✅ | ✅ |
| DLMS/COSEM (Smart Grid) | ✅ | ✅ | ✅ |
| MQTT Sparkplug B | ✅ | ✅ | ✅ |
| **Security & Auditing** | | | |
| Edge-Guard DPI Firewall | ❌ | ✅ | ✅ |
| Cryptographically Signed Logs | ❌ | ✅ | ✅ |
| **Advanced Mathematics (Linalg/DSP)** | | | |
| Matrix Operations (GSL) | ❌ | ✅ | ✅ |
| 50/60Hz Mains Notch Filter | ❌ | ✅ | ✅ |
| Haar Wavelet DWT (Anomaly) | ❌ | ✅ | ✅ |
| Extended Kalman Filter (EKF) | ❌ | ✅ | ✅ |
| AHRS Quaternion Solver | ❌ | ✅ | ✅ |
| Predictive Maintenance (RUL) | ❌ | ✅ | ✅ |
| **High-Performance Compute** | | | |
| Field-Oriented Control (FOC) | ❌ | ✅ | ✅ |
| SCARA Inverse Kinematics | ❌ | ✅ | ✅ |
| **App Ecosystem & Execution** | | | |
| Secure WASM Runtime (WAMR) | ❌ | ❌ | ✅ |
| TinyML WASM Inference | ❌ | ❌ | ✅ |
| Industrial App Store Ready | ❌ | ❌ | ✅ |

## Licensing Mechanism
The EdgeC runtime utilizes a C-level macro pruning system (`EDGEC_TIER_BASIC`, `EDGEC_TIER_PROFESSIONAL`, `EDGEC_TIER_ENTERPRISE`). At compile time, the binary is heavily optimized:
- **Basic Tier:** Strips out the `edge_algo` mathematical libraries and the `WAMR` WebAssembly engine, resulting in a microscopic footprint suitable for constrained MCU environments (Cortex-M4).
- **Professional Tier:** Includes all linear algebra and signal processing modules, catering to demanding sensor-fusion and real-time control (Cortex-M7/R5).
- **Enterprise Tier:** Unlocks the full potential of EdgeC, allowing dynamic loading of third-party logic and AI models via the secure WASM sandbox (Cortex-A53/x86_64).

*Note: Runtime licensing is enforced by the `license_engine_init()` module during boot, cryptographically verifying the deployment against the compiled tier.*
