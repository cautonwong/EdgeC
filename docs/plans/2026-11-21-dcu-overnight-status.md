# DCU overnight landing status

> Date: 2026-11-21. Operator asleep. This file is the overnight decision trail.
> Plan: `docs/plans/2026-11-21-dcu-698-production-plan.md`.

## Exit condition

Every branch with an independent PASS verdict is merged into `main`. No verified branch remains
unmerged. `main` passes:

- `cmake --build build -j$(nproc)`
- `ctest -R 'dcu|dlt698'`
- `python3 ci/scripts/run_guard_selftest.py --root .`
- `./test.sh --omnis dcu`

This file and the plan status list what landed and what remains.

"Everything" means the current stack plus any task that can be completed and independently
verified before morning. The full P2 to P5 program is blocked on the COM components
(`docs/plans/2026-11-21-terminal-common-components-plan.md`); it is not in scope tonight.

## Flake policy

- A failing command is rerun twice.
- If it passes on rerun with the diff unchanged, record the flake and continue.
- If it fails consistently, do not merge. Record `BLOCKED` with the command and the log path.
- Never skip, delete, weaken, or comment out a test or a gate to make a branch land.

## Current stack

| Branch | Task | State | Next gate |
| --- | --- | --- | --- |
| `dcu/D0-05-frame-vectors` | D0-05 | merged `0f45d0e2` | done |
| `dcu/D1-04-var-handlers` | D1-04 | merged `100f9a41` | done |
| `dcu/D0-05a-hcs` | D0-05a | merged `ff845789` | done |
| `dcu/D1-13-axdr-port` | D1-13 | returned for fix | 28/28 passed, but the driver used an 8×2048 TLS list buffer, violating ADR-0013 §3; streaming per-item encode and `EDGE_SLAVE_AXDR_MAX` rename pending |
| `dcu/D0-06-docs` | D0-06 | active, edgec-dev-0 | sync northbound and DCU context docs |
| `dcu/D0-05b-dar` | D0-05b | queued after D1-13 | align DAR 4/5/6/7 in driver/adapter |

## Landing log

| Time | Task | Result | Evidence |
| --- | --- | --- | --- |
| 2026-11-21 | D0-05 | merged `0f45d0e2` | 26/26, suite 19/2 skip; DAR vector corrected to 6 |
| 2026-11-21 | D1-04 | merged `100f9a41` | 27/27, guards, acceptance; 200A/200F and attr2/attr4 aligned |
| 2026-11-21 | D0-05a / D1-13 | dispatched in parallel | HCS codec; A-XDR port |
| 2026-11-21 | D0-05a | merged `ff845789` | 27/27, suite 20/1 skip, guards, acceptance |
| 2026-11-21 | D1-13 | verification failed review, returned | ADR-0013 §3 forbids 8×2048; constant name is product-specific in foundation |

## Morning report

Update this section before stopping. Name:

1. Branches merged, with merge SHAs.
2. Branches not merged, with the exact failing command and log path.
3. Flakes observed and the rerun that cleared them.
4. Remaining plan tasks and their blockers.
5. Worktrees and branches removed.
