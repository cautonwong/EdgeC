# Storage Hardening Implementation & Execution Plan

- **Target Release**: EdgeOS v2.6-StorageHardened
- **Date**: 2026-08-14
- **Status**: Approved & Executed
- **Associated ADR**: [ADR-0007: Storage Subsystem Hardening](file:///workspaces/edgeos/EdgeC/docs/adr/0007-storage-subsystem-hardening.md)

---

## 1. Phased Roadmap

1. **Phase 1: SQLite Service Production WAL & Security PRAGMAs Hardening**:
   - Update `modules/edge_core/src/sqlite_service.c` to enforce `PRAGMA journal_mode = WAL;`, `PRAGMA synchronous = NORMAL;`, `PRAGMA temp_store = MEMORY;`.
2. **Phase 2: Crash-Resilient Disk-Backed WAL Buffer (`resilient_buffer.c`)**:
   - Add disk persistence file (`resilient_buffer.wal`) and header CRC32 check on initialization to recover offline telemetry after crash.
3. **Phase 3: Zero-Copy SHM Guard Page Protection (`shmring.c`)**:
   - Inject tail page `mprotect(PROT_NONE)` in `shmring_create` to prevent buffer overflow corruption.
4. **Phase 4: Time-Series Gorilla Compression RLE Optimization (`timeseries_compressor.c`)**:
   - Add RLE run-length bitstream flags for zero XOR delta streams.
5. **Phase 5: Full Engineering Verification**:
   - Rebuild workspace and execute all 128 C unit tests and Go unit tests.
