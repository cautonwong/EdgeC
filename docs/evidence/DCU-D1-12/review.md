# DCU-D1-12 评审材料：`edge_slave_source_ops_t` 追加字段兼容性

## 结论

仓库内**没有位置初始化（positional initializer）**。14 个直接初始化点全部使用
designated initializer，另外 6 处只做整结构拷贝或指针转传。因此把
`resolve_axdr` / `accept_axdr` 追加到 `edge_slave_source_ops_t` **末尾**是源码兼容的：
未指定成员按 C 标准零初始化为 NULL，现有初始化点无需改动，新路径自然回落标量。

前置条件：**只追加，不插入、不重排**。若未来出现位置初始化，或字段被插到中间，
必须在同一改动里迁移全部初始化点。该规则已写入 ADR-0013 与
`docs/design/slave_plane_architecture.md` 3.3。

## 直接初始化点（14）

| # | 位置 | 初始化方式 |
| --- | --- | --- |
| 1 | `tests/test_modbus_write_guard.c:369` `k_accept_ok` | designated `.accept` |
| 2 | `tests/test_modbus_write_guard.c:370` `k_accept_reject` | designated `.accept` |
| 3 | `tests/test_dlt645_codec.c:296` `src` | designated `.resolve` / `.accept` |
| 4 | `tests/test_dlt645_codec.c:330` `src` | designated |
| 5 | `tests/test_dlt645_codec.c:360` `src` | designated |
| 6 | `tests/test_protocol_loopback.c:117` `s_src` | designated |
| 7 | `tests/test_driver_iec104.c:205` `k_slave_src` | designated `.resolve` / `.enumerate` |
| 8 | `tests/test_driver_iec104.c:560` `k_slave_src_with_close` | designated `.resolve` / `.enumerate` / `.close_link` |
| 9 | `tests/test_iems_full_stack.c:198` `s_iec_src` | designated |
| 10 | `tests/test_iems_full_stack.c:205` `s_mb_src` | designated |
| 11 | `tests/test_cia402.c:78` `s_slave` | designated `.resolve` / `.accept` |
| 12 | `cmd/dcu/dcu_north_adapter.c:120` `src` | designated `.resolve` / `.accept_action` |
| 13 | `cmd/iems/iems_north_adapter.c:138` `ops` | designated `.resolve` / `.accept` |
| 14 | `cmd/iems/iems_north_adapter.c:240` `ops` | designated `.resolve` / `.accept` / `.enumerate` / `.deadband` |

## 拷贝或转传点（6）

| 位置 | 行为 |
| --- | --- |
| `cmd/iems/iems_north_iec104.c:116` | `edge_slave_source_ops_t ops = *iems_north_iec104_source_ops();` 后补 `ops.close_link`；整结构拷贝 |
| `modules/edge_session/src/session_manager.c:26` / `:59` | 成员变量；运行期 `*ops` 赋值或 `memset(0)` |
| `modules/edge_io/src/engines/slave_engine.c:150` | 只接收并转传指针 |
| `modules/edge_io/src/bus_orchestrator.c:122` | 只接收并转传指针 |
| `tests/test_iems_iec104_north.c:177` | 取现有 ops 指针 |
| `include/bus_engine.h:33` | 仅函数指针签名 |

## 二进制 ABI

本仓库产品、驱动、测试一起编译，无跨版本共享对象；结构体只在进程内按指针传递，
不进入共享内存或持久化格式。`sizeof` 增大不影响现有调用方。若未来有外部插件按旧
布局编译，需要版本号或能力位，本 ADR 不覆盖。

## 命令输出

见 `tests.log`。核心命令：

```bash
grep -rn "edge_slave_source_ops_t" --include=*.c --include=*.h .
python3 ci/scripts/run_guard_selftest.py --root .
git diff --check
```
