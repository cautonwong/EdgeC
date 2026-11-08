# 07 应用界面：生成、声明、沙箱三层

状态：设计基线，待评审。未实现。

## 1. 应用能拥有什么界面

VS Code 的插件有自己的设置、状态和 webview。这套模型值得抄，但要抄对层级：插件能贡献视图、面板、菜单和状态栏，不能替换编辑器外壳。应用平台同样如此。

三层，越往下能力越强、审查越严：

| 层 | 形态 | 应用要做什么 | 平台保证 |
| --- | --- | --- | --- |
| L0 自动生成 | 应用详情页、设置表单 | 只在清单里声明 schema | 不用写任何界面代码 |
| L1 声明式 | 应用页面、首页卡片 | 写布局 JSON，用白名单组件 | 无脚本、无样式、无越权数据 |
| L2 沙箱 webview | 自定义面板 | 提供签名的静态包 | 隔离在 opaque origin，只能走窄 RPC |

P0 不实现任何一层界面。P1 做 L0 与 L1，P2 做 L2。

## 2. 应用不能拥有的

这些始终属于外壳，应用不能替换，也不能注入：

- 登录页、会话与权限管理。
- 左侧导航、顶栏、全局搜索、通知。
- 其他应用的页面与数据。
- 直接调用核心 API、读取平台 Cookie、访问 DataHub。
- 全局样式与主题变量。应用只能使用平台注入的 token。

这条边界是为了让现场操作面保持一个样子。应用可以有自己的面板，但不能让整个界面变成十个风格。

## 3. L0：自动生成

安装后自动出现，不需要清单之外的任何东西：

- 应用详情页：名称、版本、作者、签名摘要、运行状态、重启次数。
- 设置页：由第 06 号文档的 `settings` schema 生成，字段级校验与服务端错误回填。
- 数据页：点位列表、当前值、KV 用量、日志尾部。
- 权限页：已授予、待批准、已撤销三类，可逐项撤销。

应用详情页的入口固定在"应用"分组，不需要产品页面注册，也不需要改 WebUI。

## 4. L1：声明式页面与卡片

清单里写布局，平台用白名单组件渲染。

```yaml
contributes:
  ui:
    pages:
      - id: voltage-guard
        title: 电压越限
        permission: app.voltage_guard:read
        layout:
          - kind: metric-row
            items:
              - { label: 今日越限, source: app.voltage_guard.over_count, unit: 次 }
              - { label: 最长暂降, source: app.voltage_guard.max_sag_ms, unit: ms }
          - kind: time-series
            source: app.voltage_guard.sag_count
            range: 24h
          - kind: table
            source: app.voltage_guard.events
            columns:
              - { key: ts, title: 时间 }
              - { key: phase, title: 相别 }
              - { key: depth, title: 跌落深度, unit: '%' }
          - kind: actions
            items:
              - { label: 清空计数, action: app.voltage_guard.reset, confirm: true }
    widgets:
      - id: voltage-guard-summary
        title: 电压越限
        size: { w: 3, h: 2 }
        layout:
          - { kind: metric-row, items: [{ label: 今日越限, source: app.voltage_guard.over_count }] }
```

组件白名单，够用即可：

| 组件 | 用途 | 数据来源 |
| --- | --- | --- |
| `metric-row` | 指标组 | 点位或命名查询 |
| `status-list` | 状态列表 | 点位或命名查询 |
| `time-series` | 时序图 | 点位，需历史权限 |
| `table` | 表格 | 命名查询，服务端分页 |
| `form` | 表单 | 设置 schema |
| `actions` | 操作按钮 | 已声明的应用动作 |

规则：

- `source` 只能是点位名或清单里声明过的命名查询。平台在服务端解析成值再下发，前端拿不到任意查询能力。
- 每个页面、每个数据源各校验一次权限。页面可见但数据越权时，页面显示无权限，不返回空值。
- 应用不能提供 URL、脚本、样式、内联 HTML。文本字段按纯文本渲染。
- 操作按钮只能映射到清单里声明的动作，动作由平台 API 执行并做权限校验，不在浏览器里跑应用代码。
- 布局未知组件、未知字段一律拒绝渲染并记录错误，不做"尽量显示"。

## 5. L2：沙箱 webview

需要自定义交互的应用（例如相量图、拓扑编辑器）可以用 webview。它对应 VS Code 的 webview 面板，不是整页替换。

### 包结构

```
app.yaml
app.wasm
ui/
  index.html
  app.js
  style.css
  assets/...
```

- `ui/` 内所有文件都进清单摘要，签名覆盖。
- 禁止外链 CDN、远程字体、远程图片。所有资源在包内。
- 包体上限（初值 2MB）与文件数上限在安装时校验。
- 不允许 source map，不允许 `eval` 之外的动态代码加载（CSP 已禁止）。

### 隔离

平台用 iframe 承载，属性固定：

```
sandbox="allow-scripts"
src="/apps-static/<app-id>/<version>/index.html?token=<一次性>"
```

- **不加 `allow-same-origin`**。webview 运行在 opaque origin，读不到平台 Cookie、localStorage 和同源 API。
- 响应头带严格 CSP：`default-src 'none'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'none'; frame-ancestors 'self'`。
- `connect-src 'none'` 表示 webview 不能发网络请求。需要数据就走 postMessage。
- 静态资源由平台服务，带 `Content-Type`、`X-Content-Type-Options: nosniff`，不携带 Cookie。
- 一次性 token 绑定用户会话、应用与版本，短期有效，只用于加载资源，不能换取 API 权限。

