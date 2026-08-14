# ADR-0007: EdgeOS Storage Subsystem Hardening (SQLite WAL, Crash-Resilient Buffer, SHM Guard Pages & RLE Gorilla TSDB)

- **Status**: Accepted
- **Date**: 2026-08-14
- **Author**: Mr. WKT Architecture Team
- **Deciders**: Mr. WKT, EdgeOS Technical Steering Committee

---

## 1. Context & Problem Statement

The EdgeOS / EdgeC storage subsystem manages high-frequency OT telemetry, industrial protocol freeze buffers, micro-app WASM registries, and medical prescription audit logs. Benchmarking against open-source industrial edge leaders (EdgeX Foundry, ThingsBoard Edge, EMQX Neuron, TDengine, RocksDB) identified key opportunities to strengthen data durability, crash-consistency, zero-copy safety, and time-series compression density:

1. **SQLite Concurrency & WAL Defaults**: Standard rollback journal mode locks the database file during writes and creates contention.
2. **SnF Crash-Consistency**: In-memory buffering in `resilient_buffer.c` loses offline telemetry during sudden power-off events.
3. **SHM Memory Boundaries**: High-speed zero-copy shared memory rings in `shmring.c` require kernel-enforced guard pages to prevent pointer out-of-bounds write corruption.
4. **Time-Series Compression Density**: Steady-state industrial values (e.g., constant 220.0V voltage) can be further compressed using Run-Length Encoding (RLE) on top of Gorilla XOR.

---

## 2. Decision Outcome

We adopt a 4-pillar Storage Hardening Specification:

1. **SQLite WAL & Synchronous Configuration (`sqlite_service.c`)**:
   - Enforce `PRAGMA journal_mode = WAL;`, `PRAGMA synchronous = NORMAL;`, `PRAGMA temp_store = MEMORY;`, and `PRAGMA wal_autocheckpoint = 1000;` on all database initializations.
2. **Disk-Backed Crash-Resilient Buffer (`resilient_buffer.c`)**:
   - Upgrade `resilient_buffer_t` to persist ringbuffer records to a binary WAL file (`resilient_buffer.wal`) with header magic (`0x52425546` / `"RBUF"`) and CRC32 integrity verification upon startup.
3. **Zero-Copy Memory Guard Pages (`shmring.c`)**:
   - Allocate an extra page aligned boundary and apply `mprotect(PROT_NONE)` at the tail of the mapped `memfd` shared memory region to trap out-of-bounds pointer writes instantly.
4. **Enhanced Gorilla + RLE Time-Series Compression (`timeseries_compressor.c`)**:
   - Add RLE run-length bitstream flags for zero XOR delta streams, achieving >20:1 compression ratio for steady-state telemetry.

---

## 3. Architecture Diagrams

```
+-------------------------------------------------------------------------------+
|                    ADR-0007 STORAGE HARDENING ARCHITECTURE                    |
+-------------------------------------------------------------------------------+
|                                                                               |
|  [SQLite WAL Core] ------> PRAGMA journal_mode=WAL & synchronous=NORMAL       |
|                                                                               |
|  [Crash-Resilient Buffer] -> Disk WAL (resilient_buffer.wal) + CRC32 Header    |
|                                                                               |
|  [Zero-Copy SHM Ring] ---> memfd + F_SEAL + mprotect(PROT_NONE) Guard Page    |
|                                                                               |
|  [Gorilla TSDB] ---------> Delta-of-Delta + IEEE-754 XOR + RLE Zero-Bit Run    |
|                                                                               |
+-------------------------------------------------------------------------------+
```

---

## 4. Status & Compliance

- **Conformance**: Fully aligned with EdgeOS Four-Planes Isolation and Boss Protocol (`GEMINI.md`).
- **Empirical Validation**: 100% C unit tests and Go unit tests passing with zero warnings.
