# DCU-D4-10 Class 25/26 通信参数（只读 DAR + 物理 MAC）

分支 `dcu/D4-10-comm-params`，工作树 `/tmp/edgec-land/d4-10-comm`，基线 `feat/dcu`
`efd887a0`。本文件覆盖原始交付与 2026-11-26 的评审修复轮。

## 前提核查

表196/199 定义 Class 25 A1..A11 与 Class 26 A1..A5，两者都只有方法 1（复位）。产品
没有 APN/ICCID/IMSI/SIM 号码/信号强度/端口配置的存储或生产者，因此 Class 25 只做
DAR 诚实骨架；Class 26 唯一有来源的属性是 4510 A5 的本机 MAC。

## 落地的数据形状

| OI | 接口类 | 已有来源 | 其余属性 |
| --- | --- | --- | --- |
| 4500/4501 | IC_WIRELESS_PUBLIC (25) | 无 | resolver DAR 5（类未定义 DAR 4），ACL 先回 DAR 3 |
| 4510..4517 | IC_ETHERNET (26) | 4510 A5：平台物理单播以太网口 MAC | 其余 DAR 5；A6 起 DAR 4 |

- `dcu_comm_info_t` 是 init 时读取、reset 清空的快照；`dcu_comm_info_encode_axdr`
  只在 4510 A5 且 `mac_present` 时编码 octet-string(6)。
- ACL 属性 A1..A5 只读、方法 1；SET 回 DAR 3，方法 1 无处理器回 DAR 5。

## 修复轮（2026-11-26）

对抗评审编译真实模块并伪造 `/sys/class/net` 证明三处运行时缺陷；三条都在
`cmd/dcu/dcu_comm_info.c` 的接口选择逻辑里。

| # | 缺陷 | 复现（评审探针） | 修复判据 |
| --- | --- | --- | --- |
| 1 | 组播 MAC 被当成物理口 MAC | enp1s0 `01:00:5e:00:00:02` + ens3 → 选中组播 | 首字节 I/G 位为 1 拒绝；唯一候选时 `mac_present=0` |
| 2 | en/eth 开头的 Linux bridge 被当成物理口 | enbr0 带 `bridge/` + ens3 → 选中 bridge | 存在 `bridge/` 目录拒绝 |
| 3 | 容器/WSL 的 veth eth0 被当成物理口 | eth0 无 `device` 链接、`ip -d link` 为 veth → 返回 veth MAC | 必须有总线 `device` 链接 |

修复后的物理性判据：`type==1`、无 `wireless/`、无 `bridge/`、有 `device` 链接、
MAC 非全零且 I/G 位为 0。顺序保持原有行为：先 eth0，否则按接口名字典序取最小的
合格口；多口端口序号排序（评审 A 的 eth2/eth10 用例）记 HOLD，`shortcut:` 注释
保留并改写为“按接口名字典序，多口按板卡配置来源再映射”。

### 新增测试接缝

`dcu_comm_info_scan(const char *net_root, dcu_comm_info_t *out)` 在
`cmd/dcu/dcu_comm_info.h` 声明；`dcu_comm_info_read` 用固定根 `/sys/class/net`
调用它。测试在 `tests/test_dcu_698_axdr.c` 里用 `mkdtemp` 建夹具树（`type`、
`address`、`wireless/`、`bridge/`、`device` 符号链接），不改 `/sys`、不用 `--wrap`。

七个夹具用例：

- `test_comm_info_scan_prefers_eth0`（eth0 优先，回归守卫）
- `test_comm_info_scan_rejects_multicast_only`（缺陷 1，红到绿）
- `test_comm_info_scan_skips_multicast_for_physical`（缺陷 1，红到绿）
- `test_comm_info_scan_skips_en_named_bridge`（缺陷 2，红到绿；夹具合成 `device`
  链接让 `bridge/` 判据单独承重，真实 bridge 没有该链接）
- `test_comm_info_scan_rejects_wireless_and_non_ethernet`（回归守卫）
- `test_comm_info_scan_rejects_all_zero_mac`（回归守卫）
- `test_comm_info_scan_rejects_veth_named_eth0`（缺陷 3，红到绿；无 device 的 eth0
  回 `mac_present=0`，加入真实 ens3 后选 ens3）

提交顺序：测试 `230c9c20`（在 `31513dd0` 上链接失败），修复 `27d686e8`。红色
证据见 `red.log`（链接红，以及旧判据抽取后的 4 个行为红）。

## 验证

- 红（测试提交 `230c9c20`，基于 `31513dd0`）：`ninja -C build tests/test_dcu_698_axdr`
  链接失败，5 处 `undefined reference to dcu_comm_info_scan`。
- 行为红（同一提交 + 保持行为的抽取，判据仍是旧的）：4/53 失败，正好是三个缺陷的
  四个用例；其余 49 通过。见 `red.log`。
- 绿：同一二进制 53/53 通过，7 个新夹具用例全绿。
- 干净 Release 全量构建：`cmake --build build --clean-first -j4` rc=0（1271/1271），
  第一方 0 warning（干净日志只有第三方 open62541 unused-parameter）。
- `ctest --test-dir build -R 'dcu|dlt698' --output-on-failure`：45/45 通过（77.48 s）。
- `check_build_warnings.py`（干净日志）：无新违规；`check_build_freshness.py`：当前；
  `check_no_dynamic_memory.py`、`check_public_structs.py`、`check_stack_usage.py`、
  `check_doc_references.py`、`check_alloc_null.py`、`check_caller_allocated_storage.py`、
  `check_plane_isolation.py`、`check_component_boundary.py`、`check_resource_discipline.py`：
  通过。
- 本机 WSL2 的 eth0 是 veth（无 `device` 链接），修复后 4510 A5 回 DAR 5，不报
  veth MAC（`runtime.log` 有 `ls` 与 `ip -d link` 原始输出）。

## 跟进项（不在本分支）

1. 多口映射：4511..4517 没有逐口来源；端口序号排序（eth2/eth10）HOLD，等板卡
   配置来源落地。
2. 写与持久化（APN/IP/端口）未做；Class 25/26 的 SET 按 ACL 只读回 DAR 3。
3. APN/ICCID/IMSI/SIM 号码/信号强度没有生产者，只回 DAR 5；Class 26 A4 的 IP
   配置方式不可判定，仍回 DAR 5。
4. `docs/design/DCU_698_OBJECT_MODEL.md` 与计划 §8 D4-10 出口文字由 verifier
   按本单偏差更新。

未合并，等 dcu-verifier 验证。
