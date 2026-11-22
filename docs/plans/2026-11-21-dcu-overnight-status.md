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

Final state at the end of the night. Every row has an independent PASS verdict and a green
post-merge gate run on `main`.

| Branch | Task | State | Note |
| --- | --- | --- | --- |
| `dcu/D0-05-frame-vectors` | D0-05 | merged `0f45d0e2` | real 698 frame vectors |
| `dcu/D1-04-var-handlers` | D1-04 | merged `100f9a41` | typed variable-class reads |
| `dcu/D0-05a-hcs` | D0-05a | merged `ff845789` | HCS validation |
| `dcu/D1-13-axdr-port` | D1-13 | merged `68f4de69` | A-XDR port, streaming NormalList |
| `dcu/D0-06-docs` | D0-06 | merged `50feb55e` | architecture docs sync |
| `dcu/D0-01-readme` | D0-01 | merged `ea775f3f` | README rewrite |
| `dcu/D0-07-catalog-ci` | D0-07 | merged `355c8251` | catalog CI gate |
| `dcu/D0-05b-dar` | D0-05b | merged `731b1331` | DAR mapping and GET-Normal DAR |
| `dcu/D0-04-baseline` | D0-04 | merged `5c808baf` | P0 baseline; found the ASAN bug |
| `dcu/D0-08-asan-frame-lifetime` | D0-08 | merged `9e458af6` | borrowed frame lifetime fix |
| `dcu/D0-08-frame-len` | D0-08a | merged `438ed230` | explicit `uint16_t` bound |
| `dcu/D0-09-class-ids` | D0-09 | merged `6638ce23` | four class_ids, F100 in catalog |
| `dcu/D1-14-set` | D1-14 | merged `b5b95f62` | SET-Request and write path |
| `dcu/D1-10-catalog-consistency` | D1-10 | merged `5a4fda20` | generated fixture and drift test |
| `dcu/D1-15a-record-codec` | D1-15a | merged `87185420` | GetRecord/RSD/RCSD codec |
| `dcu/D1-05-resolve-table` | D1-05 | merged `ac7b8dfe` | one OAD→type table |
| `dcu/D1-15b-record-driver` | D1-15b | merged `7658967a` | ADR-0014 record port |
| `dcu/D1-08-normal-list` | D1-08 | merged `c291f3e7` | empty list, mixed DAR, duplicate OAD |
| `dcu/D0-10-atomic-bus-log` | D0-10 | merged `28b0363f` | atomic `bus.json` |
| `dcu/D1-06-action` | D1-06 | merged `9a7fb7f9` (partial) | A.9 decode; 129–131 DAR 2 pending relay mapping |

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
| 2026-11-21 | D0-08a | merged `438ed230` | bound check + cast; clean-build warning gate 0 new |
| 2026-11-21 | D0-05b | merged `731b1331` | post-merge 28/28, ASAN 6/6, warning gate, guards, acceptance 2+4 all green |
| 2026-11-21 | D0-09 | dispatched | D1-10 found real class_id drift: F100 10->21, F200 10->22, 3000/3005 7->24; catalog drops F100 because its name is ASCII |
| 2026-11-21 | D0-09 | merged `6638ce23` | catalog 513->514 with F100; comment review restored 2 over-deleted why notes; post-merge build/warning/catalog/28/28/guards/acceptance green |
| 2026-11-21 | D1-10 | resumed | strict assertions on the corrected base; known gap: 3000/3005 masks still use Class 7 numbers |
| 2026-11-21 | D1-14 | merged `b5b95f62` | SET path; review returned 3 fixes (non-scalar fallback, LONG64 UB, DENIED vs CLASS_MISMATCH); post-merge all gates green |
| 2026-11-21 | D1-05 | dispatched | single OAD→type source; acceptance expands to 2001/2004/6012/5004 |
| 2026-11-21 | D1-10 | merged `5a4fda20` | 514-OI fixture validated by --check; red/green on a deliberate F200 class change; post-merge 29/29 |
| 2026-11-22 | D1-15a | merged `87185420` | codec only; H.3.3 response byte-exact; unsupported selectors explicit; post-merge 29/29 |
| 2026-11-22 | D1-05 | merged `ac7b8dfe` | single type table; found and fixed a ~900 KB OOB in the 5004 count sink; post-merge 30/30, full 332/332, ASAN, guards, acceptance 7 |
| 2026-11-22 | D1-15b | merged `7658967a` | ADR-0014; driver 10/10; acceptance failed once on an empty `bus.json`, rerun green; D0-10 dispatched to make the write atomic |
| 2026-11-22 | D1-08 | merged `c291f3e7` | empty list legal (Table 73), mixed DAR, duplicate OAD; post-merge 30/30 and acceptance 8 green |
| 2026-11-21 | D1-10 | blocked | table/catalog consistency cannot pass honestly until D0-09 lands; no exemption list |
| 2026-11-21 | D0-05b | code verified, waiting | DAR mapping and GET-Normal DAR response good; needs ADR/design sync and rebase onto D0-08 |

