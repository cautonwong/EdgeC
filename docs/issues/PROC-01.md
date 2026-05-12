## Parent

Reference: /workspaces/edgeos/EdgeC/PROCESS_CTRL_PRD.md

## What to build

Integrate the `stage_iir_filter.c` into a practical `dsp_filter_stage` usable in process control pipelines. This stage will receive raw, noisy analog data (e.g., from a water level sensor via Modbus). It must apply a configurable Infinite Impulse Response (IIR) filter (e.g., a Butterworth low-pass filter) to smooth the data, outputting the clean signal to the DataHub for downstream control loops.

## Acceptance criteria

- [ ] The `dsp_filter_stage` is fully integrated and can be configured with specific cutoff frequencies via semantic contracts.
- [ ] The stage successfully reads noisy scalars and outputs filtered scalars.
- [ ] An AFK test injects a sine wave mixed with high-frequency Gaussian noise. The test must assert that the output variance is reduced by at least 80% while phase delay remains within acceptable limits.

## Blocked by

None - can start immediately