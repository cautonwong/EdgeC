# DCU-D1-14 证据包：698 SET-Request 路径

- 分支：`dcu/D1-14-set`
- 起点：`4ec3096b`（main）
- 提交：`feat(DCU-D1-14): implement 698 SET-Request path`
- 性质：新增 SET 分支与 SET-Response 编码，不改 GET/ACTION。

## 改了什么

| 文件 | 改动 |
| --- | --- |
| `adapters/io_drivers/src/driver_dlt698.c` | 新增 `_handle_set_request`：解 `SET_REQ`（0x06）的 OAD + A-XDR Data，优先 `accept_axdr`，NULL 才回落 `accept(double)`；回 SET-Response-Normal。 |
| `foundation/include/dlt698_codec.h` / `.c` | 新增 `dlt698_encode_set_response`（`86 01 PIID OAD DAR 00 00`）、`DLT698_DAR_DENIED`、`DLT698_TAG_BOOLEAN`；`dlt698_decode_data` 补 BOOLEAN/LONG64/FLOAT32/FLOAT64，让 SET 的 float/bool 参数能进产品。 |
| `foundation/include/edge_slave_plane.h` | 新增 `EDGE_SLAVE_SRC_DENIED`（→ DAR 3）。 |
| `cmd/dcu/dcu_698_axdr.c` | `dcu_north_accept_axdr` 解标量 A-XDR、按 OAD/属性校验可写与类型，调 `dcu_698_write`；只读→DENIED、未知 OAD→NOT_FOUND、类型不符→TYPE_MISMATCH。 |
| `tests/test_dcu_698_axdr.c` | 产品写入路径 4 例；驱动 SET 5 例（合法/缺数据/错类型/只读/未知 OAD + 标量回落）。 |
| `tests/test_dlt698_conformance_suite.c` | SET-Response-Normal 字节向量。 |
| `docs/adr/0013-698-slave-axdr-port.md` | 2.1 增加 `DENIED → 3`。 |
| `docs/design/DCU_698_OBJECT_MODEL.md` §5.3 | 枚举说明补 DENIED。 |
| `cmd/dcu/README.md` | SET-Request 状态改为已实现。 |

未改 `modules/`，未改对象表内容。

## DAR / 用例表

| 场景 | 源返回码 | DAR | 测试 |
| --- | --- | --- | --- |
| 合法写入并回读 | 0 | 0 | `test_product_accept_set_paths`（写入 2400 后 `dcu_698_read` 读回）、`test_set_response_dar_paths` |
| float32 写入到达 `accept_axdr` | 0 | 0 | `test_set_response_dar_paths`（0x17 参数） |
| 缺 Data | —（驱动判） | 7 TYPE_MISMATCH | `test_set_missing_data_dar7` |
| 错误类型标签 | `TYPE_MISMATCH` | 7 | `test_set_response_dar_paths`、产品 visible-string 用例 |
| 只读属性（mask 命中但无 write） | `DENIED` | 3 DENIED | `test_product_accept_set_paths`（2000 attr2）、`test_set_response_dar_paths` |
| 对象有 write 处理器但不支持该属性 | `CLASS_MISMATCH` | 5 OBJECT_CLASS_MISMATCH | `test_product_accept_set_paths`（F200 attr4） |
| 未知 OAD | `NOT_FOUND` | 6 OBJECT_NOT_FOUND | `test_product_accept_set_paths`、`test_set_response_dar_paths` |
| 标量回落（标量标签） | 0（`accept`） | 0 | `test_set_scalar_fallback`（含 long-unsigned 与 long64 -1） |
| 标量回落（结构标签） | —（驱动判） | 7 TYPE_MISMATCH | `test_set_scalar_fallback_rejects_structure` |
| SET-Response 字节 | — | 0 | `test_set_response_normal`（conformance） |

## 完成定义逐项

- [x] 驱动解 `SET_REQ`，OAD + 完整 A-XDR Data。
- [x] `accept_axdr` 优先，NULL 才回落标量；非 0 不回落。
- [x] SET-Response-Normal 由 codec 编码，驱动不手拼帧。
- [x] 复用 `EDGE_SLAVE_SRC_*`/`DLT698_DAR_*`；新增 `DENIED → 3` 并同步 ADR/设计。
- [x] 产品 accept 按属性类型写入/拒绝；无动态内存。
- [x] LONG64/DOUBLE_LONG 解码改用无符号累加后再转换，消除有符号左移 UB。
- [x] 每个 DAR 路径一条字节级断言；测试断言编码后的响应。

## 已知缺口（不在 D1-14 范围）

- F200 在本产品里把 attr2/attr3 建模为 baudrate/parity；按目录，attr2 是 `array`（端口参数数组）、attr3 是 `unsigned`（设备数）。这是既有产品画像与目录的偏差，D1-14 只负责 SET 通道，不改对象表/属性语义；需单独立项校正。

## 验证命令与结果

| 命令 | 结果 |
| --- | --- |
| `cmake --build build -j8` | exit 0，0 warning |
| `python3 ci/scripts/check_build_warnings.py --log <build.log> --root .` | No new violations |
| `cd build && ctest --output-on-failure -R 'dcu\|dlt698'` | 28/28 通过 |
| ASAN `ctest -R dlt698` | 6/6 通过 |
| `./build/tests/test_dcu_698_axdr` | 15/15 通过 |
| `./build/tests/test_dlt698_conformance_suite` | 22 passed，1 skipped（既有 GetRecord） |
| `python3 ci/scripts/run_guard_selftest.py --root .` | exit 0 |
| `./test.sh --omnis dcu` | 2 + 4 通过 |
| `git diff --check` | exit 0 |

## 证据文件

- `summary.md`：本文件
- `tests.log`：构建、warning gate、ctest、ASAN、两个测试、守卫、diff check
- `runtime.log`：`./test.sh --omnis dcu`
- `diffstat.txt`：代码提交的 `git show --stat`
- `comments.md`：`/no-comments` 审计
