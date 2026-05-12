## Problem Statement

FMCW (Frequency-Modulated Continuous Wave) radars generate massive amounts of raw ADC data (Intermediate Frequency signals). Sending this raw data to a central server or cloud for processing requires gigabit bandwidth and introduces latency, making it useless for real-time applications like drone collision avoidance, perimeter security, or autonomous traffic monitoring. Traditional embedded systems struggle with the heavy matrix math and DSP operations required to extract targets from the noise.

## Solution

The EdgeC Radar Edge Processor (Radar-Edge) runs directly on the radar module's companion chip (e.g., an ARM Cortex-A or high-end MCU). It ingests raw ADC streams using the `DataHub`'s zero-copy memory ring. It utilizes the `Algorithm Lexicon`'s highly optimized 2D FFT (`dsp_fft2d_c2c`) to generate Range-Doppler maps, and applies the CA-CFAR (`radar_cfar_ca`) algorithm to extract valid target coordinates (distance, velocity) while ignoring clutter. Only a few bytes of target metadata are published via MQTT, slashing bandwidth requirements by 99.9%.

## User Stories

1. As a **security architect**, I want the radar gateway to process raw signals locally and only send X,Y,V coordinates of moving targets to the cloud, so that I can monitor a vast perimeter over a low-bandwidth cellular network.
2. As a **DSP engineer**, I want the system to apply Windowing functions (Hann/Blackman) before the FFT, so that target reflections do not leak into adjacent frequency bins and mask smaller objects.
3. As a **system integrator**, I want to extract the detected targets and correlate them with a pan-tilt-zoom (PTZ) camera's coordinate system using homogeneous transformation matrices, so that the camera automatically tracks radar targets.

## Implementation Decisions

- **`module: radar_range_doppler_stage`**: A StreamEngine stage that accumulates a frame of raw ADC chirps. It applies a 2D FFT to generate the Range-Doppler matrix representing distance and velocity.
- **`module: cfar_extractor_stage`**: Reads the Range-Doppler matrix and applies 2D CA-CFAR (or a modified 1D pass) to identify peaks above the noise floor. It outputs a lightweight struct of detected targets.
- **`module: spatial_calibrator_stage`**: Uses `edge_matrix_mul` (4x4 homogeneous matrices) to translate radar-relative polar coordinates into absolute global cartesian coordinates.

## Testing Decisions

- **Raw Data Injection (AFK)**: A Python script will inject a pre-recorded binary file of raw FMCW chirps into the EdgeC `req_raw_io` interface. We will assert that the `cfar_extractor_stage` outputs the exact targets known to be in the recording.

## Out of Scope
- Micro-Doppler signature classification for identifying specific object types (e.g., distinguishing a dog from a human based on gait).