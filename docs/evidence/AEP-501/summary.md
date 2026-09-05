# AEP-501 Delivery Summary: Global Web Console Drawer & Interactive Ansi Terminal (WebConsoleDrawer)

## 1. Scope & Overview
- **Issue Reference**: [Issue #185](https://github.com/cautonwong/EdgeC/issues/185)
- **Components**: 
  - `web/core/src/components/console/WebConsoleDrawer.vue`: Global drawer host with 3-state sizing (docked/fullscreen/floating), multi-tab router, and hotkey interceptor.
  - `web/core/src/components/console/AnsiTerminalView.vue`: High-performance 16-color ANSI SGR terminal renderer & interactive shell engine.
- **Layout Mounting & Exports**: `web/core/src/layouts/AdminLayout.vue` (header terminal trigger + global mounting), `web/core/src/index.ts`
- **Unit Test Suite**: `tests/test_web_console_drawer.c` (registered in `tests/CMakeLists.txt`)

---

## 2. Core Functional Highlights

1. **Global Hotkey Summoning & 3-State Geometry**:
   - **Global Hotkey Interceptor**: Captures `Ctrl + \``, `Ctrl + ~`, `Alt + D` globally across all pages without losing page context;
   - **3-State Geometry Modes**:
     - `Docked`: Bottom drawer with dynamic drag-to-resize height (25vh ~ 85vh);
     - `Fullscreen`: 100vh full-screen focus terminal for extensive log and packet forensic inspection;
     - `Floating`: 720px compact floating window for side-by-side debugging with live charts.

2. **Multi-Modal Diagnostic Tab Architecture**:
   - **Tab 1: CLI Shell Terminal (`AnsiTerminalView.vue`)**:
     - Built-in EdgeOS shell command dispatcher (`help`, `datahub stats`, `dds peers`, `mtt clock`, `drivers list`, `streams dag`, `storage wal`, `metrics`, `version`, `clear`, `uptime`);
     - Tab auto-completion, history navigation (Up/Down arrow keys), and 16-color ANSI SGR text formatting;
   - **Tab 2: Live Log Stream Console**:
     - 50Hz live log stream with multi-level severity filtering (`ALL`, `INFO`, `WARN`, `ERROR/FATAL`) and module tagging (`BUS`, `DRIVER`, `STREAM`, `CHANNEL`);
   - **Tab 3: Protocol Packet Sniffer**:
     - Live DL/T 645/698 & Modbus frame capture stream with HEX dump and instant AXDR decoded summary;
   - **Tab 4: Industrial Quick Macros**:
     - 6 one-click diagnostic macro cards across EdgeOS four planes (`Dump DataHub Ringbuffer`, `Check Cyclone DDS Mesh`, `Query MTT Clock Precision`, `Inspect Southbound Drivers`, `Check SQLite WAL & SnF`, `Export OpenMetrics Payload`).

---

## 3. Verification & Compliance
- **Unit Tests**: `test_web_console_drawer` (4/4 PASS);
- **Full CTest Suite**: 174/174 PASS;
- **Omnis Multi-Plane Integration Test**: 10/10 Matrix Scenarios PASS;
- **Vite Web Production Build**: `smart_box` and `iems` compiled cleanly;
- **Zero Mock Rule**: 100% compliant with physical real-time data, genuine contract bindings, and six anti-entropy gates.
