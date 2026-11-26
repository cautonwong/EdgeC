# DCU-D1-21：移除 698 ACL 对象数上限（60 → 224）

状态：交 `dcu-verifier` 复核。基线 `feat/dcu 09827718`，已普通 merge `feat/dcu ad9dd528`（含 D3-08），分支 `dcu/D1-21-acl-cap`。
Operator 2026-11-24 决策：去掉 ACL 60 项上限。

## 1. 目标集合 → 编码字节 → 选值

现有 ACL 60 项（49 个固定行 + 11 个参数 OI）。按目录统计计划发布集合：

| 集合 | OI 数 |
| --- | --- |
| 现有 ACL | 60 |
| Class 13 非 8000（8103–8108） | 6 |
| Class 22 / 25 / 26 | 23 |
| Class 2/5/11/12/14/15/16/17 已登记 OI | 129（去重后净增 112） |
| **目标 N（去重后）** | **201**（净增 141） |

编码测量：现有 60 项对象列表实际编码 **1970 字节**（临时在 appconn 测试打印 `len`，
随后删除）。按 32.83 B/对象投影：201 项 ≈ 6599 B，224 项 ≈ 7354 B。

选值：
- `EDGE_SLAVE_ACL_MAX_OBJECTS` 60 → **224**（覆盖 201 目标，余 23 项）。
- `EDGE_SLAVE_AXDR_MAX` 2048 → **10240**（224 项投影 7354 B，余 2886 B ≈ 39%）。
- `DRIVER_PRIV_SIZE` 1024 → **4096**：`dlt698_session_t` = ACL 对象 16×224=3584 B +
  会话/ACL 头约 40 B ≈ 3624 B，静态断言保证 ≤4096。
- `driver_manager_storage_t.raw` 65536 → **147456**（32 个实例槽 × 4096 = 131072 B +
  驱动表/锁/配置余量）；`NORTH_DRIVER_ADAPTER_STORAGE_BYTES` 70000 → **163840**
  （内含上述 manager storage）。

A-XDR 数组长格式：对象数 >127 时编码 `0x01 | 0x80|nbytes | 大端长度`；`_acl_parse`
两侧同步支持（1–2 字节长度，超出按解析失败处理）。60 项仍走单字节短格式，既有字节不变。

## 2. RAM 实测与预算

- `check_component_budgets.py`：adapters/io_drivers RAM 77824 → **147987 B**（+70163）。
  `adapters/io_drivers/component.meta` `ram_bytes_max` 更新为 163840（实测 + 余量），
  理由：operator 批准 ACL 上限；每实例 `DRIVER_PRIV_SIZE` 1024→4096，32 槽静态增量为
  32×3072=98304 B，其余为 manager/north adapter storage 调整。
- `check_stack_usage.py` clean：north adapter storage 在产品里是 static/heap 成员，
  未进入调用栈。

## 3. 受保护头登记

`foundation/include/edge_slave_plane.h` 改动已登记 `contracts/schema/core_readonly_whitelist.txt`：
条目写明 operator 2026-11-24 决策与 D1-21。（该头不在 `check_core_readonly.sh` 的
受保护清单内，登记是仓库规则要求，脚本不会因此报错。）

## 4. 测试

- `test_slave_parses_long_form_acl_224`（tests/test_driver_dlt698.c）：构造 224 项、
  长格式长度（0x81 0xE0）的对象列表；GET 列表末项 0x0002 A2 进入 resolve（回 DAR 6），
  列表外 0x0003 回 DAR 3 —— 证明长格式解析成功且 ACL 查找生效。
- `test_slave_rejects_over_cap_object_list`：上限改为 224 后，225 项列表仍回 DAR 3。
- 既有驱动/产品对象列表测试与 60 项验收行为不变；测试会话缓冲从 1024/4096 调整只为
  容纳新的 `DRIVER_PRIV_SIZE`。
- 静态断言保持：`sizeof(dlt698_session_t) <= 4096`；`OBJECT_COUNT <= ACL_MAX`。
- **test_archive_config_short_buffer_dar2**：40 只表 + `buf[64]` 调
  `dcu_698_archive_encode_axdr(0x60000200)` → `EDGE_SLAVE_SRC_TEMPORARY`（DAR 2），
  证明扩容后“再超缓冲仍不拿计数冒充配置”的行为有可执行守卫。

## 5. 门禁

- clean Release build warning 0（merge 后重建）；`ctest -R 'dcu|dlt698'` 44/44。
- ASAN `detect_leaks=1` 3/3 零泄漏；fanalyzer 3 known / 0 new；SRP 干净；guard 110/110；
  component budgets clean；stack usage clean。
- 验收：`acceptance/run.sh dcu` 2/2，3 + 34 passed / 60.42 s，无 DCU 残留。
  验收测试同步 D1-21 的两个可观察变化：
  1. `_exchange` 从单次 `recv(1024)` 改为按长度域收满整帧（否则 6000 A2 的 3.4KB
     应答被截断）；
  2. 6000 A2 在 10240 缓冲下能完整编码 40 只表配置，断言从“缓冲不足 DAR 2”改为
     “array<40> 真实配置”；超缓冲仍回 DAR 2 的产品行为未改。

## 6. 未决项

- 本单不新增业务 OI 到 k_objects（后续 P4/P5 单做）。
- 目标集合 201 是目录当前登记数；若 P4/P5 新增 OI 超过 23 项余量，需再次评估 N/AXDR。
- 每对象上限仍是 16 属性 / 8 方法（`DLT698_ACL_MAX_ATTRS/METHODS`），本单未改。
