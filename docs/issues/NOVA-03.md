## Parent

Reference: /workspaces/edgeos/EdgeC/NOVA_HMI_PRD.md

## What to build

Create the `canvas_spectrum_viewer` frontend component. This component must connect to the binary WebSocket stream (from NOVA-02) and render dense, high-frequency data arrays (like 2D FFT heatmaps or raw ADC waveforms). To achieve 60 FPS without crashing the browser DOM, it must use WebGL or the HTML5 Canvas API directly. It should adhere strictly to the "Cyber-Physical" aesthetic (e.g., glowing neon lines on a dark grid).

## Acceptance criteria

- [ ] A React/Vue component is created that renders an HTML5 Canvas or WebGL context.
- [ ] JavaScript logic is implemented to receive binary WebSocket frames, unpack the ArrayBuffer (handling endianness), and update the drawing state.
- [ ] The Canvas smoothly renders a scrolling waveform or a 2D heatmap at 60 FPS.
- [ ] The visual style matches the core design system (sharp accents, dark background, technical grid).
- [ ] A synthetic test ensures the browser memory usage remains stable over a 5-minute continuous 1kHz data stream.

## Blocked by

- [NOVA-01] Frontend Core Scaffold & Design System
- [NOVA-02] Backend Binary Telemetry Streamer