# DCU-D0-05 真实帧向量

## 做了什么

`tests/test_dlt698_conformance_suite.c` 原来只断言常量与调用产品遥控接口，不解析任何报文（缺陷 D-09）。本次重写为字节级一致性套件：21 个用例，19 通过、2 跳过。每条用例都真的调用 codec 的编码或解析接口，并与标准里的字节逐位比对。

同时把该用例从 `dcu_app_lib` 上解耦，只链编解码层，不再依赖产品装配。

## 向量来源

权威来源是 `docs/698.pdf`。`docs/698_parsed.md` 只作检索，页码按 PDF 里印刷的页号（物理页 = 印刷页 + 4）。

| 用例 | 标准位置 |
| --- | --- |
| `test_fcs_algorithm_known_answer` | 附录 D（印刷第 191-192 页）；PPP FCS-16 检校值 `"123456789"` → 0x906E |
| `test_frame_known_answer_full_bytes` | 5.1.1 图6 / 5.1.2 / 5.1.5 / 5.1.7 + H.3.1（第 206 页） |
| `test_get_request_normal` | H.3.1（第 206 页）`05 01 01 40 01 02 00 00` |
| `test_get_response_normal` | H.3.1（第 206 页）`85 01 01 40 01 02 00 01 09 06 … 00 00` |
| `test_get_request_normal_list_decode` | H.3.2（第 206-207 页）`05 02 02 02 … 00` |
| `test_get_response_normal_list` | H.3.2（第 206-207 页）两项数组响应 |
| `test_get_response_list_partial_dar` | 6.3.8.2.2 表73 + 6.3.3.11 DAR；标准无失败样例，按结构构造 |
| `test_set_request_normal` | H.4.1（第 213 页）`06 01 02 40 00 02 00 1C … 00` |
| `test_set_response_normal_decode` | H.4.1（第 213 页）`86 01 02 40 00 02 00 00 00 00` |
| `test_action_request_normal` | H.5.1（第 214 页）`07 01 05 00 10 01 00 0F 00 00` |
| `test_action_response_dar0` | H.5.1（第 215 页）`87 01 05 00 10 01 00 00 00 00 00` |
| `test_action_response_dar2` | 6.3.3.11 DAR；标准无失败样例，按结构构造 |
| `test_action_response_with_data` | 6.3.3.12（Data OPTIONAL=01 后跟 A-XDR） |
| 五个畸形帧用例 | 5.1.1（起止字符）、5.1.2（长度域）、5.1.7（FCS） |
| `test_passthrough_unknown_choice` | 5.1 与 6.3.4：解析器按契约透出 CHOICE，由调用方判语义 |

整帧向量的帧头按本实现约定（1 字节 SA、CA=0x10、CTRL 0x43/0xC3），HCS/FCS 由测试内的独立实现 `orc_fcs16`（按附录 D 逐位算）核对，不与 codec 共用查表。

## 跳过项，不算通过

- `test_get_record_and_rsd_not_implemented`：GET-Request-Record（CHOICE [3]）与 RSD 未实现，指向 D1-15。附录 H.3.3 的冻结记录报文在实现后补向量。
- `test_reject_bad_hcs`：见下方缺陷。

## 发现的 codec 缺陷（未在本单修改生产代码）

**D0-05-D1 解析器不校验 HCS。** 5.1.5 要求校验帧头校验和，`dlt698_decode_frame_ex` 只算 FCS，不核对 HCS。实测：把 HCS 两字节改错、再按改后的字节重算 FCS，解析器返回 0（接受）。FCS 覆盖帧头，所以普通传输错误仍会被挡住；但按标准，HCS 缺失校验属于实现与 5.1.5 不一致，且会让"帧头被改、FCS 被同步伪造"的帧通过。

修法建议（单独立项）：在 FCS 校验之前加 HCS 校验，并在 `tests/test_dlt698_conformance_suite.c` 把 `test_reject_bad_hcs` 从 skip 改成断言返回值 -1。

**D0-05-D2 缺少三个编码入口。** `dlt698_codec.h` 没有 GET-Request-NormalList、SET-Response、GET-Request-Record 的构造函数。本单用"标准 APDU 装帧 + 解析"覆盖前两者的解析路径，并在测试注释里写明构造缺失；构造函数按 D1 系列补。

## 验证

| 命令 | 结果 |
| --- | --- |
| `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug` | 成功 |
| `cmake --build build -j8` | 成功 |
| `cd build && ctest --output-on-failure -R dlt698` | 6/6 通过（`runtime.log`） |
| `./build/tests/test_dlt698_conformance_suite` | 21 用例，19 通过、2 跳过（`tests.log`） |
| `python3 ci/scripts/run_guard_selftest.py --root .` | 全部守卫与正反夹具通过 |

证据文件：`summary.md`、`tests.log`、`runtime.log`、`diffstat.txt`。

## 改动范围

只改 `tests/test_dlt698_conformance_suite.c`、`tests/CMakeLists.txt`（去掉该用例对 `dcu_app_lib` 的依赖）与 `docs/evidence/DCU-D0-05/`。未改 `cmd/dcu/`、`foundation/`、`adapters/`、`modules/` 的任何生产代码。
