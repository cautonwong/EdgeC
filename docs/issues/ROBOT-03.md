## Parent

Reference: /workspaces/edgeos/EdgeC/ROBOT_PRD.md

## What to build

Implement the `wavelet_anomaly_stage` for predictive maintenance. This stage will accumulate a high-frequency buffer of motor current/vibration data. It will utilize the `dsp_dwt_haar` function from the math lexicon to perform Discrete Wavelet Transforms. By analyzing the high-frequency detail coefficients, the stage must detect abnormal transients (indicative of mechanical faults) and emit a high-priority alarm event if a configured threshold is breached.

## Acceptance criteria

- [ ] `wavelet_anomaly_stage` is created and can buffer incoming scalar arrays.
- [ ] The stage successfully executes the Haar DWT on the buffered data.
- [ ] Anomaly detection logic correctly analyzes the detail (high-frequency) components against a configurable threshold.
- [ ] The stage publishes a `PIPEMSG_TYPE_EVENT` when an anomaly is detected.
- [ ] A synthetic test successfully detects a simulated 10-sample transient hidden within a sine wave, without triggering false alarms on the clean sine wave.

## Blocked by

None - can start immediately