## Parent

Reference: /workspaces/edgeos/EdgeC/MED_CTRL_PRD.md

## What to build

Create the `ecg_anomaly_detector_stage` to analyze cardiac rhythms for irregular transients (like PVCs) locally. This stage must buffer the incoming, filtered ECG signal. It will execute the `dsp_dwt_haar` function to decompose the signal into approximation and detail coefficients. By establishing an energy threshold on the high-frequency detail bands, it will detect sudden morphological spikes that deviate from a normal QRS complex, outputting an asynchronous `PIPEMSG_TYPE_EVENT` alarm.

## Acceptance criteria

- [ ] `ecg_anomaly_detector_stage` is implemented and configurable with buffer size, DWT depth, and anomaly threshold.
- [ ] The stage successfully executes the Haar DWT over the rolling window.
- [ ] Energy calculation on the detail coefficients accurately identifies the location of synthetic transients.
- [ ] High-priority alarm events are published to the DataHub upon detection.

## Blocked by

- [MED-01] DSP 50/60Hz Mains Hum Notch Filter