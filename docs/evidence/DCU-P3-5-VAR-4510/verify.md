# DCU-P3-5-VAR-4510 verification

Branch `dcu/P3-5-var-4510`, base `40d8dca5`, final tip `7368b1b2` (test `aff418ba`, fix `c74f8ffd`).

## Spec and independent derivation

1. Business rule: the scalar port (`dcu_698_value_for_oad` -> `dcu_north_resolve_scalar`) carries only scalar values. `dcu_value_t` has an `int64` raw, a width of at most 4 bytes on the port, and no octet-string tag or encoder. 4510 A5 is an octet-string(6) MAC, so the scalar port has no representation for it.
2. Invariants: the main-station GET path (`resolve_axdr`) still returns octet-string(6) with the MAC; scalar attributes keep their values; nothing is truncated.
3. Observable: `dcu_698_value_for_oad(0x45100500)` returns `EDGE_SLAVE_SRC_CLASS_MISMATCH` (-21, DAR 5); the pre-fix code returned `EDGE_SLAVE_SRC_TYPE_MISMATCH` (-22, DAR 7).
4. Acceptance: the 4510 A5 scalar-path test expects -21; the main GET test still returns 0 with six MAC bytes.
5. Non-goals: adding octet-string support to the scalar port; 4511-4517 producers; a derived zero-sequence point.

Independent derivation: the scalar port cannot carry six bytes by its own contract, and `DCU_698_DEVIATIONS.md` maps a defined item without a source to DAR 5. The expected code is therefore 5, not 7; the main GET path is unchanged because it does not use the scalar port. This comes from the contract and DEVIATIONS, not from the implementation.

## Red / green

- Red at `aff418ba` (test-only): `ctest -R '^test_dcu_698_var$'` -> `-22 != -21` at `test_dcu_698_var.c:334`.
- Green at `7368b1b2`: focused 1/1; `ctest -R 'dcu|dlt698'` 45/45 (105.46s).

## Mutation guard

`DCU-P3-5-VAR-4510-scalar-4510-a5`: mutated test rc=8 / restored rc=0, 1/1 sensitive.

## Acceptance

Three consecutive rounds: `全部合格:2 个验收任务`; `dcu_northbound` 35 passed in 21.56 / 16.87 / 22.22s; `dcu_acceptance` exit 0 each round.

## Review

Four-line DAR fix; no formal adversarial review (not a parser, security or lifecycle change). The independent derivation and the mutation guard cover the changed behavior. The main-station GET path is covered by the existing octet-string test.
