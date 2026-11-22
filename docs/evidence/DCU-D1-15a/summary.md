# DCU-D1-15a 记录型读的编解码

基线 `f78ac2fa`（D1-10 已合入），分支 `dcu/D1-15a-record-codec`。

只做编解码与测试；驱动接线、产品行为、端口头文件都没动，留给 D1-15b。

## 标准依据

- RSD：6.3.3.7 表30（第 32-33 页），区间前闭后开。
- GetRecord 请求：6.3.8.1.3 表67。`GetRecord ::= SEQUENCE { OAD, RSD, RCSD }`。
- GET-Response-Record：6.3.8.2.3 表74。`A-ResultRecord ::= SEQUENCE { OAD, RCSD, CHOICE { [0] DAR, [1] SEQUENCE OF A-RecordRow } }`。
- 完整报文：H.3.3（第 207 页）给出了日冻结单条记录的请求与响应整帧。

**与派工描述的差异**：派工写的是“OAD、记录条数、每条 Data 或 DAR”，表74 的实际结构还有 RCSD（列描述符）与 CHOICE 包装，并且每条记录是一行 N 列 Data。按标准实现，响应编码器接受 OAD + RCSD +（DAR 或逐行追加）。

## 接口

```c
int dlt698_decode_get_record_request(const uint8_t *apdu, size_t len, uint32_t *oad,
                                     dlt698_record_sel_t *sel, dlt698_rcsd_t *rcsd);

int dlt698_encode_get_record_response_begin(dlt698_record_stream_t *, const dlt698_addr_t *,
                                            uint8_t piid, uint32_t oad, const dlt698_csd_t *rcsd,
                                            size_t n, size_t record_count, net_buf_t *);
int dlt698_encode_get_record_row(net_buf_t *, const uint8_t *row, size_t row_len);
int dlt698_encode_get_record_response_end(const dlt698_record_stream_t *, net_buf_t *);
int dlt698_encode_get_record_response_dar(const dlt698_addr_t *, uint8_t piid, uint32_t oad,
                                          const dlt698_csd_t *rcsd, size_t n, uint8_t dar,
                                          net_buf_t *);
```

请求解码的 `apdu` 从 PIID 之后的 OAD 开始，末尾允许一个时间标签字节。响应编码逐行追加，调用方只需要一行的缓冲。

## 选择方法支持矩阵

| RSD | 处理 |
| --- | --- |
| `[0]` 不选择 | 支持（无区间字段） |
| `[1]` Selector1 | 返回 `DLT698_RECORD_UNSUPPORTED_SELECTOR`（-2），不静默忽略 |
| `[2]` Selector2 | 支持：OAD + 起始值 + 结束值 + 间隔 |
| `[3]`-`[10]` | 返回 -2 |

Selector2 的区间值只接受 date_time（标签 0x19）与 date_time_s（0x1C）；间隔只接受 NULL（0x00）或 TI（0x54）。其他类型返回 `DLT698_RECORD_UNSUPPORTED_DATA`（-3）。截断报文返回 -1。

RCSD 支持 CSD `[0] OAD` 与 `[1] ROAD`；上限：RCSD 8 列、ROAD 4 个关联 OAD。

## 分页

编解码不做分页。表74 的记录条数 M 是一字节，单帧上限 255 行；驱动按页调用 begin/row…/end。

## 验证

| 项 | 结果 |
| --- | --- |
| H.3.3（1）响应 | 编码器输出与标准 APDU 逐字节一致（29 条一致性用例全过） |
| Selector2 请求 | 按表30 构造的报文解码正确（OAD、区间、RCSD 两列） |
| `[0]` 不选择 | 解码正确 |
| H.3.3 `[1]` 请求 | 返回 -2，用例断言了该错误码 |
| 截断 / 非日期区间 | 分别返回 -1 / -3 |
| 干净构建 | 0 错误 0 告警，`check_build_warnings.py` 通过 |
| `ctest -R 'dcu\|dlt698'` | 29/29 |
| ASAN `ctest -R dlt698` | 6/6，conformance suite 通过 |
| 守卫自检 / 验收 | 通过；验收 2 任务 6 用例全合格 |
| `git diff --check` | 干净 |

旧用例 `test_get_record_and_rsd_not_implemented` 已删除：现在有 H.3.3 的标准例子，skip 不再成立。

## 没做的

- 驱动接线与北向分派（D1-15b）。
- Selector1 与 3-10 的解码，明确拒绝而不是猜布局。
- 没有在 GitHub Actions 上重跑；以上为本机同命令结果。
