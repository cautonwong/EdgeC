# DCU-D4-08a：Class 22（F200–F20C）评估与首个切片

状态：交 `dcu-verifier` 复核。基线 `feat/dcu 6ad00780`，分支 `dcu/D4-08-class22`，
tip 见交回消息。只加对象广告与诚实 DAR，不造端口数据。

## 1. 评估：Class 22 有没有可信来源

结论：**没有完整可信来源**，按诚实 DAR 只广告、不伪造。

| 来源 | 内容 | 可用性 |
| --- | --- | --- |
| A.12（`docs/698_parsed.md` 9130–9335） | 类 22 范围 F200..F20C（F210 属类 8）；F200 A2 是端口对象列表数组（端口描述符 + COMDCB + 功能） | 只给出形状，不是产品数据 |
| `meter_inventory` | 每表地址/协议/波特率 | 只有 F201 语义的波特率，且是表计侧参数，不是端口配置 COMDCB |
| 产品参数表 | 无串口/红外/DI/继电器端口配置持久化 | 不可用 |

因此：A2（及其它 A.12 只读属性）可以按类广告，但 GET 没有可读来源 → **DAR 2**；
写路径没有落地实现 → 只读（SET DAR 3）。A.12 的 OI 专属方法 127/128 已计入定义
（resolver DAR 5，未实现；主站 ACTION 因 ACL 无方法回 DAR 3）。

## 2. 实现

- `dcu_router.h`：F200 注释改为 RS232 端口，新增 `OI_RS485_PORT 0xF201` … `OI_WIRELESS_PRIVATE 0xF20C`（F210 不并入）。
- `dcu_698_object_model.c`：13 个 IC_PERIPHERAL singleton 行，属性掩码按 A.12：
  - F200/F201/F202/F205/F207/F208/F20A/F20B：A2；
  - F203/F204/F206：A2 + A4；
  - F209：A2 + A5 + A6；F20C：A2 + A4 + A5。
- `dcu_698_axdr.c`：resolver 新增 Class 22 分支（`class_id == IC_PERIPHERAL`，按对象
  掩码回 DAR 2 / 5 / 4）；范围不在 resolver 里重复写死。
- `dcu_698_appconn.c`：12 个新 ACL 对象（只读 rights 1）。F200 既有行
  `k_attrs_f200 = {{2,1},{3,1},{4,3}}` **不变**，所以 F200 A4 SET 仍是 DAR 5。
- `dcu_698_object_model.c`：A.12 的 OI 专属方法计入 `_class_defines_method`
  （F200/F201/F202/F205/F207/F20B 的 127、F209 的 127/128）；`k_oi_max_attr` 登记
  F209→6、F20C→5，使 A5/A6 在掩码收紧后仍属类定义（DAR 5 而非 4）。
- `dcu_698_var.c`：删除 F200 的 A2/A3 标量描述（`VK_U32`/`VK_U8`，此前被掩码挡住、
  不可达）：A2 是端口对象列表（含 COMDCB），不是 uint32。
- 计数：对象表 74→**86**，ACL 60→**72**（61 手写行 + 11 参数行）；`dcu_698_object_count()`、
  对象列表长度、设计文档同步更新。
- 对象列表超过 2048 的对象级测试缓冲改为 `EDGE_SLAVE_AXDR_MAX`（appconn/scheme_mgmt/driver）。

## 3. OI → 属性 → 来源 → DAR

| OI | 名称 | 广告属性 | 来源 | GET（已广告） | 类定义未广告 | SET 已广告 |
| --- | --- | --- | --- | --- | --- | --- |
| F200 | RS232 端口 | A2 | 无（ACL 另行保留 A3 读/A4 写） | 2 | 5 | 3 |
| F201 | RS485 端口 | A2 | 无 | 2 | 5 | 3 |
| F202 | 红外端口 | A2 | 无 | 2 | 5 | 3 |
| F203 | 开关量输入 | A2, A4 | 无 | 2 | 5 | 3 |
| F204 | 直流模拟量 | A2, A4 | 无 | 2 | 5 | 3 |
| F205 | 继电器输出 | A2 | 无 | 2 | 5 | 3 |
| F206 | 告警输出 | A2, A4 | 无 | 2 | 5 | 3 |
| F207 | 多功能端子 | A2 | 无 | 2 | 5 | 3 |
| F208 | 交采接口 | A2 | 无 | 2 | 5 | 3 |
| F209 | 载波/微功率无线 | A2, A5, A6 | 无 | 2 | 5 | 3 |
| F20A | 脉冲输入 | A2 | 无 | 2 | 5 | 3 |
| F20B | 蓝牙 | A2 | 无 | 2 | 5 | 3 |
| F20C | 230M 无线专网 | A2, A4, A5 | 无 | 2 | 5 | 3 |

