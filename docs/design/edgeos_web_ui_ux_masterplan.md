# EdgeOS 工业级 Web 前端 UI/UX 架构设计与重构实施方案

## 1. 总体视觉与设计系统 (Design System)

基于 `ui-ux-pro-max` 的工业级设计模式深度检索，为 EdgeOS 确立以下核心视觉与 Token 体系：

### 1.1 视觉风格 (Visual Style)
- **核心定位**: Data-dense Dashboard (高密度数据看板) + Dark-mode OLED (纯净深邃暗色系) + 工业赛博高对比度。
- **排版密度**: 极致利用屏幕空间，组件间距采用 4/8/16px 紧凑节奏 (Spacing Scale 1-4)，确保一屏展示极大数据量且不拥挤。

### 1.2 全局设计令牌 (Design Tokens)

**Primitive Tokens (基础原子色):**
- Base Background: `#0B0F19` (深黑/午夜蓝底，完美适配 OLED 屏幕，极低功耗，减少光污染)
- Power Yellow: `#F59E0B` (电力黄，用于电网电能、有源告警)
- PV Green: `#10B981` (光伏绿，用于太阳能发电、清洁能源注入)
- ESS Blue: `#3B82F6` (储能蓝，用于电池充放电状态、SOC水位)
- Grid Purple: `#8B5CF6` (电网紫，用于市电交互、波形数据)
- Alert Red: `#EF4444` (告警红，用于保护触发、硬件异常、断开状态)

**Typography Tokens (字体排版体系):**
- **UI 框架与阅读文本**: `Inter` (无衬线、清晰中立，抗锯齿渲染佳，适用 12-16px 高密度字号)
- **高频数据与微秒时标**: `JetBrains Mono` (等宽字体，支持连字，确保数据跳动时绝对对齐，避免布局抖动。专门用于 MTT 毫秒级时间戳、电压/电流/频率等物理量展示)

---

## 2. 三大核心产品信息架构 (IA)

EdgeOS 核心控制台将拆分为三个相互独立而统一的 Web 产物：

### 2.1 工业微电网看板 (iems)
**定位:** 厂矿企业侧微电网能量管理系统的全局指挥中心。
- **微电网总览 (Dashboard)**: 光伏/储能/负荷/充电桩四象限功率流协同拓扑，Sankey 动态功率流向图。
- **经济调度决策 (Economic Dispatch)**: TOU (Time-of-Use) 分时电价套利收益曲线，峰谷平切换实时预测。
- **防逆流 PID 状态 (Anti-backflow Control)**: 实时防逆流死区监控，下发功率限制给定值，并以仪表盘展现 PID 调节状态。
- **告警中心 (Alarm Center)**: 聚合边缘网关上报的各级故障事件，支持毫秒级追溯。

### 2.2 智能台区网关看板 (smart_box)
**定位:** 配电网低压台区侧边缘计算终端的专业运维看板。
- **台区变压器监测 (Transformer Monitor)**: 变压器温升热力图、实时负载率极坐标图。
- **电能质量治理 (Power Quality)**: 三相不平衡度趋势监测与换相治理下发、SVG无功补偿动态响应柱状图。
- **拓扑与网络识别 (Topology AI)**: 台区户变关系、相位识别结果展示 (基于连线高亮交互)。
- **安全控制与五防 (Safety & 5-Prevention)**: 智能断路器高保真数字孪生控制面板，具备五防闭锁可视化呈现。

### 2.3 DCU 数据采集网关看板 (dcu)
**定位:** 集中器及通信网关的信道侦测与报文分析台。
- **抄表通信网 (Meter Network)**: DL/T 645 及 698 协议栈状态视图、实时报文抓包 (Hex Dump + 解析器)。
- **任务编排 (Task Orchestrator)**: 定时/周期/触发型抄读任务的树状调度图与执行甘特图。
- **信道质量诊断 (Channel Quality)**: PLC 载波/微功率无线的信噪比 (SNR) 散点图、丢包率拓扑。
- **时钟与对时 (Clock Alignment)**: 高精时钟对齐状态 (NTP/PTP/IRIG-B) 与 MTT 时延偏移雷达图。

