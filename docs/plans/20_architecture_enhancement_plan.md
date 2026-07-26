# EdgeC 20 项架构增强与落地实施计划 (20 Architecture Enhancement Plan)

> **版本**：v1.0  
> **日期**：2026-07-25  
> **制定者**：Planner Agent (SmartBox & EdgeC Core Team)  
> **适用产品**：EdgeC / SmartBox-Alpha (智能康复盒子)  

---

## 概述与总体目标

本实施计划旨在将从 GitHub 顶尖开源项目（如 Eclipse Iceoryx, EdgeX Foundry, KubeEdge, EMQX eKuiper, Zephyr RTOS, ThingsBoard Edge 等）吸收的 20 个高价值技术点子落地到 EdgeC 架构中。

计划严格遵循 EdgeOS 宪法中**四平面隔离（Foundation, Compute, Control, Execution）**与 **CDD 契约驱动** 原则，分为 5 个落地阶段，共 20 个具体可验证步骤。

---

## 阶段一：L0/L1 契约总线与零拷贝 IPC 增强 (Phase 1)

### 步骤 1.1: 共享内存首地址无关相对指针 (Relative Pointer SHM Engine)
- **目标**：在 `modules/edge_core/src/shmring.c` 中引入基于相对偏移量（`offset_ptr`）的无锁环形队列。
- **改动位置**：`modules/edge_core/src/shmring.c`, `modules/edge_core/include/shmring.h`
- **验证手段**：单元测试映射两个不同虚拟基地址，校验多进程无锁读写 50Hz 游戏姿态数据零拷贝传导。

### 步骤 1.2: Dynamic YAML Device Profile 运行期点表热加载
- **目标**：扩展契约编译器生成动态描述符，允许在不重启 C 主例程的情况下通过 Go 网关热更新点表。
- **改动位置**：`contracts/schema/`, `modules/edge_core/src/datahub.c`
- **验证手段**：向 Go 网关发送新 Tag 配置文件，验证 DataHub 动态注册新 ID 无需关停例程。

### 步骤 1.3: SQLite 级联 WAL 预写日志与断网离线队列
- **目标**：网络中断时自动将 BLE 体征数据流推入 WAL 缓存队列，恢复后按时间戳有序重放。
- **改动位置**：`modules/edge_core/src/sqlite_service.c`
- **验证手段**：模拟断网 5 分钟，校验高频数据落盘率及联网后数据恢复顺序。

### 步骤 1.4: DeviceTwin 离线影子状态机 (Desired vs Reported)
- **目标**：在 DataHub 标签元数据中区分 `Desired_Rx`（期望处方）与 `Reported_Rx`（报告状态）。
- **改动位置**：`contracts/schema/domains/smartbox.yaml`, `cmd/smart_box/main.c`
- **验证手段**：离线修改下发处方，恢复后验证 `Reported` 与 `Desired` 自动对齐机制。

---

## 阶段二：L2 边缘算力与流 DAG 处理引擎 (Phase 2)

### 步骤 2.1: 100Hz 拓扑滑动窗口算子 (Sliding Window DAG Node)
- **目标**：在 `modules/edge_stream` 增加拓扑滑动窗口算子，实现 20ms 卡尔曼滤波与姿态抖动抑制。
- **改动位置**：`modules/edge_stream/src/`
- **验证手段**：运行 `test_stream_dag`，验证高频姿态数据的噪声消除效果。

### 步骤 2.2: WAMR 沙箱资源看门狗与自愈隔离 (Resource Quota Guard)
- **目标**：为 WAMR `rehab_companion.wasm` 算子添加 10ms 调度超时看门狗，防止死循环阻塞主线程。
- **改动位置**：`modules/edge_stream/src/wasm_runtime/`
- **验证手段**：在 Wasm 内部构造死循环，验证底层看门狗 10ms 内捕获并重置 Wasm 沙箱。

### 步骤 2.3: 二进制 Protobuf / FlatBuffers IPC 适配器
- **目标**：高频流数据采用二进制 Protobuf 格式代替 cJSON，提升拆包效率。
- **改动位置**：`cmd/smart_box/main.c`, `go/cmd/smart-box-gateway/main.go`
- **验证手段**：对比 1000 帧/秒下 CPU 占用率下降比例。

### 步骤 2.4: Zephyr Driver Model 规范化解耦
- **目标**：将 `modules/hal` 原厂外设驱动包装为符合 Zephyr 规范的模块化 API。
- **改动位置**：`modules/hal/`
- **验证手段**：验证全志与万宝芯片 HAL 驱动一键无缝绑定。

---

## 阶段三：L3 管理平面与组网/分发增强 (Phase 3)

