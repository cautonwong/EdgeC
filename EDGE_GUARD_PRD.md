## Problem Statement

Industrial control systems are vulnerable to malicious or erroneous "write" commands that can bypass traditional IT firewalls. These firewalls often lack deep visibility into OT (Operational Technology) protocols like Modbus, IEC 104, or DLT 698. A single malformed packet or a logically impossible command (e.g., opening a high-pressure valve too quickly) can lead to catastrophic hardware failure or environmental disasters.

## Solution

The EdgeC Protocol Firewall (Edge-Guard) is a security-hardened build of the EdgeC framework. It utilizes the "Master/Slave" protocol stack to perform Deep Packet Inspection (DPI) on every industrial frame. By acting as a transparent proxy, it validates not just the protocol format, but also the *logical sanity* of commands using the `Algorithm Lexicon`. It employs Haar Wavelet Transforms (DWT) to detect high-frequency signal anomalies that might indicate a sophisticated "Stuxnet-style" attack.

## User Stories

1. As a **security analyst**, I want to define "Safety Ranges" for every Modbus register in the YAML contract, so that any write command outside these bounds is automatically blocked and logged.
2. As a **utility plant manager**, I want the gateway to perform real-time DWT analysis on control loop responses, so that I am alerted to subtle "logic-bomb" oscillations that typical alarms might miss.
3. As a **network administrator**, I want a detailed audit log of every successful and blocked command stored in the local `SQLite Sink`, so that I can perform forensic analysis after a security incident.

## Implementation Decisions

- **`module: dpi_validation_engine`**: A new service in the Execution Plane. It intercepts all `handle_rx` responses in Slave mode and `encode_write` requests in Master mode. It cross-references the target address/tag with the semantic contract's `min/max` and `rate_of_change` constraints.
- **`module: anomaly_baseline_stage`**: A StreamEngine stage that calculates the statistical baseline (mean, variance) of normal operations. It flags any command that deviates significantly from the expected physical behavior of the system.
- **`module: secure_syslog_exporter`**: An extension of `edge_log.h` that cryptographically signs security logs and pushes them to a remote SIEM (Security Information and Event Management) system via TLS.

## Testing Decisions

- **Fuzzing & Injection Test (AFK)**: We will use a script to inject millions of semi-valid but logically dangerous industrial frames (e.g., IEC 104 "Select-before-Operate" violations). The test must assert that 100% of the illegal frames are blocked while legitimate telemetry remains unaffected.
