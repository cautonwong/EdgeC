# JSONL 轨迹格式 v1（P0-04c 冻结）

> 状态：已冻结，P0-04c。实现与自检在 `tools/migration/parity/run_parity.py`；本文与实现不一致时以本文为准修实现，改格式必须升 `v`。
> 基线：`feat/iems` `f7420c8e`。样例：`tools/migration/parity/samples/sample_trace.jsonl`（14 条）。
> 证据来源：全部来自 `run_parity.py`、`edgec_target/edgec_modbus_target.c` 与样例文件本身【静态】。

## 1. 范围

本文冻结对拍轨迹的线格式、字段语义、输入与期望的关系、排序规则、未知字段策略、版本演进规则和完整性校验。不冻结具体 target 的寄存器映射或业务字段含义；那属于各 target 的适配契约（Modbus RTU target 见 harness 文件头注释）。

## 2. 文件格式

- UTF-8 文本，JSON Lines：每行一个 JSON 对象，行尾 `\n`；空行（仅空白）被忽略。
- 不使用数组包裹，不支持跨行记录，不支持注释。
- 解析时拒绝非有限数：`NaN`、`Infinity`、`-Infinity` 在任何层级都报错；`value` 内溢出为 inf 的字面量（如 `1e999`）同样报错，不是合法轨迹。
- 错误信息的行号是物理行号（空行也计数）。

## 3. 记录 schema

每条记录恰好六个字段。缺失任一字段即错误；未知字段被忽略并丢弃（见第 6 节）。

| 字段 | 类型 | 约束 | 语义 |
| --- | --- | --- | --- |
| `v` | integer | 必须存在且等于 `1`；不接受 `1.0`、`true`、字符串 | 格式版本，见第 7 节 |
| `t_ms` | integer | `>= 0`，且不小于上一条记录的 `t_ms` | 事件时间，毫秒，轨迹内单调不减 |
| `source` | string | 非空 | 记录来源的溯源标签；不参与比较 |
| `kind` | string | `point` / `alarm` / `command` 之一 | 事件类别，见第 4 节 |
| `key` | string | 非空，惯例为点分命名空间（`pcs1.active_power`） | 被观测对象或动作名 |
| `value` | JSON value | 任意合法 JSON，含非有限数则拒绝 | 该事件的载荷，形状随 `kind` 约定 |

加载器只校验上表约束，不校验 `value` 的内部形状；内部形状由 target 适配器按 `kind` 校验。样例轨迹给出了 v1 的三种标准形状。

## 4. kind 语义（v1 约定）

- `point`：遥测/状态更新。`value` 是数字（整数或浮点）。约定 `1` 与 `1.0` 是不同的值（见第 5 节严格比较），写出端应保持稳定表示。
- `alarm`：告警状态迁移。`value` 是对象，至少包含 `state`（字符串，`raise` 或 `clear`）和 `code`（字符串）。一次 raise/clear 各占一条记录。
- `command`：控制请求。`value` 是对象，包含一个数值目标字段（字段名由 target 契约决定，样例为 `target_kw` / `target_kvar`）和字符串 `req_id`。

以上形状由具体 target 强制（Modbus target 在 `parse_event` 中校验）。新增 kind 或改变既有 kind 的语义必须升 `v`。

## 5. 输入与期望的关系

同一条轨迹既是回放输入，也是比较基准（oracle）：

1. 加载器按行解析并严格校验，得到期望记录序列。
2. 回放器把完整六字段逐一交给 target；target 每个事件返回零条或多条记录。
3. 比较器把期望与实际按 `(kind, key)` 分组，组内按出现顺序逐条配对，只比较 `value` 和 `t_ms`。`source` 不比较；`v` 已在校验阶段消费。
4. `value` 用规范 JSON 比较：对象键排序；整数与浮点是不同的值（`120` 与 `120.0` 算分歧）；字符串按解析后的内容比较（`"a"` 与 `"a"` 不算分歧）；`1e2` 与 `100.0` 解析后是同一个浮点，不算分歧。
5. 同一 `(kind, key)` 组内条数不等计 `missing` / `extra`；配对条目的 `value` 或 `t_ms` 不等计 `mismatches`。
6. `first_divergence` 取各组首个分歧中 `(t_ms, kind, key)` 最小者。

退出码：`0` 无分歧；`1` 有分歧；`2` 轨迹/哈希/target 错误（未进入比较或无法回放）。

## 6. 排序与未知字段

- 全文件按 `t_ms` 单调不减排序；相同 `t_ms` 允许，记录之间的先后由文件顺序决定。
- 同一 `(kind, key)` 组内的配对依赖文件顺序。写出端必须保证顺序稳定、可复现；对同一 `t_ms` 的不同事件，建议以稳定次序输出。
- 未知顶层字段：v1 读取器忽略并丢弃，因此后续 writer 追加字段时旧读取器仍能工作。但不要依赖未知字段被保留、报告或传给 target。

## 7. 版本演进

- `v` 是整数，当前唯一合法值是 `1`。读取器遇到其它值直接报错并退出 `2`。
- 下列改动必须升 `v`：删除/重命名字段、改变字段类型或约束、改变 `kind` 集合或语义、改变比较语义（例如改为数值容差）、改变排序要求。
- 仅在“v1 读取器忽略该字段后行为仍完全正确”时，允许新增可选字段而保持 `v=1`。
- 新版本读取器可以同时接受多个 `v`；本工具目前只接受 `v=1`，这是冻结时的明确选择。

