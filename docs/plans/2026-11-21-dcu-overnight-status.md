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
| `dcu/D0-05-frame-vectors` | D0-05 | rebased to `ac239d2e`; 26/26 target tests; suite 21 cases, 19 pass, 2 skip (HCS, Record/RSD) | `/no-comments` review, then merge |
| `dcu/D1-04-var-handlers` | D1-04 | review found 2009/200A/200F macro drift and 0010/0020 attr2/4 type-tag mixing; dcu-dev fixing and rebasing to `149d2634` | fix, `/no-comments`, verify, merge |
| `dcu/D0-05a-hcs` | D0-05a | queued; needs D0-05 merged | fix HCS validation, unskip `test_reject_bad_hcs` |
| `dcu/D1-13-axdr-port` | D1-13 | queued after D1-04 | port callbacks, then D1-05/06/07 |

## Landing log

| Time | Task | Result | Evidence |
| --- | --- | --- | --- |
| 2026-11-21 | D0-05 | verification passed, comment review running | 26/26, suite 19/2 skip |
| 2026-11-21 | D1-04 | verification failed review, returned | OI macro drift, type tags |

## Morning report

Update this section before stopping. Name:

1. Branches merged, with merge SHAs.
2. Branches not merged, with the exact failing command and log path.
3. Flakes observed and the rerun that cleared them.
4. Remaining plan tasks and their blockers.
5. Worktrees and branches removed.
