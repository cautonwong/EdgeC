# DCU-D4-06 注释自审

范围：测试提交 `df20cf2c` 与修复提交 `5eef4736` 的新增注释（未跑 comment-sicko；
按仓库注释纪律人工自评）。

## 保留

- `cmd/dcu/dcu_698_object_model.c` 的 F000/F001/F002 块注释：产品无文件存储/DFU 来源，
  空掩码让 resolver 回 DAR 5，传输/校验/上传/下载方法没有处理器绝不执行；空掩码与
  遗漏无法用代码区分。
- `k_oi_max_attr` 注释补充表A.11 的 F000 A4/A5、F001/F002 A4：说明这些号不在 7.3 类表，
  但由标准为该 OI 定义，否则会被误判 DAR 4。
- `_class_defines_method` 的 F001 9/10 例外注释：说明号段在 127 以下，不能走
  `k_oi_max_method`（该表只服务 127+），避免后续误删。
- `cmd/dcu/dcu_698_appconn.c` 的 `k_attrs_file_frame`/`k_attrs_file` 注释：F000 A1..A5、
  F001/F002 A1..A4 与方法槽 8 的 HOLD。
- 测试中的表175/表A.11 说明与 driver 的 ACL 顺序注释：标准出处与 ACL 优先级是断言
  看不出的外部事实。

## 删除

0 条。未改既有 why 注释。

## 行为核对

3 个对象行 + per-OI 属性上限 + F001 方法 9/10 定义 + 只读 ACL；未改 resolver/驱动/
编码器。`ctest -R 'dcu|dlt698'` 45/45、六项守卫 0、mutation 1/1 敏感、验收连续 3 次
35/35。