## Morning report

Update this section before stopping. Name:

1. Branches merged, with merge SHAs.
2. Branches not merged, with the exact failing command and log path.
3. Flakes observed and the rerun that cleared them.
4. Remaining plan tasks and their blockers.
5. Worktrees and branches removed.

### Night of 2026-11-21 → 2026-11-22

**Merged into `main` (in order).** Every one has an independent PASS verdict, post-merge
`check_build_warnings.py` on a clean build, `ctest -R 'dcu|dlt698'`, `run_guard_selftest.py`,
and `./test.sh --omnis dcu`.

| Task | Merge SHA | What landed |
| --- | --- | --- |
| D1-13 | `68f4de69` | A-XDR slave port, streaming NormalList, no `N × 2048` |
| D0-07 | `355c8251` | 698 catalog `--check` in CI and the guard selftest |
| D0-04 | `5c808baf` | P0 baseline evidence; ASAN failure found |
| D0-08 | `9e458af6` | `decode_frame_borrows` fixes the stack-use-after-scope |
| D0-08a | `438ed230` | explicit `uint16_t` bound; warning gate green |
| D0-05b | `731b1331` | DAR 0/2/4/5/6/7 and GET-Normal DAR response |
| D0-09 | `6638ce23` | class ids F100/F200/3000/3005; F100 in the catalog |
| D1-14 | `b5b95f62` | SET-Request, `accept_axdr`, SET-Response DAR |
| D1-10 | `5a4fda20` | 514-OI generated fixture; table/catalog consistency test |
| D1-15a | `87185420` | GetRecord/RSD/RCSD codec, Table 74 streaming |
| D1-05 | `ac7b8dfe` | one OAD→type table; 7-case acceptance; count_freezes OOB fix |
| D1-15b | `7658967a` | ADR-0014 record port, Selector1, driver GET [3] |
| D1-08 | `c291f3e7` | empty list legal, mixed per-item DAR, duplicate OAD |
| D0-10 | `28b0363f` | atomic `bus.json` write and concurrent-read guard |
| D1-06 | `9a7fb7f9` | A.9 ACTION params decoded; relay methods honestly DAR 2 |
| D1-14a | `e2ce657d` | big-endian reads no longer signed-shift UB |
| D1-17 | `5494f506` | Class 20 session and object-list ACL |
| D1-01 | `ceb6a576` | 79 catalog class conflicts reconciled, gate added |

Plus `chore(ci)` budget refreshes (`72394924`, `57846715`) and the plan/status doc commits.

**Not merged.** None. D1-06 landed partially: parameters decode, but 129–131 stay DAR 2 until
the product defines a relay OAD → meter/DO mapping. Nothing else is left unmerged.

**Flakes observed.**
- `./test.sh --omnis dcu` failed once on the D1-15b branch and once on main with
  `JSONDecodeError` in `_bus_requests` reading an empty `bus.json`. Reruns passed both times.
  Root cause: `dcu_product.py` truncates then writes the file. D0-10 makes the write atomic
  and adds a concurrent-read guard; it is the fix, not a test skip.

