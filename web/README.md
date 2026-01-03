# EdgeOS Web Infrastructure (Contract-Driven UI)

EdgeOS Web 是一套基于 **契约驱动 (Contract-Driven)** 理念构建的工业监控前端。它实现了 UI 展现与后端业务逻辑的高度解耦，是整个 SDK 的“视觉中枢”。

## 1. 核心架构 (Architecture)

### 🧩 框架组织
*   **Web Core (`web/core`)**：公共组件库、通用布局 (`AdminLayout`)、路由规则。核心逻辑由 TypeScript 编写，样式基于 Tailwind CSS。
*   **Web Products (`web/products/`)**：产品级工程（如 `iems`）。它几乎不包含业务页面，仅负责产品个性的声明（Branding）和特定契约的加载。

### 📡 动态渲染链路 (The Telemetry Pipeline)
1.  **加载阶段**：前端通过 `/api/v1/contracts` 接口获取后端生成的 `contracts.yaml`。
2.  **契约映射**：`ContractLoader.ts` 自动解析其中的 `active_tags`，根据点位的 `ui.group` 属性动态构建侧边栏菜单和仪表盘布局。
3.  **数据刷新**：每个测点卡片 (`LiveTagCard`) 独立向后端 Shadow 表发起请求，利用 C 语言无锁内存的高性能实现毫秒级数据同步。

---

## 2. 视觉规范 (Visual Identity)

为了匹配工业控制场景的严谨与前卫，UI 采用了以下设计语言：
*   **沉浸式工业蓝**：侧边栏使用 `Slate-950` 深色系，顶部 Hero 区域采用星空径向渐变。
*   **玻璃拟态 (Glassmorphism)**：数据卡片采用半透明模糊背景与细腻阴影，提升视觉层次感。
*   **数据动效**：数值发生变动时，字体颜色会产生 400ms 的闪烁反馈（Flash Effect），帮助操作员实时感知变化。
*   **响应式侧边栏**：支持点击式完全收缩，为主内容区提供 100% 的展示空间。

---

## 3. 开发与部署

### 路径探测机制
Web UI 具备 **环境感知能力**。后端程序（如 IEMS 二进制）启动时，会自动在以下路径搜索 Web 资源：
1. `../www` (生产部署路径)
2. `./build/products/iems/www` (项目根目录开发路径)
3. `./www` (本地开发路径)

### 构建命令
CMake 会自动处理 Web 构建，但您也可以手动执行：
```bash
cd web/products/iems
npm install
npm run build
```

---

## 4. 契约属性对 Web 的影响

在 `tags.yaml` 或 `extension.yaml` 中配置的 UI 属性将直接决定前端展现：
*   `ui.group`：决定测点出现在哪个分类下。
*   `ui.precision`：决定显示几位小数。
*   `ui.branding.theme`：决定侧边栏是经典蓝还是储能绿。

---
**EdgeOS Web - 重新定义工业软件的审美。**
