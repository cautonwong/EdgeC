# AEP-490 架构红线与设计约束检查报告

| 检查项 | 约束规范 | 实测状态 | 判定 |
|---|---|---|---|
| OLED 深黑基底 | 背景色必须采用 `#020617` 零功耗深黑 | `tokens.ts`, `tokens.css`, `index.html` 统一采用 `#020617` | PASS |
| 双轨字体设计 | UI 框架 Inter / 物理量数值 JetBrains Mono (tnum) | CSS 与 HTML 全局声明并在 Tailwind 中注册 | PASS |
| 四态指示色 | 符合 IEC 60073 / 国网标准指示色 | Normal(#10b981), Warning(#f59e0b), Fault(#ef4444), Offline(#64748b) | PASS |
| 嵌入式体积约束 | 单页打包体积 Gzip 必须小于 500KB | IEMS JS: 72.46kB, SmartBox JS: 35.08kB, CSS < 8kB | PASS |
| 全量回归测试 | 163 项 CTest 全部通过 | 163/163 100% PASS | PASS |
| 物理链路仿真 | Omnis 10/10 全链路仿真无衰减 | 10/10 PASS (63.09s) | PASS |
