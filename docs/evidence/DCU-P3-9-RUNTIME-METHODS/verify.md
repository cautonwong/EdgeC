# DCU-P3-9-RUNTIME-METHODS verification

Branch `dcu/P3-9-runtime-methods`, base `40d8dca5`, final tip `dab16cd5` (test `42b4107f`, fix `88005fcf`).

## Spec and independent derivation

1. Business rule: the runtime class method definition must equal the 7.3 class table's real set, not `1..max`. Class 7 and Class 24 have no method 3; Class 9 has no method 6.
2. Invariants: every defined method keeps the same behavior; `k_class_max_method` max values stay the maximum of each real set; OI-specific methods (127+, F001 9/10) keep their existing path.
3. Observable: `dcu_698_class_defines_method(class, method)` is false for the holes and true for every defined number; an undefined class method reaches resolver DAR 4 (`UNDEFINED`), while a method missing from the ACL still returns DAR 3 first.
4. Acceptance: the new object-model test asserts false for Class 7 method 3, Class 9 method 6, Class 24 method 3 and true for their defined numbers; the existing class 7/9/24 tests stay green.
5. Non-goals: changing ACL advertising (it already excludes the holes); adding new methods; changing the catalog generator (P3-8).

Independent derivation: the expected sets come from `docs/698_parsed.md` tables 142/148/193 and match the P3-8 `CLASS_METHODS` table. The runtime table max and absent bitmaps are arithmetic from those sets: Class 7 {1,2,4,5}, Class 9 {1,2,3,4,5,7,8}, Class 24 {1,2,4,5}. The expected DAR for an undefined class method is 4 per the resolver's DAR rules, not 3; 3 is only the ACL's earlier refusal.

## Red / green

- Red at `42b4107f` (test-only): the build fails with an implicit declaration of `dcu_698_class_defines_method`; the dev's behavioral red also shows the old code returning `-21 != -23` for 3000/3005 method 3.
- Green at `dab16cd5`: `test_dcu_698_object_model` 1/1 (26 cases), `ctest -R 'dcu|dlt698'` 45/45 (104.86s), catalog 5/5.

## Mutation guard

`DCU-P3-9-class-method-holes`: mutated test rc=8 / restored rc=0, 1/1 sensitive.

## Acceptance

Three consecutive rounds: `全部合格:2 个验收任务`; `dcu_northbound` 35 passed in 22.51 / 22.02 / 17.00s; `dcu_acceptance` exit 0 each round.

## Review

Small table and predicate change; no formal adversarial review (not a parser or lifecycle change). The runtime max/absent values were cross-checked against the P3-8 `CLASS_METHODS` sets, and the mutation guard pins the holes.
