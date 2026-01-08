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
- [x] **Link Order Principle:** (Enforced Jan 2026) Static linking must follow physical hierarchy: `High-level (Main Loop) -> Mid-level (Engine/Executor) -> Base (Core/Protocols)`. Higher-level libs must appear first in `target_link_libraries`.
- [ ] **Configuration Race:** Current `EdgeContract.cmake` has a "double-configure" issue where the first `cmake ..` might miss generated macros. Needs a move from Build-Time to Configure-Time execution.
...
## 📝 4. Mandatory Development Conventions
- **Zero Hardcoding:** No Modbus/OBIS codes in C logic. Use `TagID` lookups.
- **Protocol Decoupling:** Codec kernels MUST reside in `src/protocols`, remain **zero-dependency** (standard C + `net_buf` only), and be independent of the IO framework or contracts.
- **Plane Boundaries:** `src/core` must NEVER include headers from `cmd/` or other planes.
- **Libuv Safety:** No blocking I/O in the main loop. All syscalls must be through `libuv`.
- **Zero-Copy Performance:** Prefer `net_buf_t` for cross-plane data transfer.
- **Test Integrity:** Every new driver or stage MUST have a corresponding `cmocka` test.

## 📅 5. Known Issues
- `test_protocol_loopback` currently fails with `uv__finish_close` assertion. **Status: Acknowledged, Skip for now.**