# DCU-P3-1-ACL-CAP verification

Branch `dcu/P3-1-acl-cap`, base `9d065b6d`, final tip `04c3855c` (product code `a6605ba8`; later commits are tests, guard and docs only).

## Spec and independent derivation

1. Business rule: every attribute/method defined by the 7.3 class table for the held classes is advertised in the Class 20 ACL; a request that passes the ACL reaches the resolver; a defined item with no source returns DAR 5, a read-only SET returns DAR 3, and an item that is undefined or not advertised returns DAR 3 from the ACL.
2. Invariants: per-object ACL description 12 B; attribute rights cover 1..18 in 5 B; at most 12 method IDs per object; the session-level 1024-byte method pool rejects the whole ACL on overflow; `sizeof(dlt698_session_t)` stays within `DRIVER_PRIV_SIZE` 4096; object list stays within `EDGE_SLAVE_AXDR_MAX` 10240; response pool frame stays within 10496; 224 ACL objects.
3. Observable: GET/ACTION/SET DAR, Class 20 list frame length, session size, ACL accept/reject, resolver call count.
4. Acceptance: F001 methods 9/10 -> DAR 5, F000/F002 method 9 -> DAR 3; 4307-4309 methods 127-129 -> DAR 5; F100 methods 9-12 -> DAR 5, method 13 -> DAR 3; 2301-2308 A17/A18 GET -> DAR 5, SET -> DAR 3; 4300/127, F100/13, 2301/A19 -> DAR 3; object list frame 6014 B; session 3752 B; a source with 1200 method IDs -> DAR 3 and zero resolver calls; exactly 1024 IDs -> accepted with the last object's method offset exact.
5. Non-goals: Class 12 16/17 conflict; producers for the held items (they stay DAR 5); raising `DRIVER_PRIV_SIZE`; runtime method-set semantics (P3-8 follow-up).

Independent derivation: the expected DARs come from the standard tables and the two-layer DAR rules, not from the implementation. F001 methods 9/10 are in table A.11; 4307-4309 methods 127-129 are in table 178; F100 methods 1..12 are in table 184; 2301-2308 attributes 1..18 are in table 190. The ACL check runs before the resolver, so an advertised defined item yields the resolver DAR (5) and an unadvertised one yields 3. The pool bound is arithmetic: 224 objects x 12 methods = 2688 worst case exceeds 1024, so a 1200-ID source must be rejected entirely; the test asserts DAR 3 and zero resolver calls. The session size is 224 x 12 + 1024 + headers = 3752, within 4096.

## Independent red / green

- Red at `b7402a41` (test-only): 5 failures, the four held classes return `3 != 5`, object list `5828 != 6014`.
- Green at `04c3855c`: targeted `test_driver_dlt698` 1/1 and `test_dcu_698_appconn` 1/1; `ctest -R 'dcu|dlt698'` 45/45 (154.10s); `check_doc_references.py --root .` exit 0.

## Adversarial review

One finding: `test_slave_rejects_acl_method_pool_overflow` was a false green. It built 1200 method IDs but queried an attribute that was not advertised, so widening the pool bound to 2x still passed. Fixed by advertising attribute 2 on the first object and asserting DAR 3 plus zero resolver calls; the new mutation guard `DCU-P3-1-acl-pool-bound` widens the bound to 2x and the test now fails (red rc=8, green rc=0). Product code was not found defective on the attack surfaces: ACL parse bounds, fail-closed overflow, budget, DAR semantics and the existing guards. Docs `DCU_698_OBJECT_MODEL.md` and `DCU_698_DEVIATIONS.md` were synced to the new 12 B / attr 18 / method 12 / pool 1024 layout.

## Mutation guards

- `DCU-P3-1-acl-cap-method-pool`, `DCU-D4-07-esam-attr15-advertised`, `DCU-D4-09-group-attr16-advertised`: 1/1 sensitive each.
- `DCU-P3-1-acl-pool-bound`: red rc=8, green rc=0; raw log in `verifier/mutation-pool-bound.log`.

## Acceptance

Three consecutive rounds: `全部合格:2 个验收任务`; `dcu_northbound` 35 passed in 22.42 / 27.83 / 21.63s; `dcu_acceptance` exit 0 each round.

## Budgets

Session 3624 -> 3752 B (priv 4096, margin 344); object list 5806 -> 5992 B data, 5828 -> 6014 B frame; AXDR margin 4434 -> 4248; response pool frame margin 4668 -> 4482; 224/10240/4096 unchanged.

## Open items

Producers for the newly advertised items remain absent, so they return DAR 5. Class 12 16/17 stays HOLD. Runtime method-set semantics remain a P3-8 follow-up. ASAN on `test_driver_dlt698` runs in the merged-tree target set.
