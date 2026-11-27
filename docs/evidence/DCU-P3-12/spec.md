# DCU-P3-12 五栏规格：4000 A2 SET 失败 DAR 契约与实现对齐

## 1. 业务规则

针对 DL/T 698.45 `SET 4000 A2`（时钟 `date_time_s`）：

| 场景 | 条件 | 返回 DAR | 时钟状态 |
| --- | --- | --- | --- |
| (a) 授权且写成功 | 会话 ACL 允许写 + A-XDR `date_time_s` 合法 + `g_clock->set` 返回 0 | **DAR 0**（成功） | 系统/时钟端口时间更新为新值 |
| (b) 授权但时钟不可用或写失败 | 会话 ACL 允许写 + A-XDR `date_time_s` 合法 + 时钟端口缺失（`!g_clock \|\| !g_clock->set`）或 `g_clock->set` 返回非 0（如 `EPERM`/`EINVAL`） | **DAR 2**（`EDGE_SLAVE_SRC_TEMPORARY` / 暂时失效） | 时钟保持不变 |
| (c) 授权但载荷畸形 | 会话 ACL 允许写 + A-XDR 载荷非法（非 `date_time_s` tag、长度不足、尾随字节或日历字段越界） | **DAR 7**（`EDGE_SLAVE_SRC_TYPE_MISMATCH` / 类型不匹配） | 时钟保持不变 |
| (d) 未授权客户端 | 会话 ACL 拒绝写（对象/属性不在当前会话可写 ACL 中） | **DAR 3**（`DLT698_DAR_DENIED` / 拒绝读写） | 时钟保持不变 |

## 2. 不变量

- 实现（`cmd/dcu/dcu_698_param.c`、`adapters/io_drivers/src/driver_dlt698.c`）、单元测试（`tests/test_dcu_698_param.c`、`tests/test_driver_dlt698.c`）、北向验收条款（`acceptance/dcu/tests/test_dcu_northbound.py` `D_00_00_02_000022` 与文件头判据说明）以及设计文档（`docs/design/DCU_698_OBJECT_MODEL.md`）对 `SET 4000 A2` 的 DAR 映射（`0 / 2 / 7 / 3`）完全一致。
- 时钟写失败时严禁回退到假 RAM 影子值，必须诚实返回 DAR 2 且 `GET 4000 A2` 仍返回原时钟值。

## 3. 可观测输出

1. `SET-Response` APDU（`0x86 0x01 piid 0x40 0x00 0x02 0x00 DAR 0x00`）中的 DAR 字节（`res[7]`）。
2. 紧随其后的 `GET 4000 A2` 响应中的 `date_time_s` 8 字节（`0x1C` + 7 字节本地公历时间）。

## 4. 验收判据

1. **参数单元测试（`tests/test_dcu_698_param.c`）**：
   - 合法 `date_time_s` 且 `clock->set` 成功 → 返回 `0`（DAR 0），`GET 4000 A2` 读回新时间。
   - 时钟端口缺失或 `clock->set` 失败 → 返回 `EDGE_SLAVE_SRC_TEMPORARY`（DAR 2），`GET 4000 A2` 读回原时间不变。
   - 非法/畸形 `date_time_s` 载荷（如错误 tag、越界字段、尾随字节）→ 返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`（DAR 7），`GET 4000 A2` 读回原时间不变。
2. **协议层 ACL + 帧级单元测试（`tests/test_driver_dlt698.c`）**：
   - 授权会话下 `SET 4000 A2` 合法载荷且时钟成功 → `SET-Response` DAR 0，GET 读回新时间。
   - 授权会话下 `SET 4000 A2` 合法载荷但时钟失败 → `SET-Response` DAR 2，GET 读回原时间。
   - 授权会话下 `SET 4000 A2` 畸形载荷 → `SET-Response` DAR 7，GET 读回原时间。
   - 未授权会话（ACL 不含 `4000 A2` 写权限）下 `SET 4000 A2` → `SET-Response` DAR 3，GET 读回原时间。
3. **北向验收用例（`acceptance/dcu/tests/test_dcu_northbound.py`）**：
   - 文件头判据第 6 条与 `D_00_00_02_000022` 的 `clause` 及断言改为精确契约：合法 `date_time_s` 时 DAR 0 必须真改时钟，时钟写失败回 **DAR 2**（不再写 `2/3`）且时间不变；并覆盖畸形载荷回 **DAR 7** 且时间不变。
4. **设计文档（`docs/design/DCU_698_OBJECT_MODEL.md`）**：
   - §8.1 更新为已对齐的 `0 / 2 / 7 / 3` 契约，移除“验收条款待修（另立缺陷评估）”的旧状态描述。

## 5. 非目标

- 不在 `clock->set` 失败时增加 RAM 影子时钟回退。
- 不修改 `modules/edge_core` 或受保护的 foundation 头文件。
