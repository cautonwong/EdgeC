# DCU-D0-05b 证据包：DAR 对齐 DL/T 698.45 6.3.3.11

- 分支：`dcu/D0-05b-dar`
- 起点：`15a51c80`（main）
- 提交：`fix(DCU-D0-05b): align 698 DAR values with 6.3.3.11`
- 性质：状态值修正 + 缺失应答路径，不改对象表。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `foundation/include/edge_slave_plane.h` | 新增从站取数回调的失败原因码 `EDGE_SLAVE_SRC_*`；驱动据此映射 DAR，旧 `-1` 按 NOT_FOUND 兼容。 |
| `foundation/include/dlt698_codec.h` / `foundation/src/protocol/dlt698_codec.c` | 新增 `dlt698_encode_get_response_dar`（`85 01 PIID OAD 00 DAR 00 00`）。 |
| `adapters/io_drivers/src/driver_dlt698.c` | 失败原因 → DAR 映射；GET-Request-Normal 不再静默，回 DAR 应答；NormalList 逐项用同一映射。 |
| `cmd/dcu/dcu_698_axdr.c` | 取数失败返回具体原因：对象不存在 / 类型宽度不匹配；`accept_axdr` 返回暂时失效。 |
| `tests/test_dcu_698_axdr.c` | 每个 DAR 路径一条字节级断言；缓冲不足改为断言 DAR 应答。 |
| `tests/test_dlt698_conformance_suite.c` | 新增 GET-Response-Normal DAR 字节向量。 |
| `cmd/dcu/README.md` | 删除“GET Normal 静默超时”的过期说明，改为按 DAR 应答。 |

未改对象表内容，未新增 OI。

## 失败原因 → DAR

| 失败原因 | 源返回码 | DAR |
| --- | --- | --- |
| 成功 | 0 | 0 |
| 对象不在对象表 | `EDGE_SLAVE_SRC_NOT_FOUND`（-20）或旧 -1 | 6 OBJECT_NOT_FOUND |
| OAD 接口类与对象不匹配 | `EDGE_SLAVE_SRC_CLASS_MISMATCH`（-21） | 5 OBJECT_CLASS_MISMATCH |
| A-XDR 类型/宽度不匹配 | `EDGE_SLAVE_SRC_TYPE_MISMATCH`（-22） | 7 TYPE_MISMATCH |
| OI 在标准/目录中未定义 | `EDGE_SLAVE_SRC_UNDEFINED`（-23） | 4 OBJECT_UNDEFINED |
| 不支持/方法失败/暂时失效 | `EDGE_SLAVE_SRC_TEMPORARY`（-24）或 ACTION 非 0 | 2 TEMPORARY_FAILURE |

- GET-Request-Normal 现在对未知 OAD 回 `85 01 PIID OAD 00 DAR 00 00`，主站不再等超时。
- NormalList 逐项沿用同一映射，行为与 Normal 一致。
- 标量路径的 `_encode_axdr_value` 失败（类型无编码器）也回 DAR=7。

## 完成定义逐项

- [x] 使用 `DLT698_DAR_*` 常量，无裸 DAR 字面量。
- [x] 五类失败原因映射到 6/5/7/4/2；成功 0。
- [x] GET-Request-Normal 未知 OAD 回 DAR 应答。
- [x] NormalList 保持逐项 DAR。
- [x] 测试每条 DAR 路径都有字节级断言；现有真实帧向量全绿。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | exit 0，0 warning |
| `cd build && ctest --output-on-failure -R 'dcu\|dlt698'` | 28/28 通过 |
| `./build/tests/test_dcu_698_axdr` | 10/10 通过 |
| `./build/tests/test_dlt698_conformance_suite` | 21 passed，1 skipped（既有 GetRecord 未实现） |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| `./test.sh --omnis dcu` | dcu-acceptance 2 passed、dcu-northbound 4 passed、退出码 0 |
| `git diff --check` | exit 0 |

## 证据文件

- `summary.md`：本文件
- `tests.log`：构建、ctest、两个测试、守卫、`git diff --check`
- `runtime.log`：`./test.sh --omnis dcu` 完整输出
- `diffstat.txt`：代码提交的 `git show --stat`
- `comments.md`：`/no-comments` 审计结果
