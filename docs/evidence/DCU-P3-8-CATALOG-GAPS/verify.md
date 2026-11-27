# DCU-P3-8-CATALOG-GAPS verification

Branch `dcu/P3-8-catalog-gaps`, base `9d065b6d`, final tip `b86af731`. Tooling-only unit: generator, per-OI method validation, new Python guard, guard-selftest registration. No product C code changed.

## Spec and independent derivation

1. Business rule: the generator must reject any 7.3 class method table whose method-number set differs from the recorded standard set; per-OI method numbers below 127 must belong to that class's 7.3 set or the recorded `F001` {9,10} exception.
2. Invariants: the real `docs/698_parsed.md` still parses 26 classes; generated catalog outputs stay byte-identical; no class method set is inferred from its maximum.
3. Observable outputs: generator exit code, stderr text (`missing`, `extra`, `duplicate`), generated file bytes, catalog-check message.
4. Acceptance criteria: removing Class 7 method 5 -> non-zero plus `missing 5`; adding Class 7 method 3 -> non-zero plus `extra 3`; changing OI 4307 method 127 to 126 -> non-zero plus `4307` and `126`; real source -> exit 0 and byte-identical outputs; repository `--check` -> exit 0.
5. Non-goals: runtime `k_class_max_method` semantics in `cmd/dcu` (Class 7/24 method 3 and Class 9 method 6 still advertised by max; follow-up unit); automatic synchronization when the source standard is revised.

Independent derivation: the expected sets come from the 7.3 tables as recorded by the plan gap, not from the implementation. Class 7 and Class 24 have methods {1,2,4,5} with no 3; Class 9 has {1,2,3,4,5,7,8} with no 6. The negative fixtures remove or add a source method line and require rejection from the business rule; the byte-identical criterion follows from the invariant that a validator must not alter valid output.

## Red / green

- Red at `f953a10f` (test-only, old generator): `python3 -I tests/test_dcu_698_catalog_method_sets.py` exit 1 with the three expected assertions (missing 5, extra 3, OI 4307 method 126).
- Green at `b86af731`: 5/5; `test_dcu_698_catalog_source_window.py` 6/6; `extract_dcu_698_catalog.py --check --root .` exit 0; `check_dcu_698_catalog.py --root .` -> `catalog matches docs/698_parsed.md`; generated artifacts diff against the base is empty.

## Mutation guard

`DCU-P3-8-method-sets-max-mask`: mutated build rc=0, test rc=1; restored original bytes/hash, build rc=0, test rc=0; runner 1/1 sensitive. Raw log in `verifier-mutation.log`.

## Guard registration

`ci/scripts/run_guard_selftest.py` now runs `tests/test_dcu_698_catalog_method_sets.py` after the source-window block. The merged-tree guard selftest covers it in the batch gate.

## Acceptance

Tooling-only unit; product code unchanged. The dev ran one acceptance round (2/2 tasks, 35 passed); the merged-tree 3x acceptance covers the product unchanged.

## Out of scope

Runtime method masks in `cmd/dcu` still use `k_class_max_method`; Class 7/24 method 3 and Class 9 method 6 remain advertised at runtime. Logged as a follow-up unit in the plan.
