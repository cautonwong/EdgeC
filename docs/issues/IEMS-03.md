## Parent

Reference: /workspaces/edgeos/IEMS_PRD.md

## What to build

Develop the `mpc_dispatcher_stage` within the StreamEngine. This stage subscribes to forecast arrays (prices, solar generation) and current SOC (ideally from the EKF stage). It will use the `control_mpc_formulate` to generate a batch matrix and `control_qp_solve_hildreth` to compute an optimal, constraint-aware power setpoint for the PCS (Power Conversion System).

## Acceptance criteria

- [ ] The `mpc_dispatcher_stage` is implemented and registered in the factory.
- [ ] The stage successfully formulates the MPC problem matrix for a rolling horizon based on incoming forecasts.
- [ ] The Hildreth QP solver is invoked to respect constraints (e.g., maximum charge current).
- [ ] The optimal power target is dispatched as a stream event.
- [ ] A virtual plant test using `stage_transfer_function` demonstrates that the MPC output smoothly drives the system to the economic target without constraint violations.

## Blocked by

- [IEMS-02] 非线性电池状态观测器算子 (EKF SOC Estimator Stage)