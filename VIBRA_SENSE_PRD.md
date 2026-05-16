## Problem Statement

Large-scale mechanical assets (e.g., wind turbines, conveyor belts, railway bearings) are prone to wear and tear. Traditional "scheduled" maintenance leads to either expensive unnecessary part replacements or catastrophic "run-to-failure" events. Existing condition monitoring systems often require high-bandwidth streaming of vibration data to a central server, which is technically impossible in remote wind farms or moving trains.

## Solution

Vibra-Sense is an EdgeC-powered predictive maintenance appliance. It utilizes high-frequency ADC sampling (up to 20kHz) and the `Algorithm Lexicon` to perform "Signal-to-Insight" conversion at the edge. It uses **2D FFT** to identify mechanical resonance peaks and **Haar Wavelets (DWT)** to detect microscopic surface pitting in bearings via transient "spikes" in the current/vibration signature. By using an **Extended Kalman Filter (EKF)**, it tracks the degradation of the asset over time, outputting a Remaining Useful Life (RUL) estimate directly via MQTT.

## User Stories

1. As a **maintenance technician**, I want to receive an alert when a bearing's "Vibration Energy Index" increases by 10%, so that I can schedule a repair before the machine seizes.
2. As an **asset owner**, I want to see a daily RUL percentage for every motor in my facility, so that I can optimize my spare parts inventory.
3. As a **telecom manager**, I want the device to process 20,000 samples per second locally and only send one health status message per hour, saving 99% of my satellite/cellular bandwidth.

## Implementation Decisions

- **`module: vibration_feature_stage`**: A StreamEngine stage that computes Root Mean Square (RMS), Kurtosis, and Skewness of high-frequency samples.
- **`module: rul_prediction_stage`**: A state-space model implemented using EKF. It models the "wear" as a hidden state that increases monotonically, corrected by the observed vibration features.
- **`module: buffer_streaming_api`**: A REST/WS endpoint that allows an engineer to occasionally pull a "snapshot" of raw raw high-frequency data for manual deep-dive analysis.

## Testing Decisions

- **Simulated Wear Test (AFK)**: We will inject a synthetic vibration signal that gradually increases in noise and "transient density". We will assert that the `rul_prediction_stage` correctly decreases its RUL estimate and triggers alarms at the specified degradation thresholds.
