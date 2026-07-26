# 智能康复盒子与琥珀安逸家项目全功能落地实施计划书

> **产品代号**: SmartBox-Rehab (琥珀安逸家康复盒子)  
> **基线体系**: EdgeOS V2.1.0-AB / EdgeC 四平面隔离架构  
> **制定日期**: 2026-07-25  
> **责任人**: Mr. WKT (老板) / Antigravity AI Agent  
> **状态**: 待执行 (In Progress)  

---

## 一、 项目背景与对照综述

本计划基于《琥珀安逸家项目书(1).pdf》（53页）与《智能康复盒子产品核心功能拆解202504231954.xls》（70行全量功能拆解），结合 EdgeC 现有 `SmartBox-Alpha`（`cmd/smart_box` 与 `contracts/schema/domains/smartbox.yaml`）的底层架构，完成**全量差异化功能补充与产品化落地**。

### 1.1 核心异同与 Gap 摘要
- **相同点**: 两者均采用 EdgeOS 的 C 语言强实时物理底座 (`smart_box`) + Go 语言慢车道网关 (`smart-box-gateway`) 双轨通信模式，具备看门狗喂狗、SOS 物理按键 U-Boot 恢复、适老化 UI、AXP209 电源管理（断电切电池）等基础设施。
- **差异点 (Gap)**:
  1. **电源预警与心跳**: 补充 `BATTERY_SOC` 残量%监控、断电急救消息触发及 1h 定时心跳上报。
  2. **蓝牙多外设**: 从单路 BLE 扩展为最多 4 路 BLE 外设并发接入管理，并提供传感器数据统一 JSON 封装接口。
  3. **云端处方与 Mesh 重传**: 开发 WebSocket 双向处方长连接通道、处方防误操作校验引擎，并集成 DataHub Mesh (Merkle Tree CRDT) 实现断网 1 个月数据缓存与恢复后增量补包。
  4. **大屏游戏与正向激励**: 提供 HDMI 4K 大屏渲染接口、50Hz 实时 JSON 游戏数据桥 (`Game Bridge`)、训练完成度语音/动画激励（TTS）及双人同屏分屏竞技。
  5. **运维与安全**: 补充运行日志自动打包远程调取、游戏/素材包静默热更新 (`Hot Update`) 以及三级角色权限控制 (RBAC)。

---

## 二、 逐行功能 Gap 对照与 TODO 任务矩阵

| 模块类别 | 细分需求项 | 现有的基线状态 | 需补充的任务 (TODO) |
| :--- | :--- | :--- | :--- |
| **契约与电源** | AXP209 电源残量与低电量预警 | 已实现 `SYS_POWER_SOURCE` AC/Battery 判定 | **TODO-1.1**: 点表增加 `BATTERY_SOC` (ID 12013)，增加 SOC < 15% 自动报警。<br>**TODO-1.2**: 点表增加 `ABNORMAL_BEHAVIOR_ALERT` (ID 12014) 与 `REHAB_PRESCRIPTION_ID` (ID 12015)。 |
| **蓝牙与外设** | 多外设接入、绑定管理与数据 JSON 封装 | 已实现单路 BLE 血压/心率 GATT 读取 | **TODO-1.3**: 开发 BLE 多设备并发连接管理器 (最多4路)。<br>**TODO-1.4**: 开发外设数据统一 JSON 封装与插件式数据解构器。 |
| **处方与云同步** | WebSocket 处方下发、处方校验、依从性预警 | 已实现 SQLite3 存储与 MQTT/REST 慢车道 | **TODO-2.1**: 实现 Go 网关 WebSocket 处方双向长连接。<br>**TODO-2.2**: 实现处方比对引擎，核对非处方项目并警告误操作。<br>**TODO-2.3**: 开发依从性下降/频繁中断识别算子。 |
| **Mesh 增量补包** | 断网 1 个月数据本地缓存，网络恢复后增量补包 | 已在 `modules/edge_core` 实现 `edge_mesh` 模块 | **TODO-2.4**: 将 DataHub Mesh (Merkle Tree) 接入盒子落盘逻辑。 |
| **游戏与大屏** | HDMI 大屏渲染、JSON 实时数据桥、正向激励、双人竞技 | 已实现 Vue 3 Web 适老化 UI | **TODO-3.1**: 开发 50Hz 实时 JSON 游戏数据桥 (`Game Bridge`)。<br>**TODO-3.2**: 开发训练结束评分与语音 (TTS) / 动画正向反馈。<br>**TODO-3.3**: 开发双人同屏分屏数据竞技比对通道。 |
| **运维与热更新** | 定时心跳、日志远程调取、素材热更新、RBAC 权限 | 已实现 Watchdog、RSA 签名、SOS Unbrick 恢复 | **TODO-4.1**: 实现 1h 定时心跳包。<br>**TODO-4.2**: 开发 `.tar.gz` 素材包解压校验静默热更新引擎。<br>**TODO-4.3**: 开发日志自动压缩与远程下载 API。<br>**TODO-4.4**: 实现普通用户/医护家属/管理员 RBAC 权限隔离。 |
| **边缘与硬件** | 网络切换、BLE插件槽、远程镜像、RGB灯控、ALSA与OTA | 基础设施已就绪 | **TODO-5.1**: 网络优先级自动切换 (Ethernet > WiFi > 5G，支持 Static/DHCP IP)。<br>**TODO-5.2**: BLE 插件槽机制与自动单用户绑定。<br>**TODO-5.3**: 远程协助屏幕镜像与触控输入网关。<br>**TODO-5.4**: RGB LED 状态映射 (绿=正常，蓝=网络，红闪=故障)。<br>**TODO-5.5**: USB 声卡 ALSA 路由与 BLE DFU OTA 固件转发。 |

