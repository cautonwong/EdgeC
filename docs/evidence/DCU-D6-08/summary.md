# DCU-D6-08 sanitizer and analyzer sweep

Commit under test: `6509c68f` (after D1-17, D1-01, D1-14a, D6-08a).
Commands run on the merged `main` tree, excluding only `app_platform_e2e` (needs the
privileged harness).

| Gate | Command | Result |
| --- | --- | --- |
| ASAN | `ctest --output-on-failure -E '^app_platform_e2e$'` in `build-asan` | 334/334, exit 0 |
| UBSAN | same in `build-ubsan` | 334/334, exit 0, 0 `runtime error` lines |
| TSAN | same in `build-tsan` | 334/334, exit 0, 0 `ThreadSanitizer` lines |
| GCC analyzer | `python3 ci/scripts/run_fanalyzer.py --build-dir build --jobs 4` | 3 known, 0 new |

The TSAN run uses `ci/tsan.supp` only on `test_dcu_node`, `test_dcu_runtime_health`, and
`test_dcu_mutex_marathon` (D6-08a). The suppression covers SQLite's internal open/close lock
order; a first-party two-mutex inversion is still reported, per the D6-08a A/B.

No new analyzer finding was added to `ci/baseline/fanalyzer.txt`; the three known entries are
unchanged.
