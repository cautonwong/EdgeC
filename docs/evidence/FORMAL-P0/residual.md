# FORMAL-P0 残余与 assumptions

1. **有界证明**。CBMC 覆盖 `in->len <= 260`，断言的是这个范围内所有符号化字节输入。超过 260 的帧、调用方把 `net_buf_simple.len` 设得大于实际可读容量、以及并发/时序不在范围内。
2. **`hi_crc`**。主入口固定 `hi_crc = false`（标准 Modbus 低字节先发）。`hi_crc = true` 的 `_get_crc` 分支未符号化覆盖；如需覆盖，复制入口并把 `hi_crc` 设为 `true`，随 manifest 一起扩范围。
3. **负例标签**。删除 `in->len < resp_len` 守卫后，`modbus_rtu_decode_frame_ex` 的全局 `in->len < frame_len` 检查仍会让该帧返回 1，因此 `rc == 0` 的成功路径断言不会失败。真正发生的是 `_get_crc` / `modbus_calculate_crc` 在读 `in->len` 之外，CBMC 报 `_get_crc.pointer_dereference.*: pointer outside object bounds: FAILURE`。负例脚本锚定这个标签；升级 CBMC 时标签格式变化会让负例先红。
4. **VLA 建模**。输入缓冲用长度恰为 `in->len` 的 VLA，使“越过 `in->len`”成为真实对象越界。生产代码只读 `in->data`，不依赖 `size`/`__buf`，该建模不改变被证明的代码路径。
5. **`fprintf` / `net_buf_simple`**。CBMC 5.12 能解析 `fprintf`/`fflush` 与包含 `uv.h` 的 `net_buf.h`；没有使用 test double，也没有退到纯函数。`uv.h` 需要 libuv 头文件（本机 `.local/linux-x86_64/include`，CI 安装 `libuv1-dev`）。
6. **property count**。1012 包含 CBMC 自动生成的指针/边界属性；新增断言或改 CBMC 版本都会改变它，必须同步 manifest，否则门禁失败（防证明缩水）。
7. **sync**。`2bbf729e` 是生产解码器最后一次确认的 commit。生产文件以后改动时 `check_formal_sync.py` 先告警，两个迭代周期后转硬失败。
8. **覆盖率**。CBMC 证明所有有界输入路径；没有做代码覆盖率统计（`__CPROVER_cover` 只证明成功路径可达）。