---

## 三、 细致可实施落地计划 (5 阶段实施路线图)

```
====================================================================================
Phase 1: 契约扩展、电池残量与 BLE 多设备管理器  (Contracts & BLE Multi-Device)
====================================================================================
Step 1.1: 扩展点表契约 smartbox.yaml
  - 文件: contracts/schema/domains/smartbox.yaml, contracts/products/smart_box.yaml
  - 动作: 增加 Tag 12013 (BATTERY_SOC), Tag 12014 (ABNORMAL_BEHAVIOR_ALERT), Tag 12015 (REHAB_PRESCRIPTION_ID)
  - 验证: ./build.sh

Step 1.2: AXP209 电源残量与 BLE 多设备管理器
  - 文件: cmd/smart_box/main.c
  - 动作: 读取 AXP209 0x33 寄存器计算电量 %；实现 ble_multi_device_poll 支持 4 路外设收发与 JSON 转化。
  - 验证: ./build/products/smart_box/bin/smart_box

====================================================================================
Phase 2: 云端 WebSocket 处方长连接与 DataHub Mesh 断网增量同步
====================================================================================
Step 2.1: Go 网关 WebSocket Client/Server 与处方校验引擎
  - 文件: go/cmd/smart-box-gateway/main.go, go/internal/prescription/manager.go
  - 动作: 实现云端处方 JSON 接收、本地比对校验、非处方误操作拦截与依从性预警算子。
  - 验证: cd go && go test ./...

Step 2.2: DataHub Mesh 增量补包落盘挂载
  - 文件: cmd/smart_box/main.c, tests/test_smart_box_offline_mesh.c
  - 动作: 接入 Merkle Tree CRDT 补包逻辑，完成断网 1 个月缓存与连网后 1ns 秒级同步测试。
  - 验证: ./build.sh && ctest -R test_smart_box_offline_mesh

====================================================================================
Phase 3: 大屏游戏 JSON 数据桥与适老化正向激励 HMI
====================================================================================
Step 3.1: 开发 50Hz 游戏 JSON 数据桥 (Game Bridge)
  - 文件: web/products/smart_box/src/components/GameBridge.vue
  - 动作: 通过 WebSocket/SHM 向大屏游戏推实时运动参数，建立 HDMI 4K 输出适配。
  - 验证: cd web/products/smart_box && npm run build

Step 3.2: 正向激励组件与双人同屏分屏
  - 文件: web/products/smart_box/src/components/PositiveFeedback.vue
  - 动作: 完成训练结算评分、TTS 语音播报、子女留言互动与双人竞技分屏显示。
  - 验证: 手动 preview 与 Vue 单元测试。

====================================================================================
Phase 4: 静默热更新、定时心跳包、日志远程调取与全链路测试
====================================================================================
Step 4.1: 开发素材热更新、1h 心跳与日志导出
  - 文件: go/cmd/smart-box-gateway/main.go, scripts/hot_update.sh
  - 动作: 支持 .tar.gz 素材静默解压替换，实现 1h 心跳包上报，提供 /api/v1/logs/download。
  - 验证: ./test.sh

Step 4.2: 全链路物理仿真验证
  - 文件: tests/test_smart_box_full_rehab_simulation.sh
  - 动作: 模拟处方下发 -> BLE 数据采集 -> 游戏输出 -> 断网补包 -> 心跳上报全流程。
  - 验证: ./test.sh --omnis

====================================================================================
Phase 5: 边缘特性与硬件集成 (Edge Features & Hardware Integration)
====================================================================================
Step 5.1: 网络优先级自动切换 (Ethernet > WiFi > 5G with Static/DHCP IP)
  - 文件: cmd/smart_box/net_manager.c, contracts/schema/domains/smartbox.yaml
  - 动作: 实现 Ethernet > WiFi > 5G 优先级网络检测与自动无缝切换机制，支持 Static/DHCP IP 配置解析。
  - 验证: ./test.sh --omnis

Step 5.2: BLE 插件槽机制与自动单用户绑定
  - 文件: cmd/smart_box/ble_plugin_slot.c, cmd/smart_box/ble_binding.c
  - 动作: 构建动态 BLE 插件槽解构引擎，并针对单一患者建立专属设备的自动抢占与锁定绑定。
  - 验证: ./build.sh && ctest -R test_ble_plugin

Step 5.3: 远程协助屏幕镜像与触控输入网关
  - 文件: go/cmd/smart-box-gateway/remote_mirror.go, go/cmd/smart-box-gateway/touch_gateway.go
  - 动作: 实现大屏画面 Websocket/WebRTC 实时流式镜像推送与远程医生 Web 端触控/鼠标指令反向注入。
  - 验证: cd go && go test ./...

Step 5.4: RGB LED 状态映射
  - 文件: cmd/smart_box/led_controller.c
  - 动作: 硬件 GPIO/PWM LED 驱动适配（Green=正常运行, Blue=网络连接, Red Blink=故障报警）。
  - 验证: ./build/products/smart_box/bin/smart_box

Step 5.5: USB 声卡 ALSA 路由与 BLE DFU OTA 固件转发
  - 文件: cmd/smart_box/audio_alsa.c, go/cmd/smart-box-gateway/ota_forwarder.go
  - 动作: 完成 ALSA 音频驱动软路由分流（语音播报 vs 运动音效）；实现云端 OTA 固件包经由盒子转发至 BLE 外设 DFU 升级。
  - 验证: ./test.sh
```
