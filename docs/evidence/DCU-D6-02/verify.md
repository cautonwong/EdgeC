# DCU-D6-02 独立验证 (`verify.md`)

规格：[spec.md](spec.md)。基线 `f443e688`，向量提交 `3a2201a1`，注释与字节修订 `c9ae7a0e`，合并 `97303424`。

## 独立推导

从标准原文取期望，不读实现输出：

- H.3.1（`docs/698_parsed.md:10204-10218`）：GET 4001 A2 的响应 APDU 为
  `85 01 01 40 01 02 00 01 09 06 12 34 56 78 90 12 00 00`，与 `test_get_normal_single` 的期望逐字节一致。
- 表54/表55（`:2319`、`:2344`）：REPORT-Response = `[8]` = 0x08；REPORT-Notification = `[136]` = 0x88。
- 5.1.3.2 表2（`:446-455`）：服务器发起的上报 DIR=1、PRM=0，控制域 0x83。
- 表34（`:1930-1937`）：DAR 0/2/4/5/6/7 的语义与测试断言一致。
- 表30（`:1809`、`:1817`）：Selector1/Selector2 的 RSD 结构；本夹具只实现这两个。

## 观测

| 期望 | 观测命令 | 结果 |
| --- | --- | --- |
| 13 个真帧用例 | `ctest -R '^test_dcu_698_wire_vectors$'`（合并树） | 13/13 PASS |
| ASAN 无越界/泄漏 | `ctest --test-dir build-asan -R 'test_dcu_698_wire_vectors|test_driver_dlt698'` | 2/2 PASS |
| 负向帧被丢弃 | 用例内断言 | PASS |

## 向量暴露的真实缺陷（D6-02a）

实现把服务字节写反（Notification 用 0x08、Response 用 0x88），上报帧控制域用了响应方向的 0xC3。已按表54/55 与表2 改为 0x88/0x08 与 0x83，并让驱动测试断言字面量 0x88；mutation guard `DCU-D6-02a-report-service-bytes` 红/绿通过（34/34 敏感）。

## 结论

**VERIFIED**：向量期望独立可复算，未修改产品语义；D6-02a 缺陷已根因修复并有可执行守卫。
