# DCU-D1-17 应用连接（Class 20）与会话 ACL

基线 `e2ce657d`（D1-14a 已合入），分支 `dcu/D1-17-appconn`。

## 标准依据

- Class 20 属性与方法：7.3.20 表181、表182、表183。
- 对象列表布局：表182 `array { structure { OI, structure { 属性权限[], 方法权限[] } } }`；属性权限是 `{unsigned 属性ID, enum 0/1/2/3}`，方法权限是 `{unsigned 方法ID, bool}`。
- 建立/断开连接：6.3.6 表59/表60、6.3.7 表61/表62；完整字节例子见附录 H.2。
- 预连接窗口：6.1.1.3.3（最低权限，由服务器定义）。

## 设计记录

`docs/adr/0015-appconn-session-acl.md`（Proposed）。

- 会话状态放驱动 ctx 的 `priv`（`ctx_size = sizeof(dlt698_session_t)`，`driver_manager` 的 1024 字节定长存储，按会话清零）。
- 对象列表经新增的协议中立端口 `object_list` 取原始 A-XDR；不复用 `resolve_axdr`，避免改变普通取值的 A-XDR/标量回落判定。
- ACL 定长：16 个对象、每对象 8 属性、8 方法；超出上限判列表不可用，不截断。
- 未连接时按同一份对象列表作为预连接窗口；类 20 自身属性 2 例外允许读取，否则主站无法得知窗口。
- session 为 NULL、端口缺失、列表解析失败 → 数据请求一律 DAR 3。

## 端口签名

```c
int (*object_list)(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len);
```

产品实现：`cmd/dcu/dcu_698_appconn.c` 的 `dcu_698_appconn_resolve`，只为 `0x44000200` 返回表182 字节。

## 授权矩阵

| 情况 | GET | SET | ACTION |
| --- | --- | --- | --- |
| 会话已 CONNECT、OAD 在列表且权限足够 | 正常数据 | 正常写入 | 正常执行 |
| OAD 不在列表 | DAR 3 | DAR 3 | DAR 3 |
| 属性权限不足（如只读属性写） | 正常读 | DAR 3 | — |
| 方法权限为 false 或方法不在列表 | — | — | DAR 3 |
| 未 CONNECT（预连接窗口） | 列表内允许，列表外 DAR 3 | 同上 | 同上 |
| session 为 NULL / 端口缺失 / 列表解析失败 | DAR 3 | DAR 3 | DAR 3 |
| 类 20 属性 2（对象列表本身） | 始终允许读取 | — | — |

CONNECT：版本不匹配回 ConnectResult 5；认证机制非 `[0]` 或参数/列表不可用回 255；成功回 0。RELEASE 成功回 0 并清空会话。

## 产品侧

- `cmd/dcu/dcu_698_appconn.c` 提供对象列表字节；`cmd/dcu/dcu_north_adapter.c` 把它接到 `object_list` 端口。
- 列表覆盖当前实际提供的能力（16 个对象）：0010/0020、2000/2001/2004/200A/200F、5004、6000/6002/6003/6012、F200 属性，以及 8000 的方法 1、126-131（方法本身是否实现由处理器决定 DAR）。
- DCU 未先 CONNECT 时走预连接窗口，北向验收不需要改流程。

## 审查后修正

- 产品对象列表补上 200A/200F（变量表提供但列表漏了，GET 会误回 DAR 3）。
- `dlt698_decode_frame_ex` 按服务区分头部：CONNECT/RELEASE 无 CHOICE 字节（表59/表61），PIID 在第二字节、体从第三字节起；驱动里的位置绕法删除。
- `dcu_north_adapter.c` 因接线增加 2 行，按先例在 `ci/baseline/composition_root_budget.txt` 登记 189 → 191。

## 验证

（见 `tests.log`：干净构建与告警门禁、ctest、ASAN、守卫、验收。）

## 已知边界

- 对象列表上限 16/8/8；更大列表的 CONNECT 会失败（255），不静默裁剪。
- 未实现断开超时后的 RELEASE.notification；本轮只处理 RELEASE 请求。
- `dcu_exit.json` 等既有项不在本单范围。
