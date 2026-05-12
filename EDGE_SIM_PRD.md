## Problem Statement

Developing control logic for complex industrial systems (like robotic arms, smart HVACs, or power grids) is dangerous and expensive to test on physical hardware. Engineers need a way to run the exact same Edge Gateway software against a simulated physical environment (a Digital Twin) before deploying to the real world. Furthermore, the rising need to train Reinforcement Learning (RL) agents requires a fast, headless simulation engine that can run identical logic to the target edge device.

## Solution

The EdgeC Industrial Simulation Engine (Edge-Sim) repurposes the high-performance C architecture into a deterministically ticking "Game Console" for industrial physics. It utilizes the WebAssembly (`stage_wasm`) runtime to execute arbitrary physics engines or game logic (compiled from Rust/C++) securely. The `Algorithm Lexicon` handles collision matrices and differential equations. It interfaces with external control scripts (like an RL Python agent) via high-speed WebSockets or shared memory, effectively turning EdgeC into a lightning-fast industrial OpenAI Gym environment.

## User Stories

1. As a **control engineer**, I want to deploy a mathematical model of a motor (using `stage_transfer_function`) inside the EdgeC engine, so that I can tune my PID loops against it without burning out real motors.
2. As a **reinforcement learning researcher**, I want to connect my Python training script to the EdgeC pipeline via zero-copy IPC or WebSockets, so that my AI agent can interact with the simulated environment at thousands of steps per second.
3. As a **QA engineer**, I want to connect a physical gamepad/joystick to the Linux host and route its inputs through `req_raw_io` to the EdgeC DataHub, so that I can manually "drive" the simulated industrial equipment to test edge cases.

## Implementation Decisions

- **`module: wasm_physics_engine`**: A specialized WASM stage that executes a compiled physics step function. It reads forces/torques from the DataHub, computes the next kinematic state, and writes position/velocity back to the DataHub.
- **`module: evdev_joystick_driver`**: A Linux `evdev` input driver bridging physical human input devices (joysticks/wheels) into EdgeC StreamEngine events, translating analog stick values to normalized float arrays.
- **`module: openai_gym_bridge`**: An RPC server (using `rest_server` or `ws_proto`) that exposes standard `reset()`, `step()`, and `render()` endpoints to allow external Python scripts to orchestrate the simulation ticks.

## Testing Decisions

- **Deterministic Tick Test (AFK)**: We will run the simulation with a fixed random seed and known inputs for 1000 ticks. The final state hash must match the expected value exactly, proving the engine does not suffer from floating-point drift or race conditions.

## Out of Scope
- Rendering 3D graphics (Unreal/Unity). The Edge-Sim engine computes purely headless physical states. The "Nova HMI" can be used for 2D visualization if needed.