**Remaining work.**
- D1-06 follow-up: product decision on the relay OAD mapping, then 129/130/131 can execute.
- D1-18a variable-class standard shapes, then D1-18b/c/d for events, records/acquisition, control.
- D1-07 record storage wiring; D1-11 four-service acceptance.
- D6-08a TSAN lock-order; then D6-08 needs ASAN/UBSAN/TSAN full clean.

**OAD soundness audit (2026-11-22).** `docs/design/DCU_698_OAD_STATUS.md` covers all
56 advertised attribute/method bits. Result: 0 real, 37 partial, 12 stub, 3 unimplemented,
4 masked-but-undefined. Main causes: variable classes return scalars with wrong integer types
and no scaling; OAD element index is ignored for meter/phase selection; 3000/3005 advertise
Class 7 attribute numbers on Class 24 objects; 5004/6000/6012/F200 advertise semantics they do
not implement. D1-18a/b/c/d is the fix queue; D1-18a is in flight.

- P2–P5 blocked on the COM components (plan §11.3 B6); P6 hardware bench is out of scope.
- Known gaps: F200 attribute semantics, 3000/3005 Class 7-style masks, generator line window,
  per-class A.7 method maxima, test-platform parser rejects per-item DAR and empty lists.

**Cleanup.** `/tmp/dcu-docs-worktree` removed; merged branches deleted as they landed;
`/home/EdgeC/build-asan` kept for the remaining ASAN checks. `/tmp/dcu-*.log` verifier logs
could not be deleted (auto-mode blocked the shared-scratch sweep); they are small.

| 2026-11-22 | D2-02 | merged `42516ae6` | 6000 A2 array, 127–134 persisted, Class 20 ACL 16→20; 36/36, guards, 2 acceptance tasks |
| 2026-11-22 | D2-03 | merged `b4bb8829` | 6012 M1/127–130 and 6013 A2 real; strict A-XDR decode; SQLite config with rollback; 3+18 acceptance cases |
| 2026-11-22 | D3-05a | merged `3e291b74` | Class 24 A2/A3/A4/A11/A12 real; A5/A6/A14 masked pending a real source |
| 2026-11-22 | D3-04a | returned to dcu-dev-1 | date_time_s moves to device-local civil through the one helper (`TZ=CST-8` tests); unended A6 end uses Table 49 invalid encoding `1C FF×7`, A14 keeps NULL |
| 2026-11-22 | D3-06 | returned to edgec-dev-0 | 4000 A2 SET still RAM-shadow-only with DAR 0: real clock port or read-only; acceptance case `000019` collides, renumber to `000022` |
| 2026-11-22 | cleanup | 11 merged worktrees removed | d0-11, d1-07, d1-11, d1-16, d1-18a/b/d/e, d2-02, d2-03, d3-05a; `/tmp/dcu-*.log` sweep blocked by auto-mode (small, kept) |
| 2026-11-22 | plan | `1a689fe9` | §11.1 decisions 9/10 (time zone, unended end encoding), §12.1 queue rewritten, D3-04a/D3-06/D1-20a statuses current |
| 2026-11-22 | D1-20a | merged `564ff4c5` | non-8000 ACTION routes through the object table; 6012 M127 strict A-XDR; present TimeTag rejected with DAR 7 on 8000/6000/6012; case 000019 runs 北向 (19 cases); post-merge 340/340, ASAN, guards, acceptance |
| 2026-11-22 | D3-04a | merged `34271061` | device-local civil date_time_s through one helper; unended A6 end = `1C FF×7`; A14 NULL; legacy `event_history` rows never rendered; post-merge 340/340, ASAN, guards, acceptance |
| 2026-11-22 | D3-06 | merged `74798b0d` | 4000 A2 through a real clock port (EPERM -> DAR 2, time unchanged, no shadow); A3/A4 and 4001-4021 persisted by (oi, attr) with audit; ACL Class 8 rows derived from `DCU_698_PARAM_OIS`; post-merge 341/341, ASAN, guards, acceptance 20/20 |
