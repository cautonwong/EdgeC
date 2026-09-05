# AEP-490 (Issue #169) 交付证据包：EdgeOS Web 设计系统 Tokens 与深黑主题

## 1. 任务概述与交付范围
- **Issue**: [#169 - [AEP-490] EdgeOS Web 设计系统 Tokens 与深黑主题建设](https://github.com/cautonwong/EdgeC/issues/169)
- **分支**: `ticket/AEP-490`
- **核心目标**:
  1. 建立工业级 OLED 深黑零功耗基底（#020617 / #0f172a / #1e293b）；
  2. 建立四态运行指示（Normal #10b981 / Warning #f59e0b / Fault #ef4444 / Offline #64748b）；
  3. 建立微电网多源能流色系（Grid Sky #38bdf8 / PV Amber #fbbf24 / ESS Charge Emerald #34d399 / Discharge Orange #f97316 / Load Violet #a78bfa）；
  4. 建立双轨字体系统（UI 框架 Inter + 物理量数值/MTT时标 JetBrains Mono 等宽 `tnum` 特性）；
  5. 前端产品（IEMS、SmartBox）Tailwind CSS 与 Vite 打包全面适配，Gzip 体积严格受控（CSS ~7kB, JS ~70kB）。

## 2. 自动化测试与验证结果
- **CMocka C 单元测试**: `test_web_design_tokens` (4/4 PASS: TS 定义、CSS 变量、Tailwind 配置、HTML 双轨字体链接)；
- **全量 CTest 回归**: 163/163 100% 绿灯 PASS；
- **Omnis 物理全链路仿真**: 10/10 场景 100% 绿灯 PASS；
- **Web 生产构建**: `web/products/iems` 与 `web/products/smart_box` 生产打包成功。

## 3. 六大反熵门禁核查
1. **契约 0 漂移**: YAML 架构无修改，前后端契约严格对齐；
2. **核心只读保护**: `modules/edge_core` 未做侵入性改动；
3. **0 第三方符号污染**: 标准 Web Tokens 与 Tailwind 扩展，无私有依赖引入；
4. **体积预算合规**: 打包产物 CSS/JS Gzip 远低于 500KB 工业嵌入式预算；
5. **0 老旧引用**: 无废弃 API 调用；
6. **仓库卫生**: 目录结构干净，无冗余调试残留。
