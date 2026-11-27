# DCU-P3-6-ZERO-SEQ-OI verification

Branch `dcu/P3-6-zero-seq-oi`, base `40d8dca5`, final tip `a92cfeba` (red test `d9943a37`, stop evidence `e21bfff0`, deletion `fba72862`).

## Spec and independent derivation

1. Business rule: an OAD macro must name a quantity the standard defines at that OI/attribute. `docs/698_parsed.md:6652` (table A.3) and `:5752` (table 204) define OI 2002 attribute 2 as voltage angle, long-unsigned, degree, scale -1. The standard text has no zero-sequence current OI.
2. Invariants: no object row, ACL row or scheme references the removed OAD; the generated catalog artifacts stay byte-identical; the rule seed table and its tests stay consistent.
3. Observable: `OAD_ZERO_SEQ_CURRENT` no longer exists; the rule seed row and the tests that referenced it are gone; the north-source name assertion is gone; the catalog guard passes.
4. Acceptance: `python3 -I tests/test_dcu_698_router_oad_catalog.py` fails when the macro exists and passes when it is absent; the OAD OIs all resolve in the catalog; `ctest -R 'dcu|dlt698'` 45/45.
5. Non-goals: deriving zero-sequence current from 2001 A4 (neutral current, IN=3I0) or inventing a private OI; the adjacent 3006 comment, script hardcodes and `OAD_REACTIVE_ENERGY` naming are follow-ups.

Independent derivation: the standard evidence makes the alias wrong; with no consumer and no standard OI, the only honest fix is deletion. The expected behavior is absence of the macro and its seed row, and a guard that fails if either returns. The decision is recorded in the decision log (`p3-6-decision`).

## Red / green

- Red at `d9943a37` (test-only): `test_dcu_698_router_oad_catalog.py` fails with `OAD_ZERO_SEQ_CURRENT=0x20020200 resolves to OI 0x2002 ... table A.3 defines no zero-sequence current OI`.
- Green at `a92cfeba`: the same test 2/2, `ctest -R 'dcu|dlt698'` 45/45 (97.43s), generated artifacts unchanged (`--check` and sha256 identical), guard selftest all green with the new block.

## Guard registration

`tests/test_dcu_698_router_oad_catalog.py` is registered in `ci/scripts/run_guard_selftest.py`; the new block passes.

## Acceptance

Three consecutive rounds: `全部合格:2 个验收任务`; `dcu_northbound` 35 passed in 22.43 / 17.15 / 17.27s; `dcu_acceptance` exit 0 each round.

## Review

Deletion plus test/doc updates; no formal adversarial review. The boundary expanded to `dcu_698_rules.c`, the rule/scheme/north-source tests and the OAD status doc to remove the only production reference. Follow-ups logged: `dcu_soe_event_engine.h:24` 3006 comment, `scripts/parse_*` hardcodes, `OAD_REACTIVE_ENERGY=0x0020` naming.