## 8. 完整性校验（SHA256）

轨迹是自带 oracle 的输入，改动其中 `value` 不会造成回放分歧。P0-04c 增加按原始字节的 SHA256 校验，在解析与回放之前执行：

```sh
python3 tools/migration/parity/run_parity.py replay \
  --trace tools/migration/parity/samples/sample_trace.jsonl \
  --target edgec-modbus \
  --trace-sha256 32de3467bb747e751954575e2d12dd56ae80540771335cad9eb09f688463fa29
```

- 哈希对象是文件原始字节（不是规范化 JSON），因此换行、空白、键顺序的任何变化都会改变哈希。
- `--trace-sha256 HEX` 接受大小写十六进制；校验在解析轨迹与启动 target 之前执行。
- 不匹配：退出 `2`，stderr 为
  `error: <trace>: trace SHA256 mismatch: expected <期望> actual <实际>`，同时给出两侧哈希。
- 不带 `--trace-sha256`：行为与 P0-04c 之前完全一致（向后兼容），不做完整性校验。
- 成功回放的报告（`--report`）会记录实际 `trace_sha256`，可用于生成登记值。
- 样例轨迹的登记值 `32de3467bb747e751954575e2d12dd56ae80540771335cad9eb09f688463fa29` 固定在 `run_parity.py` 的 `SAMPLE_TRACE_SHA256`，由 selftest 断言；轨迹被有意修改时必须同步更新该常量。

**局限（必须理解）**：这是无密钥哈希，只证明“文件字节与登记值一致”，用于防漂移、防误改、防传输损坏。能同时改写轨迹和登记值（或命令行参数）的人可以伪造一致状态；它不是签名或 MAC，不提供对抗性来源认证。需要防篡改来源时，登记值必须来自带外可信渠道（评审记录、签名清单、只读发布物），而不是与轨迹放在一起的可写文件。

## 9. 自检覆盖

`run_parity.py selftest` 的 22 个用例覆盖：

1. mock 零差异；
2. 注入 target 差异后报告首差并退出 `1`；
3. 真实 EdgeC Modbus RTU codec 零差异；
4. 篡改寄存器后首差 `t_ms=3000` 并退出 `1`；
5. 带正确登记哈希回放样例，退出 `0`；
6. 篡改首条 `value`（`0.0` → `999.0`）后带错误哈希回放，非零退出且错误含期望/实际哈希；
7-9. 缺字段、坏版本、乱序轨迹即使带正确哈希，仍按 schema/排序规则拒绝（退出 `2`）。
10. 合法 schema 且哈希正确、但 `value` 含溢出非有限数（`1e999`）→ 退出 `2`，stderr 为 `non-finite number at value` 且无 traceback。
11. IEMS MQTT golden trace（8 条）回放零差异，且每条帧与 v3 注释一致；
12. 注入载荷差异后轨迹比较器与注释检查双红，给字段与 `t_ms`，退出 `1`；
13. 篡改注释 `payload_hex` → 退出 `1`，只报注释差异；
14-16. 缺注释、未知注释字段、未知 key → 退出 `2`，不静默跳过；
17. 篡改 IEMS 轨迹后 pinned 哈希退出 `2`，错误含期望/实际哈希。
18. 有限 `value` 乘 scale 后溢出为 `inf`（`1e308`）或超出 S32 范围（`1e307`）→ 退出 `2`，错误含 `not representable as S32` 与 `value`，无 traceback；
19. sidecar 重复顶层 `t_ms` 键 → 退出 `2`，不静默 last-wins；
20. 只改注释 `topic` → 退出 `1`，报告 `field=topic` 与 `t_ms`；
21. 只改注释 `fields.value` → 退出 `1`，报告 `field=fields.value` 与 `t_ms`；
22. 注释字符串含 lone surrogate（`topic` 或 `fields` 的键/值）→ 退出 `2`，错误含 `not encodable as UTF-8`，不写 report。

## 10. IEMS MQTT 注释 sidecar（P4-02d，不改 v1 schema）

- 轨迹 `X.jsonl` 的 sidecar 是同目录的 `X.annotations.json`，只在 `--target iems-mqtt` 时读取。
- 顶层键是十进制 `t_ms` 字符串，必须与轨迹记录一一对应且唯一；缺一条、多一条或重复键都退出 `2`。
- 注释中的字符串（`topic`、`fields` 的键与值）必须可 UTF-8 编码；lone surrogate 等不可编码值 → 退出 `2`，且在写 report 前拒绝。
- 每条值恰好三个字段：
  - `topic`：按 P4-02c §1.1 渲染的 v3 主题；
  - `payload_hex`：v3 binaryFlow 帧的载荷段（`block_num(1) + 块列表`，P4-02b §1.2），小写偶数长度十六进制；帧头 TransId/DevLen/CmdLen 不属于本切片比较范围；
  - `fields`：从载荷段解出的业务字段。
- 未知注释字段、非小写十六进制、非对象值 → 退出 `2`。
- 注释与 target 输出不一致 → 退出 `1`，报告的 `annotations.mismatches` 给 `field`、`t_ms`、`expected`、`actual`；只改注释不改变轨迹比较器的结论，但整体 `ok=false`。
- sidecar 是 target 级 oracle，不进入第 3 节的 v1 记录 schema；轨迹的未知顶层字段仍按第 6 节忽略并丢弃。
