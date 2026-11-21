# DCU-D1-15b 记录型读的驱动与端口

基线 `a85217aa`（D1-15a 已合入），分支 `dcu/D1-15b-record-driver`。

## 设计记录

新增 `docs/adr/0014-698-slave-record-port.md`（Proposed，1 页）。要点：

- 端口协议中立：`edge_slave_plane.h` 只出现 `uint32_t oad`、原始选择字节与调用方行缓冲，不出现 `dlt698_record_sel_t` 或 DAR。
- 分页按绝对行号重查，不保存游标：无动态状态、无游标失效、重复行不可能。
- 表74 只有整条结果的 DAR，没有行级 DAR；行失败丢弃整页，回 `CHOICE [0] DAR`，不发明行级错误字段。
- 回调为 NULL = 功能缺失 → DAR 2；选择方法超出子集 → DAR 2；产品返回码走既有 `_dar_for_source_rc`。

## 端口签名

```c
struct {
    int (*begin)(void *ctx, uint32_t oad, const uint8_t *selection, size_t selection_len,
                 size_t *row_count);
    int (*row)(void *ctx, uint32_t oad, size_t index, uint8_t *row, size_t cap, size_t *row_len);
    int (*end)(void *ctx, uint32_t oad);
} record;
```

`selection` 是请求里 RSD+RCSD 的原始字节，长度由 `dlt698_decode_get_record_request` 新增的 `consumed` 输出给出，不含末尾时间标签。

## 编解码补充

- `dlt698_decode_get_record_request` 增加 `size_t *consumed`；新增 Selector1（`SEQUENCE { OAD, Data }`，Data 限定 date_time / date_time_s，与区间值同一套长度检查）。Selector3..10 仍返回 `DLT698_RECORD_UNSUPPORTED_SELECTOR`。
- `dlt698_record_sel_t` 增加固定大小 `value[11]` / `value_len`。
- 一致性套件新增 H.3.3 请求（Selector1）向量，断言选择值、RCSD 与 `consumed`（不含时间标签）。

## 驱动行为

GET-Request choice [3]：解码 → 端口 begin/row×N/end → 流式编码。

| 情况 | 应答 |
| --- | --- |
| 正常一页（≤255 行） | `85 03 PIID OAD RCSD 01 M` + 行 + `00 00` |
| 选择方法不在子集 / 区间类型不支持 | 整条 DAR 2 |
| 端口回调为 NULL | 整条 DAR 2 |
| begin 返回产品码 | 整条 DAR（NOT_FOUND→6 等） |
| 行失败 | 丢弃半页（`out->b.len` 复位到流起点），整条 DAR |
| 总行数 >255 | 整条 DAR 2（没有 GetResponseNext，不能截断成假完整） |
| 报文截断/结构错 | 不应答（返回 -1），不伪装成 DAR |

## DCU 产品侧

`cmd/dcu/dcu_north_adapter.c` 的 `edge_slave_source_ops_t` 是指定初始化器，新字段不影响编译；未提供记录回调，因此在 D1-07 接上存储前，从站对记录请求回 DAR 2。本单没有改产品代码或存储。

## 验证

| 项 | 结果 |
| --- | --- |
| 干净构建 | 0 错误 0 告警；告警门禁 No new violations |
| 驱动用例 | 10/10（整页、非法选择、begin 失败、行失败丢页、端口缺失、畸形请求丢弃） |
| 一致性套件 | 30/30 |
| `ctest -R 'dcu\|dlt698'` | 29/29 |
| ASAN `ctest -R dlt698` + 驱动用例 | 6/6、10/10 |
| 守卫自检 / 验收 | 通过；验收 2 任务 6 用例 |
| `git diff --check` | 干净 |

## 已知边界

- 分帧续传（GetResponseNext，表76）未实现；>255 行的查询回 DAR 2。
- Selector3..10 未实现，回 DAR 2。
- 记录存储查询归 D1-07；本单没有真实数据源。

## 没做的

- 没在 GitHub Actions 上重跑；以上为本机同命令结果。