### 步骤 3.1: mDNS 本地 P2P 去中心化服务发现
- **目标**：在 Go 网关实现局域网 mDNS/P2P 自动广播，支持多 SmartBox 无网组网 PK。
- **改动位置**：`go/cmd/smart-box-gateway/main.go`
- **验证手段**：断开互联网，两台 SmartBox 在同局域网自动识别并建立对决会话。

### 步骤 3.2: BLE Plugin Slot 动态驱动加载桶
- **目标**：新 BLE 康复外设扫码后自动装载解析 Profile 与组件描述。
- **改动位置**：`go/internal/ble/`
- **验证手段**：扫码新设备，验证动态绑定插槽与体征解析输出。

### 步骤 3.3: 内存 Chunk 日志积攒压缩与 SPI Flash 保护
- **目标**：日志在 Memory Chunk 攒批达到 64KB 后批量 Gzip 压缩写盘，延长 Flash 寿命。
- **改动位置**：`go/internal/logger/`
- **验证手段**：并发写日志，验证磁盘 I/O 频率降低 80% 以上。

### 步骤 3.4: 远程医生屏幕镜像与 Canvas 触控注入
- **目标**：在 Web/WS 接口引入 Canvas 鼠标触控事件反向注入网关。
- **改动位置**：`go/internal/prescription/manager.go`, `web/products/smart_box/`
- **验证手段**：在远端 Web 打开镜像，点击页面验证设备端产生对应坐标响应。

---

## 阶段四：L4 执行平面与硬件安全防护 (Phase 4)

### 步骤 4.1: Zephyr DeviceTree 静态编译期 Linting 校验
- **目标**：编写 `scripts/dts_lint.py`，校验所有 UART/SPI/I2C DTS 引脚复用正确性。
- **改动位置**：`scripts/dts_lint.py`, `CMakeLists.txt`
- **验证手段**：故意制造重复引脚复用配置，验证编译期被准确拦截。

### 步骤 4.2: eBPF Socket Filter / SOS 紧急报文抢占打标
- **目标**：在 Linux 内核层将 SOS 报文打标为最高优先级（DSCP EF / TOS 0xB8）。
- **改动位置**：`modules/edge_security/`
- **验证手段**：网络拥塞时模拟按下 SOS，验证 SOS 帧 0 丢包且优先到达网关。

### 步骤 4.3: MCUBoot / SUIT 标准双 Slot 安全升级与自动回滚
- **目标**：双 Slot 升级试运行机制，3 次心率失常自动退回旧 Firmware 版本。
- **改动位置**：`cmd/smart_box/main.c`
- **验证手段**：刷入故意崩溃的新固件，校验试运行失败后自动恢复历史稳定版本。

### 步骤 4.4: 硬件 ISR 无锁单生产者单消费者 (SPSC) 环形队列
- **目标**：GPIO 按键中断与 AXP209 采集采用 SPSC 无锁队列传递给 libuv 循环。
- **改动位置**：`cmd/smart_box/main.c`
- **验证手段**：高频触发 GPIO 中断，校验零中断丢帧与 0 锁竞争。

---

## 阶段五：边缘 AI、安全芯片与自愈监控 (Phase 5)

### 步骤 5.1: MicroTVM / ONNX 模型极简 C 代码编译与 Wasm 部署
- **目标**：将跌倒检测与姿态预估 ONNX 模型编译为 C 算子，部署至 Wasm 算力平面。
- **改动位置**：`modules/edge_algo/`, `modules/edge_stream/src/wasm_runtime/`
- **验证手段**：输入姿态流数据，验证跌倒判断响应耗时 < 5ms。

### 步骤 5.2: MbedTLS 硬件 KeyStore 加密芯片绑定
- **目标**：设备身份证书与云端认证密钥绑定至加密芯片。
- **改动位置**：`modules/edge_security/`
- **验证手段**：尝试导出内存私钥，验证密钥不可被明文拷贝。

### 步骤 5.3: Chrome DevTools Protocol (CDP) 远程 HMI 性能诊断
- **目标**：Vue3 Web HMI 内置 WebSocket CDP 轻量代理，捕捉 4K 屏帧率与 Console 异常。
- **改动位置**：`web/products/smart_box/src/`
- **验证手段**：远端开启 Chrome DevTools 连入，查看组件 Render Time 与 Memory 视图。

### 步骤 5.4: 全局自愈健康得分矩阵 (Proactive Health Score Matrix)
- **目标**：Arbiter 定期扫描 CPU、内存、串口丢包与崩溃率，更新健康分并触发自动重启恢复。
- **改动位置**：`modules/edge_core/src/arbiter.c`
- **验证手段**：模拟死锁，验证 Arbiter 在健康分 < 50 时自动拉起崩溃例程。

---

## 验收标准

1. 每一个步骤必须具备确切的单元测试（C Ctest / Go Test / Web Build）。
2. 点表一致性校验 `check_tag_consistency.sh` 保持 100% 通过。
3. 遵循物理真实性原则，零 Dummy/Mock 占位符。
