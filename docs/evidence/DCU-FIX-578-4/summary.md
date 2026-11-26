# DCU-FIX-578-4：3000 A6 被广告为可写，合法 SET 返回 DAR 7

## 结论

对象列表把 3000/3005 的 A6 事件记录表广告为可读写（rights=3），而写路径只接受 A11/A12：合法 A6 值通过 ACL 后落到门面的 `TYPE_MISMATCH`（DAR 7）。修复后 ACL 把 A6 广告为只读（SET 在 ACL 层回 DAR 3），门面把"mask 已开、有写处理器、但没有写种类"的属性归类为 `DENIED` 而不是 `TYPE_MISMATCH`。A8/A9 的 DAR 5 仍可达：ACL 保留 rights=3，请求进入门面后 mask 关闭按 DAR 5 拒绝；A11/A12 仍可写。

## 根因

- `cmd/dcu/dcu_698_appconn.c`：`k_attrs_event` 给 A6、A8、A9 授 rights=3。
- `cmd/dcu/dcu_698_axdr.c`：`dcu_north_accept_axdr` 在 mask 已开、`obj->write` 存在时，`_write_kind_for` 只认 A11/A12；其余属性返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`，驱动映射 DAR 7。
- 设计文档 `docs/design/DCU_698_OBJECT_MODEL.md`：写路径只接受 A11、A12；A8/A9 保持 mask 关闭。

## 修复

- `k_attrs_event`：A6 从 `{6, 3}` 改为 `{6, 1}`（只读），与写路径一致；A8/A9 保持 `{8,3}`/`{9,3}`，让门面的 mask 关闭分支（DAR 5）保持可达；A11/A12 保持可写。
- `dcu_north_accept_axdr`：`_write_kind_for` 失败 → `EDGE_SLAVE_SRC_DENIED`（只读属性拒绝访问，不是值类型错）。绕过 ACL 的本地/API 调用同样不会再看到 DAR 7。
- 既有测试同步：`tests/test_dcu_698_axdr.c` 两处 `dcu_north_accept_axdr(3000 A6, ...)` 的期望从 `TYPE_MISMATCH` 改为 `DENIED`（它们锁定的是被修复的旧分类，不是被削弱的断言；合法 A6 值不是类型错）。
- 设计文档同步：写路径句后补"对象列表把 A6 广告为只读…A8/A9 仍广告可写，mask 关闭由门面回 DAR 5"。

## A8/A9 的 DAR 5 可达性（队列要求说明）

选择改 ACL：A6 → 只读，因此 A6 的 SET 在驱动 ACL 层得到 DAR 3。A8/A9 **不**改为只读：ACL 仍广告 rights=3，驱动放行后由 `dcu_north_accept_axdr` 的 `!dcu_mask_test(obj->attr_mask, attr_id)` 分支返回 `CLASS_MISMATCH`，线上 DAR 5。测试 `test_event_set_dar_by_attribute` 直接断言 A8/A9 的线上 DAR 5。

## 完成定义

- 静态分析：`check_build_warnings.py` 通过（tests.log 末段，0 violations）。
- 测试：`tests/test_dcu_698_appconn.c`
  - `test_event_acl_advertises_only_a11_a12_writable`：解析对象列表，断言 3000 A6=1、A8=3、A9=3、A11=3、A12=3。
  - `test_event_set_dar_by_attribute`：门面直调 A6 → `DENIED`；线上 SET A6 → DAR 3；A8/A9 → DAR 5；A11 → DAR 0；A12 → DAR 0；A11 越界枚举 4 → DAR 7（类型错仍归类型错）。
  - 修复前：ACL 断言失败（A6=3）；门面 A6 `-22 != -25`；`-22` 经驱动映射即线上 DAR 7（runtime.log 的 repro 可见）。
  - 修复后：3/3 通过；ASAN 构建 3/3 通过；`test_dcu_698_axdr`、`test_dcu_698_object_model`、`test_dcu_698_value_table`、`test_dlt698_appconn` 与 `test_dcu_698_appconn` 5/5 通过。
- 运行时：`repro_acl_set`（adv-3）
  - 前：`ACL rights OI=3000 A6 = 3`；`SET 3000 A6 well-formed array[0] rc=-22 (TYPE_MISMATCH)`；A8 `rc=-21`。
  - 后：`ACL rights OI=3000 A6 = 1`；`SET 3000 A6 ... rc=-25 (DENIED)`；A8 仍 `rc=-21`。
  - 说明：该 repro 直接调门面（`dcu_north_accept_axdr`），天然绕过驱动 ACL；线上 DAR 3 由 `test_event_set_dar_by_attribute` 的驱动级断言证明。
- 证据文件：`tests.log`、`runtime.log`、`diffstat.txt`、`comments.md`。

## 未决项

- 无已知主站可达路径会把合法 A6 值映射成 DAR 7。

## 提交

- `648e183d` test: 失败复现（先失败）
- `97e9ae4a` fix: ACL 只读 + 门面 DENIED 归类 + 设计文档同步

分支 `dcu/fix-578-4-event-acl-write`，worktree `/tmp/edgec-land/fix4-event-acl-write`。未合并，等 dcu-verifier 验证。
