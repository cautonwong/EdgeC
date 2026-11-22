# DCU-D0-10 注释审查

工具：`pstack:no-comments`（comment-sicko 子代理，opus），范围为 D0-10 未提交改动。

## 接受的处理

删除 50 行陈旧注释/文档字符串：

- `bench.py`：`CHANNELS` 上方“main.c 把 0-21 硬编码成 DLT698”已过时——协议来自档案（`cmd/dcu/dcu_bus_adapter.c:15-23`），台架只在 22-31 通道声明 DLT645。
- `dcu_product.py`：模块 docstring 里的行号引用（argv/env、rs485 连接）已与当前 `main.c` / `dcu_bus_adapter.c` 不符；“壳在 DCU 退出后也跟着退”与 `:270-271` 的死循环矛盾；“地址不是 12 位 BCD 就照原样回”与 `:107` 的替换行为矛盾。三处文档字符串整段删除或裁剪。
- 新 `dump_json_atomic` 的 docstring 保留（原子可见性契约）。

审查人对保留的协议类注释逐条给了来源（DLT645 1CH、698 帧布局、SQLite `IF NOT EXISTS`、`os.replace` 原子性、平台就绪探针与 `product_exit=self_exit` 语义）。

## 执行的 MUST KILL

- `dcu_product.py` 的 BCD 地址回退：原来地址无法编码时改用 `METER_ADDRESS` 应答，把坏请求伪装成合理响应；现在抛出 `RuntimeError`，台架会响而不是编造。顺带移除因此不再使用的 `METER_ADDRESS` 导入。

## 驳回的 MUST KILL

- “`DCU_DEMO_MODE=1` 设在 env 白名单之前，外部环境可覆盖”：不成立。转发白名单只有 `MQTT_*`、`DCU_NORTH_SOCK`、`DCU_HEARTBEAT_CYCLES`、`DCU_CONTROL_DELAY_MS`，不包含 `DCU_DEMO_MODE`，外部值不会被转发。

## 派工方要求恢复（reviewer 复核）

verifier 指出注释删得过多，按要求在代码处恢复了 7 条简短约束（不写旧行号，每条 1-2 行）：运行目录可被 `DCU_RUN_DIR` 覆盖且并发共用会互相踩；二进制身份与过期判定必须先于结论；`DCU_DEMO_MODE` 固定为 1 且刻意不转发；env 是白名单，未转发变量落默认值会被误读成沉默；`_connect_north` 的短重试是因为刚被杀的产品可能拒绝一次连接；记录 CHOICE 是为了验证合批；`proc.wait()` 后壳不能跟着退出（平台记 `product_exit=self_exit`）。模块 docstring 补回了压缩版壳的理由（DCU 不读 argv、只从 env 取配置、启动时一次性连 rs485）。

## 记录未修（超出本单）

- `:249` 的 `time.sleep(0.5)` 是打开就绪端口前唯一的屏障；改成事件需要 `_unix_buses` 暴露“已开始服务”状态，属台架启动顺序的小重构。
- `dcu_exit.json` 目前没有读者，写入也不是原子的；要么删除，要么等有读者时再处理。
- `NORTH_BRIDGE_PORT = 19212` 与 `configs/devices_north.yaml` 重复，应改为单一来源；涉及配置文件，超出本单文件范围。

## 审查后的验证

- `py_compile` 三个文件：通过。
- `git diff --check`：干净。
- `./test.sh --omnis dcu` 三次：见 `tests.log`（每次 3 + 7，退出 0）。
