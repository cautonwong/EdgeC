# AEP-493 Architectural Redlines

## 1. Boss Protocol & Architectural Gates
- [x] Boss Protocol Compliance: Mr. WKT priority focus maintained across all iterations.
- [x] Zero Mock Placeholders: Telemetry and power flows strictly adhere to DataHub contracts and physical units (kW, V, Hz).
- [x] Surgical Editing: Changes strictly isolated to DataCurves/HistoryCurves views, Vite configuration, and dedicated test suite.
- [x] Four Planes Isolation: Visual curves engine decoupled from C kernel via contract schemas and REST/WS APIs.
- [x] Memory & Performance Gates: LTTB downsampling enabled on all series, animation disabled during high-frequency live streams to ensure 60fps, explicit chart disposal on unmount to guarantee zero memory leaks.
