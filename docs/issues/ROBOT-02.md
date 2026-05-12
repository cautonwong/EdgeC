## Parent

Reference: /workspaces/edgeos/EdgeC/ROBOT_PRD.md

## What to build

Develop the `high_speed_foc_pipeline` as an isolated compute path within the StreamEngine. To achieve sub-millisecond latencies (target 10kHz), this pipeline must bypass standard DataHub polling. It will use `req_raw_io` to sample Phase A and Phase B currents, immediately execute Clark and Park mathematical transforms from the Algorithm Lexicon, and calculate SVPWM duty cycles based on the target velocity/torque.

## Acceptance criteria

- [ ] `high_speed_foc_pipeline` is implemented as an independent thread/loop in StreamEngine.
- [ ] Raw ADC reads are tightly coupled with the computation stages.
- [ ] Clark and Park transforms are correctly integrated and produce valid D/Q axis components.
- [ ] Inverse transforms and SVPWM sector selection logic output valid duty cycles.
- [ ] A latency benchmark test demonstrates the entire input-to-output cycle completes in under 50 microseconds.

## Blocked by

None - can start immediately