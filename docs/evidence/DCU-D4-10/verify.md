# DCU D4-10 验证记录（dcu-verifier，2026-11-26）

分支 `dcu/D4-10-comm-params`，merge `a926964b`（tip `4b242540`，首版 `31513dd0` + 修复 `27d686e8`），基线 `efd887a0`。

## 结论

VERIFIED（含一轮对抗修复）。Class 25（4500/4501）与 Class 26（4510..4517）登记为只读 DAR 诚实对象；唯一真实来源是平台第一个**物理单播**以太网口的 MAC，经 `dcu_comm_info_scan(net_root, out)` 从 sysfs 选取（type=1、无 wireless、无 bridge/、有 device 总线链接、I/G 位为 0、非全零），4510 A5 编码 octet-string(6)。其余类定义属性 DAR 5、类未定义 DAR 4、未注册 OI DAR 6、未进 ACL DAR 3；SET 回 DAR 3，方法 1 无处理器回 DAR 5。容器/WSL 的 veth eth0 不再冒充物理口。

## 对抗评审的三条缺陷（修复轮）

1. 组播 MAC 被接受（I/G 位未查）。
2. 以 en*/eth* 命名的 Linux bridge 被当成物理口（bridge/ 目录未查）。
3. 容器/WSL 的 eth0 veth 经常量快路径直接当物理口（无 device 总线链接）。

三条均以新建 sysfs 夹具树（`mkdtemp` + type/address/wireless/bridge//device 链接）复现并修复；评审用 faked /sys 编译探针给出实际输出（01:00:5e:…、enbr0、eth0@if7）。

## 红先绿后

- 首版测试提交 `dd8d9cc0`：链接红（`undefined reference to dcu_comm_info_read/install/reset`），对象计数 `106 != 116`、`dcu_698_find(0x4500)==NULL`。
- 修复轮测试提交 `230c9c20`：`undefined reference to dcu_comm_info_scan`（seam 先由测试定义）；dev 用行为等价抽取（旧判据 + 新 seam）得到 4/53 红，恰为组播×2、bridge、veth。
- 绿：tip `4b242540` `test_dcu_698_axdr` 53/53、`ctest -R 'dcu|dlt698'` 45/45（77.48s），原文 `verifier/green2-*`。

## 独立变异（验证判据真的承重）

在修复 tip 上逐条去掉判据、重建并跑夹具用例，每次恰目标用例红、恢复后 53/53：I/G 位→2 例红；wireless→1 例；bridge/→1 例；device→1 例。原文 `verifier/mut*.log`、`verifier/restore*.log`。

## 验收（COVERAGE=ON build-cov）

3 连绿，2/2 任务，3+35 passed：10.06/18.43、11.84/15.93、11.02/17.93s。原文 `verifier/accept-fix-{1,2,3}.log`。

## 合并树门禁

- build/warnings/守卫自检/全量 ctest/变异守卫 `DCU-F5-heartbeat-slot` 结果见 `verifier/merged-*.log`；全量 ctest 367/367。
- 首轮 fanalyzer 报 `dcu_comm_info.c:112` `-Wformat-truncation`（`best[64]` 装 dirent 名最多 255 字节）；热修 `98eb5115` 把 `best` 加宽到 256 并注明 NAME_MAX，重建后 fanalyzer 3 known/0 new，全量 ctest 再次 367/367（`verifier/hotfix-*`、`verifier/fanalyzer2.log`、`verifier/final-ctest.log`）。
- ASAN 目标集（axdr/object_model/appconn/driver_dlt698/runtime_health/advanced_modules/scheme_mgmt）全绿、无 sanitizer 报告，原文 `verifier/asan-*.log`。
- 本机实测：`/sys/class/net/eth0/device` 不存在、`eth0@if7` 为 veth，修复后 `mac_present=0`，4510 A5 回 DAR 5（不返回 `ea:92:…`）。

## HOLD

- 多口回退仍按接口名字典序；eth2/eth10 端口序号排序待板卡配置来源（`shortcut:` 注释已写明）。
- `dcu_698_value_for_oad` 标量路径对 4510 A5 回 DAR 7；主站 GET 走 `resolve_axdr`，不受影响，收口需改 `dcu_698_var.c`，不在本单。
- APN/IP/端口写与持久化未做（ACL 只读）。
