# EdgeC Development Roadmap & Backlog

This document translates the Deep Analysis Report into an actionable sprint-based execution plan.

## 🟢 Sprint 1: Foundation Hardening & Stability (Core Engine)
**Goal:** Eliminate the remaining P0/P1 architectural risks in the core engine.

- [x] **C1: DataHub Memory Barriers** - Fixed (C11 Atomic Acquire/Release).
- [x] **C3: ShmRing Integrity** - Implement drop counters and overflow notifications to prevent silent data loss.
- [x] **C4: Arbiter Deadlock Prevention** - Refactor `on_start` callback execution to be outside the mutex lock.
- [x] **Logging Centralization** - Redirect `LOG_INFO/ERROR` macros to use the `edge_log_t` instance system instead of direct `printf`.

## 🟡 Sprint 2: Control & DSP Maturity (Math Plane)
**Goal:** Complete the promised algorithm suite and unify the Stage factory architecture.

- [x] **FIR Filter Implementation** - Completed (Circular Buffer).
- [x] **FFT Pure C Fallback** - Completed (Radix-2 Cooley-Tukey).
- [x] **Linear Kalman Filter** - Implement the standard Kalman filter (EKF is already present).
- [x] **FOC Algorithm Suite** - Implement Clark/Park/SVPWM (Essential for the Robot/Motor market).
- [x] **Factory Unification** - Deprecate V1 factories; migrate all stages to a schema-aware V2 factory system.

---
*Status: Sprint 2 Completed. Entering Sprint 3.*

## 🔴 Sprint 3: Protocol Safety & Compliance (Execution Plane)
**Goal:** Ensure the protocol stacks are production-ready and power-grid compliant.

- [x] **C2: SQL Injection Mitigation** - Introduce a table name whitelist or parameter binding wrapper for the SQLite Sink.
- [x] **IEC 101/104 Fuzzing & Security Tests** - Write unit tests for malformed packets and sequence number attacks.
- [x] **DLMS/COSEM Validation** - Implement test cases for security handshake and object access violations.
- [x] **C5: Driver Type Safety** - Fix UB risks where `void*` is cast to protocol-specific drivers without validation.

---
*Status: Sprint 3 Completed*