DAR 分类（resolver 层）：已广告属性 2（暂时无来源）；类定义（类 22 属性上限 4，
F209/F20C 的 A5/A6 按 OI 广告）但实例未广告 5；类未定义 4；只读属性的 SET 3；
方法 1/2 → 5、方法 3+ → 4、A.12 的 127/128 → 5。**主站视角**：未广告属性/方法
先被 ACL 拒，回 DAR 3，5/4 只在 resolver 层可见。标量端口对所有已广告属性回
TYPE_MISMATCH（不截断、不造值）。

## 4. 行为变化（评审要求写明）

1. **F200 A2 从标量改为端口对象列表**：删除 `dcu_698_var.c` 的
   `{OI_CHANNEL_CONFIG, 2, VK_U32}` 与 `{OI_CHANNEL_CONFIG, 3, VK_U8}` 标量描述
   （此前被掩码挡住，不可达；A3 未广告，标量端口仍回 DAR 5）；
   `dcu_698_value_for_oad(F200 A2)` 由 CLASS_MISMATCH 变为 **TYPE_MISMATCH**；
   north GET 回 **DAR 2**（此前 F200 A2 未广告，GET 回 DAR 5）。
2. **F200 A2 SET** 由 DAR 5 变为 **DAR 3**（属性已广告但只读，无写处理器）。
3. **F200 A4 SET 仍是 DAR 5**（ACL 可写、对象掩码未广告，行为未变），验收
   `D_00_00_02_000014` 继续通过。

## 5. 测试

- 新增 `tests/test_dcu_698_class22.c`：每个 OI 的已广告属性 GET=DAR 2；未广告属性
  DAR 5/4；标量端口 TYPE_MISMATCH；已广告属性 SET=DENIED；方法 1=5、3=4；A.12 的
  OI 专属方法 F200 M127=5、F209 M128=5、F203 M127=4；F209 A6/F20C A5 计入 OI 定义。
- `tests/test_dcu_698_appconn.c`：ACL 只读权限锁定（A2 全部 rights 1；F203/F204/F206
  A4、F209 A5/A6、F20C A4/A5 为 1），对象列表 72。
- 验收扩充 `D_00_00_02_000035`：13 个 A2 + F203 A4、F209 A5/A6、F20C A4/A5 GET 回
  DAR 2；F203 A3/A9（未进 ACL）GET 回 DAR 3；F203 A4 SET 回 DAR 3；响应 OAD/service
  断言与 000034 相同口径（35 passed）。

## 6. 门禁

- clean Release build warning 0（`check_build_warnings.py`，仅 third-party open62541 不计）。
- `ctest -R dcu` 38/38；`ctest -R 'dcu|698|dlt698'` 45/45；首轮复核时全量 357/357。
- ASAN `detect_leaks=1`：45/45，零泄漏。
- guard selftest 118/118（含 `check_doc_references` live）；fanalyzer 3 已知 0 新增；
  SRP / plane isolation / component boundary 干净。
- 验收 2/2（最终源码 run 4，35 passed；`/interrogate` 修复后重跑；第一次
  `D_00_00_02_000031` 60s 恢复等待超时属台架时序抖动，见 runtime.log）。
- 全量 ctest 曾在对端 worktree 同时跑全量时失败 DDS 4 例（固定 `/dev/shm/edgeos_shadow_*`
  段互撞，`incompatible header`）；与 iems 侧串行后独占复跑 357/357，单跑各例亦绿，
  非本单回归。

## 7. 未决项

- 方法 127/128（端口参数设置/读取）未实现：A.12 已计入定义（resolver DAR 5），主站
  ACTION 因 ACL 无方法回 DAR 3；实现需要端口配置的持久化与来源，另立任务。
- F200 A3 的 ACL 读权限（`k_attrs_f200`）与对象掩码不一致：ACL 放行后仍由对象表回
  DAR 5；保持既有合同，不在本单收敛。
- F210 属类 8（参数），不在 Class 22 范围。
