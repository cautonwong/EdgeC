## Parent

Reference: /workspaces/edgeos/EdgeC/NOVA_HMI_PRD.md

## What to build

Develop the `ui_telemetry_streamer` on the EdgeC backend. Extend the existing `ws_proto.c` WebSocket implementation to support bulk binary transfers. Instead of serializing large mathematical arrays (like 2D FFT results or EKF covariance matrices) into slow, bloated JSON strings, implement a mechanism to pack these arrays directly into binary WebSocket frames to maximize throughput and minimize CPU overhead.

## Acceptance criteria

- [ ] `ws_proto.c` is updated to handle binary opcode WebSocket frames.
- [ ] A C-side serialization function is created to pack arrays (e.g., `double[]`) with a small header (metadata like array dimensions or tag ID) into a binary buffer.
- [ ] A test route is added to stream a dummy 1024-element array via binary WebSocket at 60Hz.
- [ ] Memory limits and frame fragmentation are correctly handled to prevent buffer overflows.

## Blocked by

None - can start immediately