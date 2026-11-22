# DCU-D1-17 注释审查

工具：`pstack:no-comments`（comment-sicko 子代理，opus），范围为本单提交与未提交改动。ADR-0015 作为约束来源，不改。

## 执行的 MUST KILL

1. **产品对象列表漏了 200A/200F**：`k_objects` 没列这两个变量类 OAD，而 `dcu_698_var.c` 的 `k_vars` 提供它们，GET 会回 DAR 3。已补上；列表现在 16 个对象，与 ACL 上限一致。测试夹具本来就含这两项，审查指出的"夹具掩盖产品缺口"随之消失。
2. **`decode_frame_ex` 把 CHOICE 位置写死**：CONNECT/RELEASE 是 `服务 | PIID | 体`（表59/表61），没有 CHOICE 字节。已在解码器里按服务区分：`0x02/0x03` 不读 choice，PIID 在第二字节、体从第三字节起；驱动里 `choice` 当 PIID 与 `payload - 1` 的绕法删除。
3. **组合根预算**：`dcu_north_adapter.c` 因接线 object_list 增加 2 行，按既有先例在 `ci/baseline/composition_root_budget.txt` 登记 189 → 191 并写明原因。

## 接受的删除

删除 13 处叙述性注释与 2 处条款裁剪：Class 20 横幅；`_acl_parse` 的实现说明尾巴（保留表182 形状句）；`_acl_load`、`_acl_check` 的复述；"其余机制不假装成功"的说教；"对象列表不可用时不授权"；产品列表的"与实际能力一致"（原来还是错的）；两个测试的旁注与测试 ACL 的理由段落；`0x0011` 与预连接窗口的测试旁注。

## 修正的协议说明

CONNECT-Response 的 APDU 注释原写尾部两个 00，按 H.2 实际是四个字段（结果、认证附加信息、FollowReport、时间标标），已改成四个 00。

## 保留

表182 形状与权限枚举、预连接窗口依据（6.1.1.3.1/6.1.1.3.3）、CONNECT-Request 字段布局（表59/H.2）、ConnectResult 5/255 的表43 依据、object_list 端口契约、公开 API 文档、协议字段标签（0x82/0x83、厂商扩展、无认证附加信息）、ADR-0015 链接、测试里的标准向量出处。

## HOLD

无。审查未发现被标准推翻的保留项。

## 审查后的验证

- 全量重建：0 错误 0 告警；告警门禁无新增（见 tests.log）。
- `ctest -R 'dcu|dlt698'`：32/32。
- 守卫自检：通过（含组合根预算）。
- ASAN、验收见 tests.log/runtime.log。

## 未处理（记录）

- 断开超时后的 RELEASE.notification 未实现（本单只处理 RELEASE 请求）。
- 对象列表上限 16/8/8，超过即 CONNECT 失败（255），不裁剪。
