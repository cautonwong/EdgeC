# 迁移对拍工具（P1-01 / X-01）

把 JSONL 轨迹回放给可替换 target，逐字段比较并报告首次分歧。CI 的 x86_64 作业在
`build/foundation` 生成静态库后构建 C harness，再跑 `selftest`；任何注入的差异都会让
selftest 退出 1，从而阻断构建。

## 文件

- `run_parity.py`：轨迹加载、回放、比较、报告与 selftest；`--target mock|edgec-modbus|iems-mqtt`，
  `--trace-sha256` 可选完整性校验。
- `samples/sample_trace.jsonl`：14 条样例轨迹，同时用作输入与 oracle。
- `samples/iems_mqtt_trace.jsonl` 与 `.annotations.json`：IEMS MQTT golden trace 与 v3 注释
  sidecar（主题、binaryFlow 载荷段、字段），8 条 point；缺注释即失败。
- `edgec_target/edgec_modbus_target.c`：C harness，链接仓库内真实 Modbus RTU codec。
- 轨迹线格式、语义与哈希校验规则冻结在 `docs/design/trace-format.md`。

## 本机复现 CI 门禁

在仓库根目录执行。`build/` 与 CI 的 Configure & Build 步骤使用相同生成器与参数。

```sh
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCOVERAGE=ON
cmake --build build --target edge_codec edge_netbuf -j"$(nproc)"

cc -std=gnu11 -O2 -Wall -Wextra \
  -o tools/migration/parity/edgec_target/edgec_modbus_target \
  tools/migration/parity/edgec_target/edgec_modbus_target.c \
  -Ifoundation/include -I.local/linux-x86_64/include \
  build/foundation/libedge_codec.a build/foundation/libedge_netbuf.a \
  .local/linux-x86_64/lib/libuv.a -lm

python3 tools/migration/parity/run_parity.py selftest
```

libuv 取自仓库内已跟踪的 `.local/linux-x86_64`，与 CI 使用同一份，不依赖仓库外的路径。

## 自检与回放

```sh
python3 tools/migration/parity/run_parity.py selftest
python3 tools/migration/parity/run_parity.py replay \
  --trace tools/migration/parity/samples/sample_trace.jsonl \
  --target mock --report tools/migration/parity/reports/replay_mock.json
python3 tools/migration/parity/run_parity.py replay \
  --trace tools/migration/parity/samples/sample_trace.jsonl \
  --target edgec-modbus \
  --trace-sha256 32de3467bb747e751954575e2d12dd56ae80540771335cad9eb09f688463fa29 \
  --report tools/migration/parity/reports/edgec_target.json
python3 tools/migration/parity/run_parity.py replay \
  --trace tools/migration/parity/samples/iems_mqtt_trace.jsonl \
  --target iems-mqtt \
  --trace-sha256 3e1256b3f21d8bb75087a167eb7faf82239710b800fcb20988609e4247a264e7 \
  --report tools/migration/parity/reports/iems_mqtt.json
```

`iems-mqtt` 把每条 point 映射为 `ipc/{sn}/{module}/{dev_id}/{type}/GetResp/binaryFlow`
（P4-02c §1.1）和单块 binaryFlow 载荷段（P4-02b §1.2），再从帧解码回 observed 记录；
sidecar 是 v3 侧 oracle，缺注释、多余注释、重复 `t_ms` 键、不可 UTF-8 编码的字符串、
未知注释字段、未知 key 均退出 2。台架用 Python
复刻映射，不链接 C codec；真实 v3 两侧零差异仍是残余（等 P4-02a 现场证据）。

轨迹是自带 oracle 的输入，单改 `value` 不会产生回放分歧；`--trace-sha256` 在解析与
回放前按原始字节校验，不匹配退出 2 并打印期望/实际哈希。不带该参数时行为不变。
哈希只证明文件与登记值一致（防漂移/误改），不防能同时改登记值的人；细节与局限见
`docs/design/trace-format.md` 第 8 节。

selftest 二十二例：mock 零差异；mock 注入差异后报告 `meter1.grid_power` 并退出 1；真实
codec 零差异；篡改寄存器后首差 `t_ms=3000` 并退出 1；正确哈希回放退出 0；篡改轨迹
后错误哈希非零退出且错误含两侧哈希；缺字段、坏版本、乱序轨迹带正确哈希仍被拒绝；
`value` 含溢出非有限数（`1e999`）时退出 2 且无 traceback。第 11-17 例是 IEMS MQTT
台架：golden 零差异且 8 条注释全匹配；注入载荷差异后轨迹与注释双红并给字段与 `t_ms`；
篡改注释退出 1 且只报 `payload_hex`；缺注释、未知注释字段、未知 key 退出 2；篡改 IEMS
轨迹后 pinned 哈希退出 2 并给两侧哈希。第 18-22 例钉住注释边界：`1e308`/`1e307` 这类
有限值乘 scale 溢出或被 S32 范围拒绝时退出 2 且无 traceback；sidecar 重复 `t_ms` 键退出 2；
只改 `topic` 或只改 `fields.value` 各退出 1 并报告字段与 `t_ms`；`topic` 或 `fields`
含 lone surrogate 时退出 2 且不留下 report。
第 2、4 例是“注入 target 差异会拦住构建”的证明，第 5-10 例是轨迹完整性门禁，第 11-22 例
是 IEMS MQTT 主题/帧/注释门禁。
`reports/` 与 harness 二进制是生成物。
