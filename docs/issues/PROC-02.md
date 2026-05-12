## Parent

Reference: /workspaces/edgeos/EdgeC/PROCESS_CTRL_PRD.md

## What to build

Implement the `advanced_pid_stage` within the StreamEngine. This stage wraps the `control_pid_compute` function from the math lexicon. It will subscribe to a process variable (e.g., filtered water level) and output a control signal (e.g., pump speed). Crucially, the stage must enforce output constraints and implement integral anti-windup to prevent overshoot when actuators saturate.

## Acceptance criteria

- [ ] `advanced_pid_stage` is created and registered.
- [ ] It can be configured with Kp, Ki, Kd, setpoint, and output limits.
- [ ] The stage correctly computes the control signal based on incoming process variable updates.
- [ ] An AFK test simulates a blocked actuator scenario, proving that the integral term does not wind up and the system recovers smoothly once the blockage is removed.

## Blocked by

- [PROC-01] DSP Filtering Pipeline