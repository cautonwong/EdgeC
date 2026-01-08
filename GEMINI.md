# EdgeC Project Context & Guidelines (Industrial v2.0)

This document serves as the **highest instructional context** for Gemini AI. It captures the project's architectural essence, identified technical debt, and the long-term governance roadmap. **Last Updated: Jan 2026.**

## 🚀 1. Project Essence
EdgeC is a high-performance, deterministic edge computing SDK for industrial IoT.
- **Standards:** C11, libuv, Go-based Contract Compiler.
- **Architecture:** "Four Planes" decoupling (Core, Main Loop, Stream Engine, IO Executor).
- **Core Principle:** Contract-Driven Development (CDD). Logic must be derived from YAML, never hardcoded.

## 🏗️ 2. Build System: Status & Deep Analysis
Current build system uses CMake with a hybrid logic of static libraries and generated code.

### 🚩 Identified Issues & Technical Debt (Roadmap)
- [x] **Plane Isolation:** (Completed Jan 2026) Transitioned from `OBJECT` to `STATIC` libraries to enforce physical boundaries and detect circular dependencies at link time.
- [ ] **Configuration Race:** Current `EdgeContract.cmake` has a "double-configure" issue where the first `cmake ..` might miss generated macros. Needs a move from Build-Time to Configure-Time execution.
- [ ] **Source Tree Pollution:** Go compiler outputs binaries to `.local/` in the source tree. Should be moved to `${CMAKE_BINARY_DIR}` to ensure build idempotency.
- [ ] **Dependency Management:** Transition from manual `.local/` hacks to modern `FetchContent` or `vcpkg`.

## 🛡️ 3. CI/CD: Scrutiny & Hardening Roadmap
The current GitHub Actions pipeline covers basic build and test but lacks industrial-grade rigour.

### 💡 Divergent Thinking / Future整改项
- **[Contract Verification]** CI should verify YAML schema integrity (TagID overlaps, address conflicts) BEFORE running C tests.
- **[Environment Idempotency]** CI should recompile the `contract-compiler` from source instead of trusting pre-compiled binaries in the repo.
- **[Time-Jump Simulation]** Add test cases simulating NTP clock drift/backwards jumps to ensure scheduler stability.
- **[Frontend Sync]** Automatically run `npm run type-check` in the `web/` directory whenever `tags_type.ts` is regenerated.

## 📝 4. Mandatory Development Conventions
- **Zero Hardcoding:** No Modbus/OBIS codes in C logic. Use `TagID` lookups.
- **Plane Boundaries:** `src/core` must NEVER include headers from `cmd/` or other planes.
- **Libuv Safety:** No blocking I/O in the main loop. All syscalls must be through `libuv`.
- **Zero-Copy Performance:** Prefer `net_buf_t` for cross-plane data transfer.
- **Test Integrity:** Every new driver or stage MUST have a corresponding `cmocka` test.

## 📅 5. Known Issues
- `test_protocol_loopback` currently fails with `uv__finish_close` assertion. **Status: Acknowledged, Skip for now.**