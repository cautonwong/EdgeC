# DCU-D6-02b 五栏规格：698.45 负向与边界真帧向量扩展

## 1. 业务规则
在 `tests/test_dcu_698_wire_vectors.c` 中补充 DL/T 698.45 负向与边界真帧向量测试（每条向量注明 `docs/698_parsed.md` 原文行号或条款号）：
1. **链路层负向帧**：破坏 HCS 校验字节（5.1.5）与破坏 FCS 校验字节（5.1.6）的请求帧，验证 `dlt698_decode_frame_ex` 拒绝（返回 `-1`）且从站驱动丢弃不回帧。
2. **截断 APDU 负向帧**：截断的 `GET-Request-Normal`（缺少 OAD 字节）、截断的 `SET-Request-Normal`（缺少 Data 字节）、截断的 `ACTION-Request-Normal`，验证从站驱动拒绝/返回 DAR 7（`DLT698_DAR_TYPE_UNMATCHED`）或丢弃，不崩溃不越界。
3. **GetRecord 边界与不支持选择方法**：
   - `n > DLT698_RCSD_MAX` 时 `dlt698_encode_get_record_response_begin` / `_dar` 返回 `-1`（`FORMAL-05` 回归向量）。
   - 不支持的 RSD Selector（如 Selector 3/4/5）在从站返回 DAR（`DLT698_DAR_UNSUPPORTED` / 协议拒绝）且不越界。
4. **CONNECT / RELEASE 响应帧布局**：验证 `dlt698_encode_connect_response`（`0x82`）与 `dlt698_encode_release_response`（`0x83`）的 APDU 服务字节、控制域 `0xC3`、HCS/FCS 校验合法。

## 2. 不变量
1. 仅修改 `tests/test_dcu_698_wire_vectors.c`，不修改产品代码与 `foundation/`。
2. 所有期望值来自 DL/T 698.45 标准原文（标注 `docs/698_parsed.md` 行号或条款号），不从当前实现输出反推。

## 3. 可观测输出
- `test_dcu_698_wire_vectors` 全部新增与既有向量用例通过。

## 4. 验收判据
1. `ctest --output-on-failure -R '^test_dcu_698_wire_vectors$'` 全绿。
2. ASAN 下 `test_dcu_698_wire_vectors` 无越界或泄漏。
3. Clean build 0 warning。

## 5. 非目标
- 不修改 `cmd/dcu/`、`adapters/`、`foundation/`。
