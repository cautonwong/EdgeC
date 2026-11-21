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
- Every merge check includes `check_build_warnings.py` on a clean-build log. D0-08 merged with a `-Wconversion` warning the test-only checks missed; D0-08a fixes it.

## Current stack

| Branch | Task | State | Next gate |
| --- | --- | --- | --- |
| `dcu/D0-05-frame-vectors` | D0-05 | merged `0f45d0e2` | done |
| `dcu/D1-04-var-handlers` | D1-04 | merged `100f9a41` | done |
| `dcu/D0-05a-hcs` | D0-05a | merged `ff845789` | done |
| `dcu/D1-13-axdr-port` | D1-13 | merged `68f4de69` | post-merge 28/28, guards, acceptance 2+4 passed; `/no-comments` 23 lines |
| `dcu/D0-06-docs` | D0-06 | merged `50feb55e` | done |
| `dcu/D0-01-readme` | D0-01 | merged `ea775f3f` | done |
| `dcu/D0-07-catalog-ci` | D0-07 | merged `355c8251` | post-merge catalog 0, parity 11, guards, 28/28, acceptance 2+4 |
| `dcu/D0-05b-dar` | D0-05b | verified code, doc sync + rebase pending | mapping 0/2/4/5/6/7, GET-Normal DAR response; waits on D0-08 |
| `dcu/D0-04-baseline` | D0-04 | merged `5c808baf` | baseline evidence; ASAN found a real test lifetime bug |
| `dcu/D0-08-asan-frame-lifetime` | D0-08 | merged `9e458af6` | ASAN 330/330, dlt698 6/6; post-merge main ASAN 6/6 |
| `dcu/D1-10-catalog-consistency` | D1-10 | active, edgec-dev-0 | table/catalog drift test + generated fixture |
| `dcu/D0-08-frame-len` | D0-08a | active, dcu-dev | explicit `uint16_t` bound, clears the `-Wconversion` warning |

## Landing log

| Time | Task | Result | Evidence |
| --- | --- | --- | --- |
| 2026-11-21 | D0-05 | merged `0f45d0e2` | 26/26, suite 19/2 skip; DAR vector corrected to 6 |
| 2026-11-21 | D1-04 | merged `100f9a41` | 27/27, guards, acceptance; 200A/200F and attr2/attr4 aligned |
| 2026-11-21 | D0-05a / D1-13 | dispatched in parallel | HCS codec; A-XDR port |
| 2026-11-21 | D0-05a | merged `ff845789` | 27/27, suite 20/1 skip, guards, acceptance |
| 2026-11-21 | D1-13 | verification failed review, returned | ADR-0013 §3 forbids 8×2048; constant name is product-specific in foundation |
| 2026-11-21 | D1-13 | fixed and verified | streaming per-item encode; 28/28, guards, acceptance; comment review running |
| 2026-11-21 | D1-13 | merged `68f4de69` | post-merge build 0, 28/28, guards all green, acceptance 2+4; README A-XDR line updated |
| 2026-11-21 | D0-07 | verified | catalog gate 0 on live tree, parity 11 gates, selftest accepts copy and rejects corrupted copy |
| 2026-11-21 | D0-07 | merged `355c8251` | post-merge catalog 0, parity 11, guards all green, 28/28, acceptance 2+4; `/no-comments` deleted 10 lines |
| 2026-11-21 | D0-04 | merged `5c808baf` | Release/UBSAN/acceptance/coverage pass; ASAN red reproduced standalone, test helper lifetime bug; coverage `cmd` depends on running app_platform_e2e first |
| 2026-11-21 | D0-08 | dispatched | fix `decode_frame`, restore CI x86_64_asan; ASAN full ctest is the guard |
| 2026-11-21 | D0-08 | merged `9e458af6` | ASAN before 8 / after 0; full 330/330, dlt698 6/6; post-merge main ASAN 6/6, release 28/28, guards green |
| 2026-11-21 | D0-08a | found and dispatched | clean build on main: `check_build_warnings.py` red on `size_t` -> `uint16_t` in `decode_frame_borrows`; test-only checks missed it |
| 2026-11-21 | D0-05b | rebased on `9e458af6` | conflicts in the helper call sites resolved to `decode_frame_borrows`; 28/28, ASAN 6/6, guards, acceptance |
| 2026-11-21 | D0-05b | code verified, waiting | DAR mapping and GET-Normal DAR response good; needs ADR/design sync and rebase onto D0-08 |

## Morning report

Update this section before stopping. Name:

1. Branches merged, with merge SHAs.
2. Branches not merged, with the exact failing command and log path.
3. Flakes observed and the rerun that cleared them.
4. Remaining plan tasks and their blockers.
5. Worktrees and branches removed.
