# DCU-P3-5-VAR-4510 设计

## 判定

标量端口 `dcu_value_t` 没有 octet-string 的 `type_tag`/宽度/编码器，6 字节 MAC 不能
进入标量通道。4510 A5 的值源只属于 A-XDR resolver（`dcu_comm_info` 快照）；对
`dcu_698_value_for_oad` 而言该属性没有标量来源。按
`docs/design/DCU_698_DEVIATIONS.md` Class 25/26 行（“GET 无来源回 DAR 5”）与规则 4，
返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5），替换当前落入“已广告但无标量描述”
分支得到的 DAR 7。

## 改动点

- `cmd/dcu/dcu_698_var.c`：在 `dcu_698_value_for_oad` 的“已广告但无标量描述”分支里，
  仅对 `OI_ETHERNET_COMM_FIRST` 的 A5 返回 CLASS_MISMATCH；其它 OAD 保持原分级
  （数组/结构仍 TYPE_MISMATCH，类未定义仍 UNDEFINED，未知 OI 仍 NOT_FOUND）。
- 不改 `dcu_comm_info.c`、`dcu_698_axdr.c`、对象表、ACL、`dcu_value_t`；主站 GET 的
  `dcu_north_resolve_axdr` 分派与 octet-string(6) 编码不变。

## 被否方案

1. 扩展 `dcu_value_t` 承载 octet-string：改变“标量便捷结构”契约，调用方和编码器都
   要跟着改，且与本单“标量端口不可承载”的前提冲突。
2. 无条件让所有 Class 25/26 属性回 DAR 5：会把 4510 A6（类未定义）从 DAR 4 变成
   DAR 5，违反 DEVIATIONS 的“类未定义 DAR 4”。
3. 按 MAC 快照有无在 DAR 5/7 之间切换：让纯类型映射 API 依赖运行期快照，且
   DEVIATIONS 无此口径；标量端口无来源是常量，不是运行期状态。

## 回归面

- 主站 GET 4510 A5（有 MAC / 无 MAC）由既有 resolver 路径保持；新测试同时断言。
- 其它 `value_for_oad` 输入不受影响；定向 `ctest -R 'dcu|dlt698'` 与验收回归。