### 通信

只有一条通道，`postMessage`，双向都做校验：

- 父页面校验 `event.source === iframe.contentWindow`，不信任 `event.origin`（opaque origin 为 `null`）。
- 消息带 `version`、`id`、`type`、`payload`。未知 `type` 丢弃并计数。
- 请求响应按 `id` 配对，超时丢弃。
- 平台不接受 webview 传来的 HTML、脚本或任意查询，只接受白名单方法。

RPC 方法白名单：

```
app.settings.get(key)
app.settings.set(key, value)        # 需 app.<id>:configure
app.points.read(name)               # 需已授予的 tags.read 范围
app.data.get(key) / app.data.put(key, value)
app.action.invoke(actionId, args)   # 需清单已声明且用户有权限
```

服务端对每次调用做三件事：确认调用者是该应用、确认资源在应用命名空间内、确认用户权限。webview 不能借用户的会话去调用核心 API，也不能读别的应用的数据。

### 主题与可访问性

- 平台向 webview 注入主题 token 的 CSS 变量，应用用变量，不硬编码颜色。
- 平台提供浅色与深色两套变量，主题切换时通过 postMessage 通知。
- webview 必须支持键盘焦点、`prefers-reduced-motion`，并在应用详情页显示"自定义界面"标记，让现场人员知道这不是平台生成的页面。

## 6. 生命周期与缓存

- 安装时注册界面贡献；卸载时移除路由与静态资源目录。
- 升级时新版本静态包以版本号隔离存放，激活后切换引用；旧版本保留一代用于回滚。
- 静态资源 URL 带版本或内容哈希，长缓存；切换 generation 后旧 URL 失效。
- webview 加载失败（缺文件、摘要不符）时显示平台错误页，不白屏，不降级成不受控页面。

## 7. 权限与威胁

| 威胁 | 控制 |
| --- | --- |
| 页面越权读数据 | 每个数据源单独校验；服务端解析，前端拿不到查询能力 |
| webview 读取平台会话 | opaque origin，无 same-origin，无 Cookie |
| webview 外联或回传数据 | CSP `connect-src 'none'`，资源全部本地 |
| 伪造消息来源 | 校验 `event.source`，不接受任意 window 的消息 |
| 借用户权限调用核心 API | RPC 白名单加应用命名空间校验，服务端拒绝跨应用资源 |
| 恶意静态包 | 签名覆盖全部文件，安装时校验摘要，运行时再校验一次 |
| 界面钓鱼 | webview 强制显示归属应用与"自定义界面"标记，不允许隐藏 |

## 8. 与 VS Code 的对应

| VS Code | 本平台 | 差异 |
| --- | --- | --- |
| `contributes.configuration` | `settings` schema 生成表单 | 无运行时设置弹窗，管理员配置 |
| `globalState` / `secrets` | `app_kv` / `app_secret` | 无 workspace 概念，按设备隔离 |
| `contributes.views` | L1 `ui.pages` / `ui.widgets` | 组件白名单，不允许自定义树控件 |
| webview | L2 沙箱 iframe | 无 Node 集成，无网络，无同源 |
| `contributes.commands` | `ui.actions` | 只能调用已声明的应用动作，不能执行任意命令 |
| 扩展宿主进程 | app-host / sidecar | 已在第 04 号文档定义 |

## 9. 分阶段

- P0：不做界面，只有命令行与状态接口。理由是先验证事务、授权与隔离。
- P1：L0 自动详情页与设置页，L1 只读页面与首页卡片。入口条件：06 号文档的参数模型跑通。
- P2：L2 webview。入口条件：CSP、opaque origin、RPC 白名单三项安全测试通过。

## 10. 验收

1. 安装应用后，应用详情页与设置页自动出现，未改 WebUI 代码。
2. 卸载后页面、卡片、静态资源目录全部消失。
3. L1 页面引用未授权点位，页面显示无权限，不返回空值。
4. L1 布局含未知组件或未知字段，拒绝渲染并记录错误。
5. L1 操作按钮只能触发清单声明的动作，未声明的动作被拒。
6. webview 无法访问平台 Cookie 与同源 API，尝试时被 CSP 或 origin 隔离阻断。
7. webview 发起外部网络请求，被 `connect-src 'none'` 阻断。
8. 非 iframe 窗口向父页面发送消息，被丢弃。
9. webview 调用未在 RPC 白名单内的方法，被拒绝并计数。
10. webview 读取另一个应用的点位，返回 `denied_scope`。
11. 安装包含超大静态包或缺失文件的包，安装被拒。
12. 升级后旧静态资源 URL 失效，新版本生效；回滚后旧 URL 恢复。
13. 主题切换后 webview 收到通知并使用新 token。
14. 应用详情页明确标记"自定义界面"，不可隐藏。

## 11. 未决

- L1 组件目录是否需要树形控件与地图控件，取决于真实页面需求。
- webview 是否允许内置 WASM（例如本地图表加速），目前不允许，避免再开一层运行时。
- 命名查询的分页与聚合语义，是否在 P1 就支持，还是只给原始点位。
- 自定义界面与平台生成页面能否混排在同一个应用页，还是必须分 tab。
