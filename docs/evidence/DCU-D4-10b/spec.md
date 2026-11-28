# DCU-D4-10b 五栏规格：Class 25/26 通信参数 (`4500..4517`) 软件配置领域引擎

## 1. 业务规则
1. 新建 `cmd/dcu/dcu_698_comm.h` 与 `cmd/dcu/dcu_698_comm.c`，实现纯软件 Class 25 无线公网（`0x4500..0x4501`）与 Class 26 以太网（`0x4510..0x4517`）通信参数配置领域引擎（独立模块，暂不修改 `dcu_698_object_model.c`）：
   - 无线公网配置 `dcu_comm_wireless_cfg_t`：APN（`visible-string`）、用户名、密码、主站 IP（4B）+ 端口（`uint16_t`）、备用主站 IP + 端口、心跳周期（`uint16_t`）。
   - 以太网配置 `dcu_comm_eth_cfg_t`：工作模式（DHCP/静态）、本机 IP（4B）、子网掩码（4B）、网关（4B）、主站 IP（4B）+ 端口（`uint16_t`）。
   - 提供配置读写与校验接口：`dcu_698_comm_init`（支持注入可选持久化端口 `save`/`load`）、`dcu_698_comm_set_wireless` / `get_wireless`、`dcu_698_comm_set_eth` / `get_eth`。
   - 提供 A-XDR 编解码接口：`dcu_698_comm_encode_attr(oi, attr_id, buf, cap, out_len)` 与 `dcu_698_comm_decode_set(oi, attr_id, axdr, len)`，非法类型/越界端口返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`（DAR 7）。

## 2. 不变量
1. **零动态内存**：全部使用固定静态存储（通过 `check_no_dynamic_memory.py`）。
2. **输入边界与原子性**：SET 解码失败或持久化 `save` 失败时回滚内存配置，不留半写入状态。
3. **缓冲区安全**：`cap` 不足时编码返回 `-1`，字符串定长截断保护。

## 3. 可观测输出
- `SET` 无线 APN/主站 IP/端口与以太网 IP/掩码/网关/主站端口后，回读与 A-XDR 编码一致；持久化失败时回滚至旧值。
- 错 tag / 截断载荷返回 `EDGE_SLAVE_SRC_TYPE_MISMATCH`（DAR 7）。

## 4. 验收判据
1. `tests/test_dcu_698_comm.c` 覆盖无线/以太网配置读写、A-XDR 编解码、持久化成功与失败回滚、负向边界，并接入 `tests/CMakeLists.txt`。
2. `cmd/dcu/CMakeLists.txt` 编译 `dcu_698_comm.c`。
3. Clean Release build 0 warning，`ctest -R 'dcu_698_comm'` 全绿，ASAN 下 0 错误，`check_no_dynamic_memory.py` 通过。

## 5. 非目标
- 不在本单修改 `cmd/dcu/dcu_698_object_model.c` 或 `dcu_comm_info.c`。
- 不修改真实 Linux 内核网卡或拨号守护进程。
