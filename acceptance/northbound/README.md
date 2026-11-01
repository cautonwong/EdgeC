# 北向验收（通路口径）

## 这个目录和 `acceptance/dcu`、`acceptance/iems` 的区别

| | 设备验收 | 北向验收（本目录） |
|---|---|---|
| 问的问题 | 这台设备**合不合格** | 这条链路**通不通** |
| 口径 | 判据集（设备手册的阈值） | 通路三要素：有上线帧、有可解码数据帧、metric 名是点位名 |
| 归因 | 设备不合格 / 测试无效 / 平台错误 / 证据损坏 | 通过 / 不通（缺件时判"无有效结论"） |
| 判据来源 | 清单 + 判据集 | 平台侧的载荷解码（`testplatform.protocol.mqtt_json`） |

**为什么不塞进设备口径**：硬塞会让报告出现"VOLTAGE 不合格"这种指错方向的结论——
链路不通时，设备的读数根本没到云端，谈不上合格与否。

## 跑法

```bash
# 前置：产品二进制已构建；平台 venv 装好 paho-mqtt；容器内有 mosquitto
cd /workspaces/test-platform
./.venv/bin/python /workspaces/edgeos/EdgeC/acceptance/northbound/smoke_northbound.py
```

环境变量：

| 变量 | 缺省 | 说明 |
|---|---|---|
| `EDGEC_BUILD_DIR` | `<repo>/build` | 构建根。开发时构建产物常在 `/tmp/edgec-build`，**必须显式指定** |
| `IEMS_BIN` | `$EDGEC_BUILD_DIR/products/iems/bin/iems` | 直接指到某个二进制（验证别人分支用） |
| `IEMS_RUN_DIR` | `/tmp/edgec-nb-run` | 运行目录（sqlite / socket / 日志） |
| `NB_PROTOCOL` | `MqttJson` | 北向协议。`SparkplugB` 也能跑，但平台目前解不了 protobuf |
| `NB_SHELL_PORT` | `23457` | 产品壳监听端口（与验收套件错开） |

### ⚠️ 验错二进制是本目录最容易踩的坑

踩过一次：台架把路径写死成 `<repo>/build/products/iems/bin/iems`，而当时的构建产物在
`/tmp/edgec-build`。于是北向用例对着**三周前**的二进制跑——现象是 `MQTT_BROKER` 生效、
`MQTT_PROTOCOL` 与 `MQTT_NODE_ID` 被无视（旧二进制里根本没有这两个字符串），看起来
像"环境变量没传进去"，实际是文件根本不是那一份。

现在 `acceptance/build_probe.py` 统一负责定位与过期判定：

- 产品壳启动第一句打印 `产品二进制：<路径>（编译于 <时间>）`
- 二进制比 `cmd/<p>/` 或 `modules/` 下的 `.c/.h` 旧时打警告，并指出是哪个源文件更新

**看到这条警告就不要相信后面的结论。**

退出码：`0` 通过 / `1` 不通 / `2` 缺件（无有效结论，不是产品问题）。

## 已验证的结果（2026-10-31）

```
topic: edge/IEMS_NB_SMOKE/NBIRTH
payload: {"online":true,"ts":1793457771000}
共 3 条，上线帧 1 条
点位 {'VOLTAGE_PHASE_A': 231.7, 'CURRENT_PHASE_A': 0.0}
```

`VOLTAGE_PHASE_A = 231.7` 是台架 Modbus 从站给的真实读数——**采集→推送→名字正确**这条链已经通了。

## 还没做的

- **断网补发（E2）**：需要 DCU 也接上 SnF——DCU 的 `northbound_mgr_init_ex` 目前传 `db=NULL`，
  **没有断网缓存**；IEMS 有（`app->db_service`）。
- **Sparkplug B 解码**：平台侧只有 JSON 解码器，protobuf 需要 `.proto` schema。
- **上线帧断言**：正式用例里没验 BIRTH。观察器在 pytest 启动时才订阅，而产品已被
  `run_task.py` 拉起，BIRTH 每会话只发一次、早就过去了。要验它得用"重启产品"的场景（E3），
  当前一个任务只拉一次产品，表达不了。

## 正式用例在哪

本目录只放**通路口径的冒烟**（手工跑、不落证据）。正式验收用例在
`acceptance/iems/tests/test_iems_northbound.py`（`tag:验收`），由 `acceptance/run.sh`
或 `run_task.py` 驱动，落 `summary.json` 给 `gate.py` 判。

跑法：

```bash
EDGEC_BUILD_DIR=/tmp/edgec-build NB_E2E=1 ./acceptance/run.sh iems
```
