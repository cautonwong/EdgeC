# DCU-D0-05 comment review

Tool: `pstack:comment-sicko` (no-comments), model opus, scope `main...ac239d2e`.

## Result

- Files edited: `tests/test_dlt698_conformance_suite.c`, `tests/CMakeLists.txt`. Comment-only.
- Deleted 12 comment blocks, 26 comment lines, and 7 blank lines: section banners, `/* SA */` / `/* CA */` restatements, assertion narration, and the CMake scope narration.
- Kept PDF page and appendix citations, vector-construction rationale, FCS/HCS rules, DAR meaning, and the two skip reasons (HCS, Record/RSD).
- No code, assertion, vector byte, skip condition, or test wiring changed by the reviewer.

## HOLD resolved

The reviewer left `/* 对象未定义 */` on `.dar = 5` as a HOLD because the standard and the design doc disagreed.

`docs/698.pdf` 6.3.3.11 表34 is explicit:

| DAR | 含义 |
| --- | --- |
| 4 | 对象未定义 |
| 5 | 对象接口类不符合 |
| 6 | 对象不存在 |
| 7 | 类型不匹配 |

The constructed partial-failure vector uses a valid OAD whose instance is absent, so the correct DAR is 6. The vector now uses `.dar = 6` (`对象不存在（表34：6）`) and the expected byte is `0x06`. The design doc DAR table was corrected in the same change. The existing `tests/test_dlt698_get_list.c` already used 6 for the same case.

`D0-05b` tracks the code-side alignment: the driver and adapter should distinguish 4 (OI not defined) from 6 (instance absent) instead of falling back to one value.

## Verification after the fix

- `cmake --build build -j$(nproc)` exit 0.
- `ctest -R dlt698` 6/6 passed.
- Conformance suite 21 cases, 19 passed, 2 skipped (HCS, Record/RSD).
- `run_guard_selftest.py --root .` passed before the comment-only edit; the edit does not touch a guard.
