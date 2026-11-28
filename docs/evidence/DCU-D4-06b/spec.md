# DCU-D4-06b 五栏规格：Class 18 文件传输 (`F000..F002`) 软件领域引擎

## 1. 业务规则
1. 新建 `cmd/dcu/dcu_698_file.h` 与 `cmd/dcu/dcu_698_file.c`，实现纯软件 Class 18 文件传输（`0xF000` / `0xF001` / `0xF002`）领域引擎（独立模块，暂不修改 `dcu_698_object_model.c`）：
   - 维护静态有界传输会话上下文 `dcu_file_session_t`（固定分块缓冲与接收状态位图 `DCU_FILE_MAX_BLOCKS = 64`、每块上限 `DCU_FILE_BLOCK_SIZE = 256` B、总文件缓存 `16384` B）：
     - `dcu_698_file_start`（对应 F001 M7 启动传输）：初始化文件名、总字节数（`<= 16384`）、块大小（`1..256`）、预期 CRC32，清空状态位图；超限或非法参数返回错误。
     - `dcu_698_file_write_block`（对应 F001 M8 写文件分块）：按块序号校验边界并写入固定缓冲，置位接收位图；重复写同一块幂等覆盖。
     - `dcu_698_file_read_block`（对应 F001 M9 读文件分块）：按块序号回读已写入分块，未写入块返回未就绪错误。
     - `dcu_698_file_verify`（对应 F001 M10 校验）：检查全部块均已接收且 CRC32 与预期一致；缺失块或 CRC 不匹配返回明确失败码。
   - 提供 A-XDR 编码器：`dcu_698_file_encode_attr(oi, attr_id, buf, cap, out_len)`（覆盖 `F001` A2 文件信息/状态、A3 传输位图）。
   - 硬件 DFU 激活/回滚端口未注入时诚实返回 `EDGE_SLAVE_SRC_CLASS_MISMATCH`（DAR 5）。

## 2. 不变量
1. **零动态内存**：全部使用固定静态存储（通过 `check_no_dynamic_memory.py`）。
2. **分块与总长边界**：`block_idx >= total_blocks`、`offset + len > total_size`、`total_size > 16384` 严格拒绝，不越界写。
3. **校验严谨**：任一分块未接收或 CRC32 不一致，`verify` 必须返回失败。

## 3. 可观测输出
- 启动 512 字节（2 块 × 256 B）传输，写入块 0/1 后回读一致，CRC32 校验通过；缺块或篡改 1 字节后校验失败。
- 越界块号、超长文件、未激活状态读写均返回明确错误码。

## 4. 验收判据
1. `tests/test_dcu_698_file.c` 覆盖启动、分块读写、幂等覆盖、缺块/错 CRC 拒绝、A-XDR 编码及越界边界，并接入 `tests/CMakeLists.txt`。
2. `cmd/dcu/CMakeLists.txt` 编译 `dcu_698_file.c`。
3. Clean Release build 0 warning，`ctest -R 'dcu_698_file'` 全绿，ASAN 下 0 错误，`check_no_dynamic_memory.py` 通过。

## 5. 非目标
- 不在本单修改 `cmd/dcu/dcu_698_object_model.c`。
- 不实现真实 Flash DFU 分区刷写或看门狗重启激活。
