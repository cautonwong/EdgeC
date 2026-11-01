# 迁移期历史用例（当前不运行）

这三个 spec 针对重构前的 AdminLayout 引导流程与 `/api/v1/contracts` 取数方式，
在 WebUI 平台重构（Core Shell + 静态 Registry）后**不再成立**：

- 它们假设进入首页即自动加载契约、菜单由契约动态生成；
- 现在进入任何页面都先经过登录、能力快照与 Registry 过滤；
- 断言里的选择器（旧侧栏结构）已被 `AppShell` 取代。

处理方式：**不修复、不删除、不假装通过**。`playwright.config.ts` 用
`testIgnore: ['legacy/**']` 把它们排除在运行之外，随 WBUI-072（Core 通用页面与
L0–L3 组件迁移）逐条重录为平台用例。

对应的新用例在 `../platform.iems.spec.ts` / `../platform.dcu.spec.ts`。