---

## 3. 关键高保真组件设计规范

### 3.1 一次接线图 (Single-Line Diagram / SLD) 交互模式
- **架构实现**: 基于 SVG + D3.js 动态渲染母线、断路器、变压器和负载图元。
- **交互规范**:
  - 缩放平移 (Pan & Zoom) 支持无极缩放，保证 60fps。
  - 断路器分合闸状态采用高反差颜色映射 (闭合为红色/绿色配置，断开为空心)。
  - 悬浮 (Hover) 高亮回路，并弹出关联电气参数浮窗。

### 3.2 动态流动动效 (Dynamic Power Flow)
- **设计模式**: 采用纯 CSS/SVG `stroke-dashoffset` 动画实现能量流向。
- **性能与视觉**:
  - 动画速度线性映射物理功率大小 (1kW 缓慢，100kW 急速)。
  - 纯 GPU 硬件加速，避免引起主线程 `layout thrashing` 和重绘 (Repaint)。

### 3.3 工业级负荷曲线与实时散点 (ECharts)
- **UI/UX 规则**:
  - **大数据量优化**: ECharts 开启 `sampling: 'lttb'` 与 WebGL 渲染，支持单图 10w+ 点流畅平移。
  - **Tooltips**: 采用跟随鼠标且带十字准星 (Crosshair) 的高亮提示，数值统一采用 JetBrains Mono。
  - **图例联动**: 可拖拽区间，支持多系列数据的隔离与叠加对比。

### 3.4 双人授权复核安全写值弹窗 (Double-Authorization Modal)
- **UX 安全规范**:
  - 在下发断路器开合、定值修改等关键命令时，弹窗采用高危样式 (边缘 Alert Red 微光)。
  - **强制复核机制**: 需要操作员 (Operator) 与监护人 (Supervisor) 双方输入凭证 (或指纹/MFA硬件验证)。
  - **状态机引导**: 锁定前界面 -> 第一人签名 -> 第二人签名 -> 下发并展示 3 秒的写值反馈进度条。

---

## 4. 极致性能与流式渲染优化

针对 EdgeOS 嵌入式环境 (边缘网关可能仅有 512MB RAM)，提出严格的轻量化与高并发渲染机制：

### 4.1 WebSocket / REST Envelope MTT 毫秒级流式渲染流水线
- **数据流转**: 
  - 弃用传统的短轮询，全面切换至 WebSocket 或 SSE (Server-Sent Events) 长连接订阅快车道 (Fast Track) 数据。
  - 响应遵循标准 Envelope: `{ "code": 0, "message": "OK", "data": [...], "meta": { "mtt": 1718182928374 } }`。
- **前端吞吐**:
  - Vue 3 采用 `shallowRef` 取代深度 `ref`/`reactive` 存储高频数组源数据，极大幅度降低 Proxy 开销。
  - 结合 `requestAnimationFrame` 配合 `debounce`/`throttle` 节流函数批量提交 UI 更新。

### 4.2 轻量化嵌入式运行优化 
- **打包体积**: 
  - 严格限制 ECharts 与外部库按需引入 (Tree-shaking)，打包后 Gzip 大小必须 `< 500KB`。
  - Vue Router 使用 `() => import()` 路由级懒加载。
- **内存泄漏防护 (Memory < 20MB)**:
  - 组件卸载 `onBeforeUnmount` 时必须强制解绑 EventListeners、销毁 ECharts 实例、清理定时器。
  - 采用长列表虚拟化 (Virtual Scrolling) 渲染超大告警日志与报文，仅渲染视口 (Viewport) 内的节点。

---
> **结论:** 本架构结合了 EdgeOS 契约驱动开发 (CDD) 与 ui-ux-pro-max 最新的人机交互工业极简最佳实践，从底层引擎到前端体验实现物理数据的真实、敏捷、安全展示。
