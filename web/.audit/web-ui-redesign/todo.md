# Web UI consistency run

Playbook: figure-it-out (large cross-cutting migration)

## Phase A: Frame
- [x] State definition of done as falsifiable predicate
- [x] Quantify scope and blockers
- [x] Choose rigor level
- [x] Present framing and tradeoffs (proceed on reversible work)

## Phase B: Design the workflow
- [x] Decompose into atomic landable units, riskiest unknown first
- [x] Build verification harness and capture pre-change baseline
- [x] Decide what fans out; one writer per file
- [x] Write the phase list down

## Phase C: Run the loop
- [x] Each unit: hypothesis, smallest change, measure vs predicate, keep/revert
- [x] Verify the artifact, never a self-report
- [x] Judge delegated work
- [x] Verdict per unit: VERIFIED / NOT VERIFIED / INCONCLUSIVE

## Phase D: Keep the audit trail
- [x] Log decisions via show-me-your-work
- [x] Commit trail with the diff

## Phase E: Verify and hand back
- [x] Check whole against Phase A predicate on real product
- [x] Encode recurring corrections as gates/lints/scripts
- [x] Reply: playbook, rigor, trail path, verified/open

## Concrete steps
- [x] S1 Create worktree, link deps, copy baseline screenshots
- [x] S2 Add token-derived Tailwind preset + RGB channel tokens
- [x] S3 Add DCU Tailwind pipeline; dedupe three configs
- [x] S4 Bundle CJK + mono + emoji subsets, wire @font-face, verify no tofu
- [x] S5 Theme store: system default, toggle, persistence, no-flash init
- [x] S6 Codemod legacy hex/classes to token vars; remove dead token files
- [x] S7 Translate page headings to match menu; remove decorative glow/claims
- [x] S8 Charts read theme tokens and react to theme changes
- [x] S9 Responsive shell: rail <1280, drawer <768, max-width 1600
- [x] S10 A11y: focus, dialogs, DataTable, icon names, chart text alternative
- [x] S11 Fix brand hover derivation and brand-colour contrast gate
- [x] S12 Verify: builds, all routes x both themes, mobile, keyboard, e2e
- [x] S13 Import ECharts modules on demand; update size budget
- [x] S14 Translate remaining visible chrome; keep domain abbreviations
- [x] S15 Compute DataCurves peak/trend from live buffers
- [x] S16 Re-verify after the follow-up sweep (builds, size, e2e, routes)

Notes:
- skip: brand-colour contrast gate for arbitrary product overrides; the two
  shipped brands were calculated by hand (iEMS 5.3:1, DCU 4.9:1 on white) and
  the tooling gate still covers the default palette.
- skip: full i18n message catalogs; page headings and primary chrome are
  Chinese, domain abbreviations stay English by intent.
