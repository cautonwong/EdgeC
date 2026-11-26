# DCU-D2-05a-r：dcu_698_axdr.c SRP 拆分

## 目的

`check_single_responsibility.py` 在合并后的 `feat/dcu` 报 `cmd/dcu/dcu_698_axdr.c` 1775 行超过 1500 行上限，导致门禁红。D2-05a 的 6016-601B codec 是最后一段增量。本单只做行为不变的拆分，不改公共头、不改逻辑。

## 拆分

| 文件 | 内容 | 行数 |
| --- | --- | --- |
| `cmd/dcu/dcu_698_axdr_internal.h`（新） | `axdr_cursor_t` 与共用的 take/put 原语（`static inline`）；`dcu_698_axdr_take_ms` 声明 | 83 |
| `cmd/dcu/dcu_698_scheme_axdr.c`（新） | 6016/6017/6018/6019/601A/601B codec | 517 |
| `cmd/dcu/dcu_698_axdr.c` | 6012/6013、6014/6015、action/param/archive codec 与分派 | 1202 |

- 共用的 MS 解码 `_decode_ms` 改为非 static 的 `dcu_698_axdr_take_ms`，6014 与 6016 仍只有一份实现。
- 共用原语移入内部头，两个 codec 文件各自内联，避免复制第二份 tag 解析。
- `cmd/dcu/CMakeLists.txt` 增加 `dcu_698_scheme_axdr.c`。

## 行为不变证据

- `check_single_responsibility.py`：SRP clean，函数均 ≤ 200 行。
- `run_guard_selftest.py`：105/105 PASS（含 SRP 与 live 检查）。
- 受影响 15 个 ctest 目标全过；`test_dcu_698_scheme_mgmt` 的精确字节向量覆盖 6017/6019/601B 编解码。
- ASAN `detect_leaks=1`：scheme_mgmt / schemes / axdr 三个测试 exit 0，0 leak 报告。
- `acceptance/run.sh dcu`：两个任务全过（acceptance 3 例、northbound 25 例）。
- `check_build_warnings.py`：0 violations。

## 未决项

- 无。UBSAN 不在本轮 verifier 的验证清单内，未跑；拆分是纯移动，行为面由上述用例覆盖。
