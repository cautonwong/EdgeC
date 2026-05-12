## Parent

Reference: /workspaces/edgeos/EdgeC/AIOT_FUSION_PRD.md

## What to build

Enhance the existing WebAssembly support by creating a `tiny_ml_wasm_stage`. This stage will load a pre-compiled WASM binary containing a lightweight machine learning model (e.g., a TensorFlow Lite for Microcontrollers model compiled to WASM). The stage must buffer a window of sensor data from the DataHub, pass this buffer to the WASM inference function, and publish the resulting classification or regression output back to the DataHub.

## Acceptance criteria

- [ ] `tiny_ml_wasm_stage` is implemented and can be configured with a WASM file path.
- [ ] Memory mapping between the C host and the WAMR runtime is established to pass input buffers.
- [ ] The stage successfully executes the WASM inference function upon receiving sufficient data.
- [ ] Output predictions are extracted from WASM memory and published as stream events.
- [ ] A benchmark test verifies the inference invocation overhead is < 1ms.

## Blocked by

None - can start immediately