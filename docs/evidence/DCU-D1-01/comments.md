# DCU-D1-01 — comment audit

`pstack:comment-sicko` reviewed the generator, the guard selftest, and the
generated-document template. No `HOLD`s.

## Deleted / rewritten

- Killed the ticket-narration header above `ClassDecision`; kept the OCR reasons
  (201E..202C, 4022) and the class-11-vs-8 rule that the decisions rely on.
- Killed the docstring on the conflict-audit helper (restated the name).
- Killed the "DCU-D1-01 reconciled… / four deferred / not unexplained drift"
  paragraph in the catalog header; kept only the column semantics.
- Reconciliation template: removed ticket narration and the two hardcoded
  "four" counts. The deferred count is now interpolated.
- Killed the selftest block comment that restated the mutation and assertion.

## Code findings fixed

- **Tautological gate branch.** Non-deferred decisions were auto-explained for
  any parsed value because `infer_class` returned the decision class. Decisions
  now record the printed IC (`parsed`) and the chosen class (`model`); the gate
  rejects a decision whose recorded `parsed` no longer matches the text, and a
  decision for an OI missing from the parsed table. Probes for decided,
  deferred, OCR-corrected, and undecided rows all fail correctly.
- **Overloaded tuple.** Replaced with `ClassDecision(parsed, model, source,
  deferred)`; prose that explained the overloading is gone.
- **Duplicate function.** `reconciliation_path` was defined twice; one remains.
- **Hardcoded counts.** "four" removed from both templates; computed instead.
- **Guard fixture copy loop.** The conflict fixture now copies only the parsed
  document, which is all the gate reads, instead of duplicating the five-file
  copy.

## Open

- The four deferred OIs are a product/standard mismatch, not a catalog defect:
  6000, 6002, 6003, 8000. Separate defect for the object table.
