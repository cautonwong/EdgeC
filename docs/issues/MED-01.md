## Parent

Reference: /workspaces/edgeos/EdgeC/MED_CTRL_PRD.md

## What to build

Develop the `dsp_notch_filter_stage`. This stage will ingest raw, high-frequency (e.g., 1kHz) ADC values representing an ECG signal. It will utilize the `edge_dsp` library to instantiate an extremely narrow-band IIR notch filter specifically tuned to eliminate either 50Hz or 60Hz powerline interference (configurable via the stage definition) before passing the cleaned signal down the pipeline.

## Acceptance criteria

- [ ] `dsp_notch_filter_stage` is implemented and can parse the target frequency (50/60) and Q-factor from the semantic contract.
- [ ] The stage computes the precise IIR `b` and `a` coefficients dynamically based on the configured frequency.
- [ ] An AFK test injects a compound signal (1Hz sine + 60Hz noise) and asserts that the 60Hz component is attenuated by >40dB, while the 1Hz amplitude remains >95% intact.

## Blocked by

None - can start immediately