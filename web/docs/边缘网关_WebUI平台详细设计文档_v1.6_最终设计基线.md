# 边缘网关 WebUI 平台详细设计文档

适用产品：iEMS、DCU 等 Linux 边缘网关 ｜ 版本：v1.6（最终设计基线）｜ 日期：2026-11-01

**文档状态**：最终设计基线（文档冻结版）。本版冻结平台设计规则、接口/权限语义、工程任务与发布门禁；表中仍标记“待决”的 ADR、型号差异和产品功能适用性，必须由项目团队按规定流程确认。文档冻结不代表 ADR 已全部关闭、代码已实现、目标硬件已验证或产品已获安全认证。

**方案摘要**：iEMS、DCU 等网关共用一个 WebUI 平台，由共享的 Core 加每个产品自己的 Product Pack 组成。产品页面由构建产物、设备能力、功能授权和用户授权共同决定；后端对每个请求执行能力、权限和资源范围检查。产品扩展在构建期静态组装，不允许业务服务运行时任意增加外部路由。关键事件采用“先持久化、后通知”的可恢复投递模型；长任务按至少一次恢复、步骤幂等和不确定状态人工/受控恢复设计，不宣称跨进程 exactly-once。首发发布范围按 §14.1、§17.1、附录 D–F 的 P0/P1 规则统一判定。

| 决策 | v1.6 设计基线 | 详见 |
| --- | --- | --- |
| 多产品组织方式 | Monorepo；Core 与 Pack 单向依赖；Pack 通过 Manifest 和受控扩展点接入 | 第 4 章 |
| 产品差异的装配时机 | 构建期裁剪并生成静态注册表；运行期仅做能力/授权判定与页面懒加载，不加载任意插件 | 4.3–4.5、7.3 |
| 页面可见性 | 产品构建清单 ∩ 设备能力 ∩ 有效功能授权 ∩ 用户权限；资源范围另行约束数据 | 4.5、5.4 |
| 权限边界 | webd 是外部 API 的认证、授权和审计入口；IPC 另有调用方身份与方法 ACL；前端显隐不构成安全边界 | 3、5、7、10 |
| API 与权限 ID | 全部 API 使用 `/api/v1`；权限 ID 统一为 `domain.resource:action`，并由 CI 校验 | 5.2、7.2、附录 A |
| 实时通道 | WebSocket 只允许订阅/退订；业务写操作走有明确权限、幂等和审计的 HTTP API | 6.3、7.1、7.2 |
| 高风险配置 | 持久化事务 + 设备端独立 watchdog；默认 60 秒确认窗口仅作为初始值，必须按配置类别及真机验证 | 6.4、7.2、12 |
| 升级与恢复 | A/B、可信启动和硬件密钥按型号声明能力，不作为所有设备均具备的前提 | 10、11、17 |
| 视觉体系 | Design Token 三层结构；对所有语义前景/背景组合做对比度检查 | 第 9 章 |
| 首屏体积 | gzip 后 ≤ 300KB 作为当前预算目标；还需真机测量运行时 RAM、延迟和连接压力 | 6.8、12.8 |
| 工程交付 | ADR 状态、工程任务 ID、需求追踪、CI 门禁和真机证据必须可追踪；未验证项不得标记为已完成 | 14、17、18、附录 D–F、J |
| 事件与长任务 | 关键事件持久化后发布；任务恢复按至少一次执行与幂等步骤处理；订阅权限撤销后立即失效 | 6.3、7.5、10、12、附录 D–F、J |
| 发布范围 | P0 为架构/安全基线；适用 P1 是首个产品发布要求；条件性功能由发布范围清单约束 | 14.1、15.1、17.1、附录 D–G、J |

**v1.6 最终整合原则**：本版以 v1.5 为唯一修订基线，完成最终一致性收口。规范正文为第 1–18 章、附录 A–G 和附录 J；附录 H（v1.4）与附录 I（v1.5）仅保留历史审计记录，不再作为当前规范依据。对于尚未确定的硬件/软件选型，保留 ADR 决策边界；对于条件性功能，以每个产品构建对应的 `release-scope.yaml` 明确是否开放，并由构建产物和 API/Registry 自动校验。

## 1. 背景、目标与约束

### 1.1 背景

公司有多条边缘网关产品线（iEMS、DCU 等），均运行 Linux，各自独立开发 WebUI 会带来：重复造轮子（登录、网络、升级、日志）、风格不一致、权限体系各写一套、维护成本高。本文设计一套统一的 **WebUI 平台**：一个共享的 **Core**，加上每个产品自己的 **Product Pack**（自定义页面、组件、主题、能力声明），并通过统一的 **多用户 / 角色 / 权限** 体系，让不同用户登录后看到不同数量、不同内容的页面。

### 1.2 设备侧约束（决定架构取舍）

| 约束 | 典型情况 | 对设计的影响 |
| --- | --- | --- |
| 资源受限 | RAM 256MB\~2GB，Flash 16\~512MB | 前端产物小、后端常驻进程少、静态资源预压缩 |
| 离线 / 局域网 | 现场无外网，不能用 CDN、Google Fonts | 所有资源本地化，字体自带子集 |
| 访问方式 | 笔记本、平板，偶尔手机；浏览器版本不一 | 桌面优先 + 响应式，目标浏览器近 3 年主流版本 |
| HTTPS | 自签证书或设备证书 | 默认 HTTPS，HTTP 仅跳转；支持证书替换 |
| 硬件差异大 | 同一产品不同型号有无 4G、LoRa、RS485 路数不同 | 页面按 Capability（能力）动态显隐 |
| 固件升级 | 整包升级，前端随固件发布 | 前后端版本强绑定，API 带版本号，兼容策略明确 |
| 安全合规 | 工控/能源场景，需按产品销售区域、客户和目标市场确定适用法规/标准 | 强制改初始密码、审计、会话安全、最小权限；形成适用性判断与证据追溯，不以本设计文档宣称合规 |

### 1.3 目标与非目标

**目标**

- 一套 Core 支撑所有产品，新产品接入只需编写 Product Pack，不改 Core。
- 页面级、功能级、数据级三层权限；菜单与路由由权限自动生成。
- 统一的设计语言（Design Token），各产品可换肤但保持一致体验。
- 配置变更具备「草稿 → 校验 → 应用 → 确认 / 自动回滚」事务机制；回滚计时与恢复由设备端独立于 Web 请求生命周期的事务协调器保障，避免改网络把自己锁在门外。
- 前端首屏 gzip 后 ≤ 300KB，低配设备上可流畅使用。

**非目标**

- 不做云端多租户管理平台（那是云平台的职责），本文只覆盖设备本地 WebUI。
- 不支持第三方在设备上运行任意代码的开放插件市场（仅支持公司内部产品包，安全边界清晰）。

### 1.4 设计原则

1. **后端是权限的唯一权威**：前端隐藏只是体验优化，每个 API 必须服务端鉴权。
2. **声明式优于命令式**：页面、菜单、权限、表单尽量用 manifest / schema 声明，减少手写样板。
3. **Core 稳定、Pack 灵活**：Core 对外提供版本化的 SDK，Pack 只能通过扩展点接入。
4. **能力驱动**：页面是否出现取决于「设备有什么」，而不是「写死产品型号」。
5. **轻量优先**：能用静态文件 + 少量 API 解决的，不引入重型框架和常驻服务。
6. **安全条件合取**：页面可见性不是授权；API 执行必须同时满足身份有效、权限匹配、资源范围匹配、设备能力可用和功能授权有效。
7. **失败安全**：授权、能力、事务状态或可信配置无法确定时，对写操作采取 fail-closed；只读降级必须由接口明确允许。
8. **单一事实来源**：权限目录、OpenAPI、Schema、Manifest 输入和生成注册表各有明确权威来源，生成产物不得手工修改。

## 2. 业界参考与取舍

| 参考方案 | 值得借鉴 | 我们的取舍 |
| --- | --- | --- |
| OpenWrt LuCI | 菜单与 ACL 均用 JSON 声明（menu.d / acl.d）；配置先暂存再应用；改网络后带回滚计时器 | 借鉴声明式菜单 + ACL，借鉴「应用并 N 秒内确认否则回滚」 |
| Cockpit | 每个功能是独立 package，带 manifest；按用户权限显示 | 借鉴 package + manifest 的扩展模型 |
| Home Assistant 前端 | 实体 / 卡片 / 面板的组件化；主题 Token 化 | 借鉴 Dashboard 卡片注册机制和主题变量 |
| Grafana | 插件有 plugin.json、声明能力与依赖；组织 / 角色 / 权限分层 | 借鉴 manifest 字段设计和角色模型 |
| VS Code | Contribution Points（贡献点）：插件向宿主声明扩展 | 借鉴「扩展点」概念：路由、菜单、卡片、设置项 |
| 工业网关厂商（Moxa、Advantech、西门子 SCALANCE 等） | 左侧树状导航 + 顶部状态栏 + 配置后应用；角色分 Admin / Operator / Viewer | 借鉴信息架构与角色划分，视觉上现代化 |

结论：采用 **「Core 壳 + Manifest 声明 + 扩展点 + Capability 驱动 + RBAC」** 的组合方案，构建期按产品裁剪，运行期按能力与权限动态呈现。

### 2.1 EdgeX UI 的借鉴点与取舍

EdgeX UI 是官方提供的演示和开发用管理界面，面向单个 EdgeX 实例，官方说明不建议用于生产（来源：[edgex-ui-go 说明](https://github.com/wallong/edgex-ui-go)、[EdgeX GUI 文档](https://docs.edgexfoundry.org/3.1/getting-started/tools/Ch-GUI)）。因此我们借鉴它的交互模式，权限、安全和产品扩展由本方案自行设计。

| 借鉴点 | EdgeX 的做法 | 我们的落地 | 对应章节 |
| --- | --- | --- | --- |
| Dashboard 下钻 | 磁贴点击后跳到对应详情页 | 概览卡片统一支持点击下钻，widget 声明跳转目标 | 8.4 |
| 对象互相跳转 | 设备行链接到关联档案与服务，档案行可反查设备 | iEMS 数据点与设备、DCU 电表档案与抄表方案做双向链接 | 8.7 |
| 模板驱动的向导 | 选设备服务、选档案模板，提交时一次建好关联 | 点表 / 电表档案模板，批量创建设备 | 8.3 |
| 设备命令调试 | 选资源或命令，直接试 GET / SET | Engineer 及以上的点位读写调试页，需写权限 | 8.1 |
| 实时数据流 | Data Center 展示事件与读数 | 协议报文监视页，增加暂停、过滤、缓存上限 | 8.3 |
| 系统服务管理 | 启停服务，查看 CPU、内存、网络与配置 | 诊断分组的服务状态页，重启需 execute 权限 | 8.1 |
| 侧栏与响应式 | 可折叠为图标栏，窄屏降级 | 沿用 9.4 的断点策略 | 9.4 |

不照搬的部分：

- 有教程中的默认账号为 admin / admin（[来源](https://www.ubuntu.com/blog/turning-your-raspberry-pi-4-into-an-edge-gateway-part-i)），我们强制首次改密，不留通用默认口令。
- 未见其细粒度角色模型的公开说明，RBAC、页面级与 API 级鉴权由我们自行实现。
- 事件数据以 JSON 形式展示，我们面向现场人员做成带单位和状态色的表格与曲线，原始 JSON 作为折叠项。
- 菜单按后端微服务划分，我们按任务分组（第 8.1 节），更贴近使用场景。
- 它没有产品扩展机制，这一块由 Core + Product Pack 补上。

## 3. 总体架构

### 3.1 分层视图与请求路径

```text
浏览器
  │ HTTPS（Host / Origin / 会话校验）
  ▼
静态资源服务器 / 反向代理（TLS 终止点，须明确且唯一）
  ├── 静态资源：只读文件
  └── /api/v1、/api/v1/ws
          ▼
       webd（唯一外部业务 API 入口）
       认证 / 会话 / CSRF / 权限 / 资源范围 / Capability / 审计 / 限流
          │ 受控本机 IPC（UDS / ubus / D-Bus，按 ADR 选择）
          ▼
       产品业务服务 / 系统服务
       再按 IPC peer、方法 ACL 和业务对象约束执行
```

图中自上而下是一次请求的路径。静态服务器可以与 webd 合并部署，也可以独立部署；若独立部署，必须通过本机受限链路代理 API，不能直接把业务服务暴露给浏览器。TLS 终止点由 ADR 冻结，不允许部署环境自行形成多套互不一致的代理信任链。

**边界规则**：

1. **外部入口**：除明确的 HTTP→HTTPS 跳转外，业务 API 只通过 webd 暴露。监听地址、端口和网卡由设备部署配置控制，默认不暴露到非管理接口。
2. **TLS 与代理**：若由 Nginx/lighttpd 终止 TLS，webd 只信任明确配置的本机代理身份/地址；客户端提交的 `Forwarded`、`X-Forwarded-*` 头一律清除，只有可信代理重新写入的值才可使用。
3. **Host / Origin**：代理与 webd 对 Host 使用 allowlist；状态变更请求校验 Origin；WebSocket 握手也校验 Host、Origin、会话和 CSRF/握手策略，防止 DNS Rebinding、跨站请求与伪造代理头。
4. **本机 IPC**：业务服务只监听受限 Unix Domain Socket 或受控系统总线，不监听外部网卡。Socket 文件权限、服务用户/组、peer credential、总线 ACL、允许调用的方法、消息大小、超时、取消和错误语义必须定义。
5. **服务端授权上下文**：webd 在可信 IPC 请求中传递不可由客户端伪造的主体、权限决策标识、资源范围、requestId 和策略版本。业务服务验证 IPC 调用方与上下文来源；业务服务若可被其他本机组件调用，必须为这些调用方定义独立 ACL，不能只因“正常流量经过 webd”就取消服务自身边界。
6. **审计责任**：webd 对外部用户操作形成统一审计事件；业务服务对实际执行结果和业务对象变化返回关联 ID。审计失败策略由操作风险等级决定，不能静默丢弃高风险操作记录。
7. **实时数据**：遥测、告警、任务进度通过 WebSocket 只读订阅推送。WebSocket 不提供通用 `publish` 写入能力；业务写操作统一经鉴权 API。

### 3.2 技术选型建议

| 层 | 建议 | 冻结条件 / 备选 |
| --- | --- | --- |
| 前端框架 | Vue 3 + TypeScript + Vite | 体积、目标浏览器和团队能力验证后冻结；备选 React + Vite |
| UI 基础 | 自研 Design Token + Headless 组件按需引入 | 禁止整包引入导致设备资源失控；Pack 只能使用 core-ui |
| 状态管理 | Pinia | 权限/能力状态必须可失效、可刷新；缓存不能作为后端授权依据 |
| 请求层 | Axios / ofetch + TanStack Query（可选） | 统一 requestId、超时、取消、重试和错误码；写操作不得盲目自动重试 |
| 图表 | ECharts 按需引入或 uPlot | 通过真机资源和点数压力测试选择 |
| 表单 | JSON Schema 驱动的薄封装 | 前后端使用同一权威 Schema 版本；服务端校验为准 |
| 国际化 | vue-i18n，按模块懒加载 | Core 与 Pack 独立命名空间，构建检查 key 完整性 |
| 静态资源服务器 / TLS 终止 | Nginx 或 lighttpd（二选一） | 以目标设备实测、Host/Origin 校验、代理 ACL 和维护成本决定；TLS 终止点必须唯一明确 |
| 后端 webd | Go 或 C++（与现有服务栈一致） | 最终选择须有资源实测、IPC 与服务管理方案 |
| IPC | 首选 Unix Domain Socket；也可使用 ubus / D-Bus | 需完成 peer identity、ACL、超时、取消、消息尺寸和错误语义 ADR |
| API 契约 | OpenAPI 3 为 HTTP 契约单一事实来源 | 生成 TS 客户端、Mock、静态路由/权限注册表和授权测试 |
| 前端测试 | Vitest + Playwright + 设备 Mock Server | Mock 与真机契约均需验证 |

## 4. 多产品架构：Core + Product Pack

### 4.1 代码仓库结构（Monorepo）

```
webui/
├─ packages/
│   ├─ core-sdk/          # 对 Pack 暴露的稳定 API（类型、扩展点、hooks），语义化版本
│   ├─ core-ui/           # 基础组件库 + Design Token（按钮、表格、表单、状态标签…）
│   ├─ core-app/          # App Shell：登录、布局、路由、菜单、权限、i18n、主题
│   ├─ core-modules/      # 通用功能模块：dashboard、network、users、system、logs、upgrade…
│   └─ tooling/           # 构建插件：manifest 校验、产品组装、体积检查
├─ products/
│   ├─ iems/              # iEMS Product Pack
│   │   ├─ product.yaml   # 产品清单（manifest）
│   │   ├─ pages/         # 产品专属页面
│   │   ├─ components/    # 产品专属组件
│   │   ├─ locales/       # 产品语言包
│   │   └─ theme.css      # 产品主题覆盖（仅 Token）
│   └─ dcu/               # DCU Product Pack（结构同上）
├─ mock-server/           # 设备模拟服务（按产品加载 fixtures）
└─ openapi/               # API 契约（core.yaml + 各产品 extension.yaml）
```

依赖方向严格单向：`Product Pack → core-sdk / core-ui`，Core 永远不引用任何 Pack，用 lint 规则（依赖边界检查）强制。

### 4.2 Product Pack 清单（product.yaml）示例

`product.yaml` 是**构建输入**，不是设备运行时动态路由注册指令。示例中的每一项权限引用都必须在本文件的 `permissions` 或 Core 权限目录中声明；CI 不允许悬空引用。

```yaml
id: iems
name: iEMS 能源管理网关
version: 2.3.0
coreSdk: ^1.4.0

theme:
  brandColor: '#0B6BCB'
  logo: assets/logo.svg

modules:
  core:dashboard: { enabled: true }
  core:network:   { enabled: true, hiddenFeatures: [wifi-ap] }
  core:users:     { enabled: true }
  core:upgrade:   { enabled: true }

pages:
  - id: iems.datapoints
    path: /datapoints
    component: ./pages/DataPoints.vue
    title: iems.datapoints.title
    menu: { group: collect, order: 20, icon: database }
    permissions: { allOf: [iems.datapoint:read] }
    requires:
      capabilities:
        - { id: software.collector.modbus, op: eq, value: true }

  - id: iems.strategy
    path: /strategy
    component: ./pages/Strategy.vue
    title: iems.strategy.title
    menu: { group: control, order: 10, icon: bolt }
    permissions: { allOf: [iems.strategy:read] }
    requires:
      entitlements:
        - { id: iems.peak-shaving, state: active }

widgets:
  - id: iems.power-flow
    component: ./components/PowerFlowCard.vue
    size: { w: 6, h: 4 }
    permissions: { allOf: [iems.telemetry:read] }

settingsSections:
  - id: iems.cloud
    component: ./components/CloudLinkSettings.vue
    permissions: { allOf: [iems.cloud:read] }
    writePermission: iems.cloud:write

permissions:
  - { id: iems.datapoint:read,  desc: 查看数据点 }
  - { id: iems.datapoint:write, desc: 编辑数据点 }
  - { id: iems.telemetry:read,  desc: 查看 iEMS 遥测数据 }
  - { id: iems.strategy:read,   desc: 查看策略 }
  - { id: iems.strategy:write,  desc: 编辑策略 }
  - { id: iems.cloud:read,      desc: 查看云连接配置 }
  - { id: iems.cloud:write,     desc: 修改云连接配置 }

rolePresets:
  viewer:
    - iems.datapoint:read
    - iems.telemetry:read
    - iems.strategy:read
  operator:
    - iems.datapoint:read
    - iems.telemetry:read
    - iems.strategy:read
  engineer:
    - iems.datapoint:read
    - iems.datapoint:write
    - iems.telemetry:read
    - iems.strategy:read
    - iems.strategy:write
    - iems.cloud:read
    - iems.cloud:write
  admin:
    - iems.datapoint:read
    - iems.datapoint:write
    - iems.telemetry:read
    - iems.strategy:read
    - iems.strategy:write
    - iems.cloud:read
    - iems.cloud:write
```

规范：

- `id` 全局唯一；权限 ID 遵循 `domain.resource:action`，大小写敏感。示例权限与页面、Widget、设置分区、角色预置引用必须完全一致。
- `permissions.allOf` 表示进入页面/使用该功能需要同时满足的权限；不得用字符串简写或隐含 OR 语义。确需任一权限即可时，必须显式使用 `anyOf`，并由安全评审批准。
- `requires.capabilities` 只表达硬件/软件能力条件；`requires.entitlements` 只表达许可证/功能授权条件；健康状态、在线状态和运行时故障不得伪装成能力声明。
- 页面显隐条件只改善体验，不替代服务端 API 对能力、权限和资源范围的重新校验。
- `rolePresets` 只给预置角色追加该产品权限，不允许移除 Core 权限、扩大 Core 角色的全局资源范围或覆盖安全策略。新增权限默认不授予自定义角色。
- `product.yaml`、OpenAPI、JSON Schema、权限目录及资源文件在构建时校验并生成静态注册表；设备运行时不得从可写目录加载任意页面代码或新增外部路由。

### 4.3 扩展点（Contribution Points）

| 扩展点 | 说明 | 冲突规则 |
| --- | --- | --- |
| routes / menu | 新增页面与菜单项，可指定分组与排序 | ID 全局唯一；重复构建失败 |
| widgets | Dashboard 卡片，网格布局，用户可显隐 / 排序 | 默认 ID 全局唯一；仅 Core 显式标记 `overridable: true` 的 Widget 可被白名单替换 |
| settingsSections | 在「系统设置」等聚合页追加分区 | ID 全局唯一；按 order 排序 |
| formFields | 向 Core 表单追加字段 | 通过 Schema 的 extension slot 注入；字段 ID 唯一 |
| fieldRenderers | 自定义字段渲染器 | 类型名注册必须在构建清单中唯一；不得运行时抢占 |
| overrides | 替换 Core 页面 / 组件 | 仅允许 Core 白名单中的目标；须评审并记录来源、版本、审批人 |
| i18n | 追加文案 | 产品文案放在 `<product-id>.*` 命名空间下；禁止覆盖 Core 安全提示 |
| theme | 覆盖 Design Token | 仅允许被声明可覆盖的品牌 Token；禁止任意全局 CSS |
| permissions / rolePresets | 声明产品权限与预置角色追加授权 | 不允许删除 Core 权限；所有引用必须可解析 |
| API operations | 声明产品 HTTP API 契约和权限绑定 | 由 OpenAPI 构建期生成静态路由/策略注册表，不支持运行时注册 |

**ID 与覆盖规则**：默认所有扩展 ID 在最终固件注册表中全局唯一。唯一例外是 Core 事先声明为可覆盖的 Widget/组件；覆盖必须指定目标 ID、来源 Pack、目标 Core SDK 版本和审批记录。禁止“后注册者自然覆盖”，禁止依赖打包顺序决定行为。

三种定制深度，从轻到重：

1. **配置级**：隐藏 / 重排 Core 模块；隐藏仅影响导航，不构成安全控制。
2. **扩展级**：追加页面、卡片、字段、设置分区和 API operation。
3. **替换级**：整体替换白名单内 Core 页面/组件，必须经平台、安全和产品评审。

### 4.4 组装方式：构建期裁剪 + 运行期懒加载

- **构建期**：`tooling` 读取 `product.yaml`、该产品/版本的 `release-scope.yaml`、Core 模块清单、OpenAPI、权限目录和 JSON Schema，按产品功能清单与发布范围的合取结果裁剪前端模块与业务 API 契约；只有两者均启用的功能才生成最终产物。生成 `route-registry`、`permission-registry`、`capability-requirements`、Schema 索引、release-scope 摘要和构建报告，随固件只读交付。
- **运行期**：页面按 chunk 懒加载；启动后根据当前设备能力、有效功能授权和用户权限生成菜单/路由视图。运行期可以隐藏不满足条件的页面，但不能加载新代码、增加外部 API 路由或修改权限目录。
- **设备端注册表**：静态注册表应包含产品 ID、版本、Core SDK 版本、路由/操作摘要、权限绑定、Schema 版本、release-scope 摘要和制品摘要。构建期必须验证 `product.yaml` 与 `release-scope.yaml` 的功能开关一致性；发布范围关闭的功能不得残留外部路由或可调用执行入口。启动时只校验摘要、签名/来源和版本兼容性，不接受业务服务通过 IPC 临时添加外部路由。
- **为什么不做运行期动态插件加载**：固件整包升级，前后端版本绑定；构建期组装更容易审计、测试、回滚和控制资源。若未来提出第三方热插拔需求，必须重新进行威胁建模和架构 ADR，不能直接启用 Module Federation。

### 4.5 Capability（能力）与功能授权机制

能力、许可证授权和运行健康状态必须分离：

| 类别 | 含义 | 示例 | 是否可单独授权 API |
| --- | --- | --- | --- |
| Hardware capability | 设备实际装配的硬件资源及数量 | RS485 端口数、以太网口数、SD 卡 | 否；只能作为 API 可用前置条件 |
| Software capability | 固件中编译/安装且可运行的软件功能 | Modbus 采集、IEC 104、某协议驱动 | 否；只能作为 API 可用前置条件 |
| Entitlement | 经签名验证且对本设备有效的许可/功能授权 | 削峰填谷、云连接高级功能 | 否；仅与权限及资源范围共同判定 |
| Runtime health | 服务在线、链路状态、错误状态和资源水位 | collector healthy、RS485 busy | 否；用于状态展示/操作前健康检查，不得当作永久能力声明 |

建议运行时响应结构：

```json
{
  "productId": "iems",
  "model": "IEMS-300-4G",
  "firmware": "2.3.0",
  "capabilityRevision": 17,
  "schemaVersion": 1,
  "hardware": {
    "network.ethernet.ports": 2,
    "io.rs485.ports": 2,
    "storage.sdcard": true
  },
  "software": {
    "collector.modbus": true,
    "collector.iec104": true
  },
  "entitlements": {
    "iems.peak-shaving": {
      "state": "active",
      "expiresAt": null
    }
  },
  "runtimeHealth": {
    "collector.modbus": "healthy"
  }
}
```

规则：

1. 设备启动后由可信设备服务生成能力快照；`capabilityRevision` 在能力/许可集合变化时递增。`GET /api/v1/system/capabilities` 需要登录及 `system.capability:read`，不对匿名网络开放。
2. `GET /api/v1/system/info` 仅匿名返回登录页必需的最小字段（产品展示名、语言、登录页主题版本等）；不得暴露完整型号配置、序列号、网络拓扑、固件内部组件清单或许可证细节。具体字段由 allowlist 固定。
3. 每个业务 API 在服务端重新检查：身份/会话有效、所需权限、资源范围、硬件/软件能力、Entitlement 状态和必要的运行健康前置条件。任一条件不满足即拒绝；不能以菜单隐藏、路由守卫或旧缓存代替。
4. Entitlement 必须定义签名算法/信任根、设备绑定、有效期、时钟回拨处理、撤销/宽限策略和升级兼容。签名错误、设备绑定不匹配或状态未知时，受许可保护的写操作 fail-closed。
5. 未知能力、能力服务不可用或 `capabilityRevision` 不一致时，写操作 fail-closed；前端收到能力版本变化通知后刷新页面能力视图。浏览器的能力缓存只用于呈现，不具备授权效力。
6. 页面声明 `requires`，不满足时菜单不出现；直接输入 URL 时显示“当前设备不支持”或“功能未授权”。后端仍必须独立执行相同条件检查。

### 4.6 版本与兼容规则

- core-sdk 遵循 semver：主版本变更才允许破坏性变更，Pack 在 manifest 中声明 `coreSdk` 范围，构建时校验。
- 扩展点的新增向后兼容；废弃的扩展点至少保留一个主版本并在构建时给出警告。
- API 路径带版本 `/api/v1`，同一固件内前后端配套发布；字段只增不删，删除需先标记 deprecated。
- CI 中对每个产品跑「Pack 构建 + 契约测试 + E2E」，Core 变更自动回归所有产品。

## 5. 用户、角色与权限设计

### 5.1 授权模型与强制检查链

保留页面、功能、数据/API 三层授权呈现，但授权决策统一由后端策略引擎执行。一次请求的允许条件为：

```text
Allow = 身份有效
     AND API 所需权限匹配
     AND 主体对目标资源具有有效范围
     AND 设备能力满足
     AND 功能授权有效（若该 API 需要）
     AND 环境/状态策略满足（若声明）
```

| 层级 | 控制对象 | 实现与安全边界 |
| --- | --- | --- |
| 页面级 | 菜单是否出现、路由能否进入 | 前端根据 `permissions`、能力和授权呈现；后端不信任此结果 |
| 功能级 | 按钮、表单、危险操作 | `v-permission` / `usePermission()` 仅作 UI 控制；写 API 仍逐请求授权 |
| 数据/API 级 | 接口能否调用、能看到/修改哪些对象 | 后端对每个 operation 检查权限与资源范围；批量接口逐对象授权或拒绝整批 |
| 设备能力/授权 | 当前固件/硬件/许可证是否支持 | 服务端依据可信能力快照和有效许可证检查，不依赖客户端提交的 capability |

HTTP、WebSocket 订阅、后台任务、IPC 和调试通道必须使用同一授权策略语义；不得各自维护一套容易漂移的权限判断。

### 5.2 权限点命名与语义

唯一格式：`domain.resource:action`。例如：`network.interface:read`、`system.upgrade:execute`、`user.account:admin`、`iems.strategy:write`。权限 ID 大小写敏感，不允许空格、自由文本别名或未登记的简写。

动作集合：

- `read`：读取该资源；是否包含敏感字段由字段级数据策略另行决定。
- `write`：创建、修改或删除该资源的普通业务数据；不能仅凭 `write` 推导出系统级 `execute`。
- `execute`：触发一个明确的任务或操作，如升级、重启、备份、抓包。
- `admin`：管理该资源的授权/生命周期，如用户或角色管理；不自动隐含所有其他动作。

**不隐含动作继承**：`write` 不自动包含 `read`，`execute` 不自动包含 `read`，`admin` 也不自动包含所有动作。确需依赖（例如 UI 编辑必须同时查看）在权限目录的 `dependencies` 中显式声明，并由构建检查和授权策略实现，不依赖前端猜测。

**通配符**：只允许在角色/权限模板的构建输入中使用，如 `iems.strategy:*`；构建时展开为具体权限 ID 并保存解析结果。运行时请求和 API operation 不允许使用通配符。`*` 不能绕过产品、资源范围、能力或 Entitlement 检查。

权限目录由 Core 与各 Pack 声明合并生成。CI 必须检测：重复 ID、未声明引用、未使用权限、废弃权限引用、角色预置悬空引用、API operation 未绑定权限、页面/Widget 无权限声明及不合法通配符。

### 5.3 预置角色与角色约束

| 角色 | 定位 | 典型权限 | 约束 |
| --- | --- | --- | --- |
| Viewer 查看员 | 只读巡检 | Dashboard、遥测、告警、允许查看的日志 | 不包含用户、密钥、证书私钥和敏感配置读取 |
| Operator 运维员 | 日常运维 | Viewer + 告警确认、受控服务操作、有限任务 | 不默认允许网络/证书/固件等高风险写操作 |
| Engineer 工程师 | 现场调试、集成 | Operator + 通信/采集/点表/策略配置、限定网络配置、调试 | 调试写操作受通道租约、时间限制和审计约束 |
| Admin 管理员 | 设备所有者 | 用户/角色、系统、安全配置、升级、备份恢复 | 不应绕过设备能力、不可逆操作保护或本地恢复机制 |
| Service 厂商服务 | 厂商售后 | 明确授权的诊断、抓包、调试、日志导出等 | 默认禁用/不显示；需设备绑定、短时授权、范围、原因、审批/客户授权和全量审计；不得存在通用后门口令 |

内置角色只是模板。每个角色赋予的资源范围必须明确；本地单设备部署中 `all` 仅表示当前网关及其子资源，不代表云端或其他网关。Admin 也不能仅通过 UI 设定绕过服务端策略。

管理员可创建自定义角色，但必须遵守：

1. 授权者只能授予其自身有权授予的权限，不得通过编辑角色提升自己或同级账号权限。
2. 不允许删除、禁用或移除最后一个可用管理员；需要账号恢复流程。
3. 角色变更、权限授予、资源范围调整和 Service 授权均需写入安全审计。
4. 高风险权限（恢复出厂、固件回退、远程控制/物理输出等）可配置二次审批或双人控制；具体范围由产品安全 ADR 确定。
5. 权限新增默认不授予自定义角色；内置角色按版本化 Pack preset 迁移，并生成变更报告。

### 5.4 资源范围与页面显隐算法

权限 ID 只描述“能做什么”，资源范围单独描述“能对哪些对象做”。资源范围可按设备内对象层级表达，例如：

- `gateway:self`：当前网关本身；
- `device-group:<id>`：某设备组；
- `device:<id>`：某个设备/电表；
- `channel:<id>`：某个通信通道；
- `point-group:<id>`：某点位组。

具体资源层级由产品 Pack 定义，必须提供父子关系解析和对象归属校验，不能让客户端直接声明任意 scope 字符串。

授权合成规则：

1. 用户通过多个角色获得的授权按**并集**合并；每个角色授予的权限和资源范围作为一个独立 grant。用户直接额外授权也参与并集，但必须记录授予者、原因、到期时间和授权依据。
2. 不定义隐式拒绝优先级；缺少任一必需条件即拒绝。若未来必须引入显式 deny，须另做 ADR 并明确 deny 优先级、组合规则和迁移行为。
3. 用户直接额外授权不能扩大授予者本身无权授予的权限或资源范围，且应支持到期自动失效。
4. 多对象/批量 API 必须逐对象执行范围检查；如无法安全部分成功，应采用整批拒绝并返回可审计的失败对象摘要。
5. 资源 ID、父子关系和资源归属必须从服务端可信数据解析；不能信任客户端随请求提交的“所属设备/组”。

页面可见条件：

```text
可见页面 = 构建产物包含该页面
          AND requires.capabilities 全部满足
          AND requires.entitlements 全部有效
          AND permissions.allOf 全部满足（或显式 anyOf 满足）
```

菜单分组下若所有子页面不可见则隐藏。只有读权限时进入只读态；直接 URL 无权限显示 403，有能力/许可缺失显示“不支持/未授权”。这些状态均不替代后端检查。

### 5.5 数据结构示例

```yaml
user:
  id: u_1024
  username: alice
  roleAssignments:
    - roleId: operator
      scope: { type: gateway, id: self }
    - roleId: engineer
      scope: { type: channel, ids: [rs485-1] }
  directGrants:
    - permission: system.log:export
      scope: { type: gateway, id: self }
      expiresAt: 2026-11-02T08:30:00Z
      grantedBy: u_admin
      reason: 现场故障诊断
  mustChangePassword: false
  status: active
  lastLogin: 2026-11-01T08:30:00Z

role:
  id: operator
  builtin: true
  permissions:
    - core.dashboard:read
    - core.telemetry:read
    - core.alarm:read
    - core.alarm:write
    - system.service:read
  defaultScope: { type: gateway, id: self }
```

`GET /api/v1/auth/me` 返回用户身份、角色分配、展开后的权限 ID、资源范围摘要、必须改密状态及授权策略版本。前端使用这些字段生成菜单/按钮状态；服务端不接受前端回传的权限集合作为授权依据。

### 5.6 认证、会话与浏览器安全

| 项 | 设计基线 |
| --- | --- |
| 密码存储 | Argon2id；参数按目标 CPU/RAM 基准选择并记录版本；资源极紧时改用 bcrypt 须经 ADR，不能仅固定 cost 不压测 |
| 初始账号 | 不使用通用默认密码；首次登录强制设置强密码，或使用设备唯一初始口令；口令交付与重置流程由产品安全设计定义 |
| 密码策略 | 最小长度建议 ≥ 12；禁止与用户名相同和已知弱口令；复杂度/过期策略按客户要求评估，避免无必要的周期强制更换 |
| 登录保护 | 用户名/密码失败统一响应，避免用户枚举；按账号+来源组合进行渐进退避/限速，避免仅按账号永久锁定导致远程拒绝服务；锁定/退避事件审计 |
| 会话 | 默认使用服务端可撤销 Session；Cookie 使用 `HttpOnly`、`Secure`、`SameSite=Strict`（需要跨站身份源时另行评审）；登录及权限提升后轮换 Session ID；空闲超时默认 15 分钟可配置；另设绝对生命周期上限，具体时长由安全 ADR 冻结，不允许无限期 Session |
| 会话撤销 | 登出、管理员强制下线、密码修改、角色/权限变化、安全事件后支持即时撤销；服务端每次请求检查会话状态/授权版本；WS 订阅同步撤销，后台任务按阶段策略重新检查授权 |
| JWT | 非默认方案。若采用，必须补齐短期访问令牌、刷新令牌轮换、重放检测、撤销/黑名单、密钥轮换和时钟偏差策略的 ADR |
| CSRF | Cookie 会话下，对状态变更 API 校验 CSRF token 和 Origin；使用自定义请求头；状态变更只允许明确的 HTTP 方法和内容类型 |
| Host / Origin / CORS | Host allowlist；状态变更请求和 WebSocket 握手校验 Origin；CORS 默认关闭，仅对精确 allowlist 开放，禁止 `*` 与凭据组合 |
| 代理头 | 仅信任配置中的反向代理；丢弃客户端自带 `Forwarded` / `X-Forwarded-*`，避免来源 IP 与协议被伪造 |
| 并发会话 | 可限制同账号并发数；管理员可强制下线；新会话策略必须提示用户并审计 |
| 多因素 | TOTP 可作为增强项；外部 LDAP / RADIUS 仅在确认需求后纳入，外部身份失联时的本地恢复规则需明确 |
| API Token | 使用随机高熵令牌，只保存哈希；每个 Token 绑定主体、scope、资源范围、到期时间和创建者；可单独吊销；禁止在 URL、日志或诊断包中暴露；必须有绝对到期时间和轮换/撤销策略 |
| 审计 | 记录登录、登出、认证失败、角色/权限变化、Token 生命周期、高风险操作和结果；审计只允许按保留策略自动轮转，普通用户不可删除或篡改 |

**本机管理界面网络防护**：默认只允许管理网接口/配置网段访问；Host 必须 allowlist；HTTP 只允许跳转到 HTTPS；管理面是否可经 WAN/蜂窝接口访问必须显式配置并审计。设备服务不得因“处于局域网”而默认信任浏览器来源。

## 6. 前端详细设计

### 6.1 App Shell 启动流程

```text
加载 index.html（含首屏骨架）
  → GET /api/v1/system/info（最小产品展示信息；匿名 allowlist）
  → 应用产品主题与 Logo
  → 未登录：渲染登录页
  → 登录成功：服务端建立 Session 并轮换 Session ID
  → GET /api/v1/auth/me
  → GET /api/v1/system/capabilities（需登录 + system.capability:read）
  → 计算可见路由/菜单（仅用于 UI 呈现）
  → 进入目标页面或 Dashboard
  → 建立 WebSocket 并按当前页面所需主题订阅
```

所有路径统一包含 `/api/v1`。Capability 请求失败或能力版本未知时，不允许前端推断“默认支持”；写操作始终由服务端再次判定。

### 6.2 路由与菜单注册表

- Core 模块与 Pack 页面在构建期通过 Manifest 生成静态 Registry；Shell 启动时仅按能力、Entitlement 和用户权限过滤，生成 vue-router 路由和侧边栏树。
- 运行时不执行 extension.yaml 注册，不允许业务服务向 webd 临时注册路由或权限。设备启动仅校验固件内静态注册表的摘要、版本和产品 ID。
- 菜单分组固定为：概览 / 采集与数据 / 控制与策略 / 网络与通信 / 系统 / 诊断。Pack 通过 `menu.group` 挂入，不得随意创建一级分组。
- 路由守卫用于体验：未登录转登录页；无权限显示 403；能力缺失显示不支持；强制改密只允许进入改密页；未保存修改离开时提示。所有 API 仍必须服务端鉴权。

### 6.3 数据、实时事件与状态

| 类型 | 方案 | 说明 |
| --- | --- | --- |
| 会话 / 权限 / 能力 | Pinia（auth、capability store） | 登录后加载；权限或 capabilityRevision 改变时刷新；本地缓存不授权 |
| 服务端数据 | 请求层 + 缓存 | 列表分页走服务端；权限变更/会话撤销后清除敏感缓存 |
| 实时数据 | WebSocket 只读订阅 | 页面进入订阅、离开退订；连接数、主题数、消息大小和速率受限 |
| 实时事件协议 | `eventId`、`streamId`、`partitionId`、`sequence`、`bootId`、`eventType`、`resourceId`、`occurredAt`、`schemaVersion` | 游标以持久化的 `(streamId, partitionId, sequence)` 为基础；`bootId` 仅标识事件发生时的设备启动周期；`eventId` 全局唯一 |
| 事件连续性 | 告警、任务、配置事务等关键事件在保留窗口内支持游标补发；遥测允许合并/丢弃旧值 | 游标失效、保留窗口过期或序列缺口时服务端发 `resync_required`，前端通过 REST 快照重拉；必须暴露 gap/丢弃计数 |
| 消息语义 | 关键事件至少一次投递（可能重复）；遥测采用最新值/采样合并 | 前端按 eventId 去重；不能宣称 exactly-once；串口调试帧必须保留帧边界和顺序 |
| 订阅失效 | Session 撤销、角色/授权策略版本变化、资源范围变化或 Entitlement 失效 | 服务端主动撤销受影响订阅/关闭连接；事件分发前重新校验订阅身份、权限与资源范围；客户端不得依赖旧订阅继续收数 |
| 表单草稿 | 本页局部状态 + 脏检查 | 保存前校验；配置事务草稿存设备端；离开前提示 |
| 偏好设置 | 设备端用户配置优先，其次 localStorage | 偏好不包含权限；切换账号时隔离和清理 |

WebSocket 断线后指数退避重连，携带最后确认的**不透明游标**。游标必须绑定 `streamId`、`partitionId`、序号及订阅过滤/授权范围摘要，客户端不得自行拼接或修改。`sequence` 由持久化事件源按分区单调分配，服务/设备重启后继续递增；`bootId` 仅作为诊断字段，不作为跨重启恢复游标的主键。若存储重建导致 `streamId` 改变、保留窗口已过、schema 不兼容或序列不连续，服务端返回 `resync_required`，前端重新读取权威 REST 快照。任务状态以持久化任务 API 为准，WebSocket 只是通知通道。

**事件持久化与发布顺序**：告警、任务、配置事务和安全审计等关键事件必须先形成持久化事实，再对 WebSocket 发布。若业务状态与事件记录由同一服务/数据库管理，优先在同一事务内写入状态和事件/outbox；若跨服务或跨存储，各权威业务服务必须维护自己的事务内 outbox 或等价持久事件日志，再异步发布。不得宣称 webd 能跨多个独立数据库提供全局原子提交。禁止先推送成功事件、后异步写入权威状态。发布器崩溃后允许重复投递，消费者按 `eventId` 幂等去重；无法证明某个副作用/事件是否完成时，进入可观察的恢复/重同步状态，不伪称 exactly-once。

**订阅生命周期**：Session 登出/过期、账号禁用、角色或直接授权变化、资源 scope 变化以及许可失效时，服务端必须使相关订阅失效；不得仅依赖浏览器重新连接或前端自行刷新。

### 6.4 配置页统一交互：草稿 → 应用 → 确认 / 回滚

配置事务由设备端**独立于 webd 请求生命周期**的事务协调器/watchdog 管理；浏览器计时器只显示倒计时，不承担回滚责任。协调器的具体承载方式（独立守护进程或系统 supervisor/定时器）由 ADR 冻结，但必须能在 webd 崩溃后继续执行。

**风险分类**：

| 风险类别 | 示例 | 处理要求 |
| --- | --- | --- |
| R0 普通配置 | 仪表列偏好、非关键展示设置 | 校验后直接保存，可版本回退 |
| R1 服务中断 | 重启采集服务、切换普通通信参数 | 维护窗口/状态提示；失败恢复或补偿动作 |
| R2 管理通道失联 | IP、网关、防火墙、HTTPS 端口、管理路由 | 必须确认窗口 + 自动回滚 + 带外恢复路径 |
| R3 安全敏感 | 证书、密钥、身份源、访问控制策略 | 额外校验、审计；按风险需要双人审批；回滚策略预先验证 |
| R4 业务控制/物理影响 | 费控、负荷策略、继电器/开关量、主站控制参数 | 单独审批/计划执行/维护窗口；回滚或补偿逻辑必须由业务确认 |
| R5 不可逆操作 | 恢复出厂、不可逆密钥销毁、不可回退的数据格式转换 | 不进入普通配置事务；双重确认、备份/恢复条件及不可逆提示 |

**事务状态机**：

```text
DRAFT
  → VALIDATING
  → APPLYING
  → PENDING_CONFIRM（仅需确认的风险类别）
      ├─ confirm → CONFIRMED
      └─ deadline / crash recovery → ROLLING_BACK → ROLLED_BACK
                                          └──────→ ROLLBACK_FAILED
VALIDATING/APPLYING 失败 → FAILED（保留错误与恢复动作）
```

**强制规则**：

1. 事务记录持久化 `transactionId`、操作者、创建/应用时间、配置域、`baseRevision`、目标 revision、变更差异摘要、应用前快照位置、deadline、bootId、状态、回滚结果和关联审计 ID。
2. apply 前先校验权限、能力、Schema、资源范围和基线版本；同一配置域通过锁/租约串行化。`baseRevision` 已变化时返回 409，不得静默覆盖。
3. 应用前持久化旧配置快照和事务日志，再进行变更；apply、confirm、rollback 必须幂等，重复请求返回同一事务结果。
4. R2 默认确认窗口可从 60 秒开始，但必须按设备响应时间和现场网络验证；窗口时长属于配置，不是硬编码的安全承诺。
5. 独立 watchdog 监控 deadline。webd 退出、请求断开或浏览器关闭不影响 watchdog；设备重启后，启动恢复流程在管理服务对外可写前检查未完成事务，默认对未确认事务执行回滚。
6. 计时使用单调时钟；持久化 `bootId`。跨重启不能信任旧进程的 monotonic 值，未确认事务按恢复策略处理；墙上时钟异常不得延长确认窗口。
7. confirm 必须是已认证用户通过 `transactionId` 对当前事务明确确认，重新检查权限、资源范围和会话；记录确认者、最终 revision 和健康检查结果。相同确认请求幂等。
8. 回滚失败必须进入 `ROLLBACK_FAILED`，显示持久化告警、保留恢复证据并限制危险写操作；必须提供经过设计的串口/物理按键/本地维护等带外恢复路径。不能将失败伪装为已回滚。
9. 跨多个配置域时明确原子边界；无法原子提交的使用分阶段事务和补偿动作，不得声称全局原子。
10. 所有草稿、差异、应用、确认、回滚和失败均进入审计；审计失败时的处理按风险等级执行。

### 6.5 Schema 驱动表单与文件导入导出

- 每个配置对象由版本化 JSON Schema 描述类型、范围、枚举、依赖、必填、单位、风险等级、权限映射和校验；OpenAPI 引用同一 Schema 版本。前后端生成/引用同一权威定义，后端校验为准。
- Schema 的 `risk` 分类与 §6.4 风险等级一致；后端根据被修改字段解析权限与配置事务策略，不能信任客户端自行声明 `risk`。
- YAML/JSON 导入只允许数据子集，禁用任意对象构造、标签扩展和代码执行；导入前做 Schema、版本、权限、资源范围和差异预览。
- 上传限制单文件大小、总展开大小、文件数、压缩比、处理时间和临时目录配额；拒绝路径穿越、符号链接逃逸、设备文件和非允许类型。
- CSV 明确 UTF-8/BOM、分隔符、小数点、单位和时区规则；导出时对公式触发字符做防注入处理。批量导入明确全有全无或部分成功语义，并返回逐行错误；重复提交必须具备幂等保护。
- 配置备份默认不包含明文私钥、密码和一次性 Token；需要包含敏感材料时必须显式授权、加密并记录审计。

### 6.6 组件分层

```text
L0 Design Token      颜色 / 间距 / 圆角 / 字体 / 阴影 / 动效（CSS 变量）
L1 基础组件           Button、Input、Select、Switch、Table、Tabs、Modal、Toast、Tag…
L2 业务通用组件       StatusBadge、MetricCard、ConfirmDanger、DiffViewer、FormSection、
                     FileUpload、LogViewer、TimeRangePicker、IpInput、PermissionGate…
L3 页面模板           ListDetailPage、WizardPage、DashboardPage、SettingsPage、LogPage
L4 产品组件           PowerFlowCard、MeterTable、PointMapEditor…（位于 Pack，只组合 L0~L3）
```

规则：Pack 不得直接引入未经批准的第三方 UI 库，只能使用 core-ui 暴露的组件/扩展点；确需新增组件时先判断是否应上升到 Core。

### 6.7 国际化与本地化

- 语言包按模块拆分、懒加载；Core 文案 key 形如 `core.network.title`，Pack 文案以产品 ID 开头，避免冲突。
- 默认中文 / 英文；时间在 UI 中明确设备时区或 UTC，API 时间戳使用带时区的 RFC 3339/ISO 8601 表达；数值单位按用户偏好格式化，但底层存储统一单位。
- 文案不拼接，使用 ICU 占位符；预留文本膨胀空间，CI 执行伪本地化测试。

### 6.8 性能预算与优化

| 指标 | 目标 / 约束 |
| --- | --- |
| 首屏 JS（gzip） | ≤ 300KB（Shell + 登录 + Dashboard，现阶段预算） |
| 单个页面 chunk | ≤ 100KB（gzip），图表/编辑器按需异步加载 |
| 静态资源 | 构建期 brotli + gzip 预压缩；设备不实时压缩 |
| 缓存 | hash 资源 immutable 长缓存；index.html 不缓存；权限/能力数据按版本校验 |
| 字体 / 图标 | 本地子集字体；SVG sprite 按需引入 |
| 大表格 | 虚拟滚动 + 服务端分页；单次渲染 ≤ 200 行 |
| API 分页 | 默认 pageSize 50，服务端最大值建议 200；具体数值根据设备内存和查询压测冻结 |
| 时序查询 | 服务端限制最大跨度、采样点数、超时和降采样；客户端不得要求无限数据 |
| WebSocket | 连接数、每连接主题数、消息尺寸、消息速率、排队字节数和慢客户端策略均设上限 |
| 低配设备 | 动画/图表刷新率降级；不以 `hardwareConcurrency` 作为安全或资源预算事实 |
| CI 卡口 | 体积超预算失败；性能预算必须在最低配置真机测量并归档 |

## 7. 后端与 API 设计

### 7.1 API 统一约定

1. **路径**：所有 HTTP API 使用 `/api/v1/<domain>/<resource>`；路径、operationId 和 API 版本由 OpenAPI 定义。文档、前端、Mock、代理和测试不得省略 `/api/v1`。
2. **认证/授权元数据**：每个 OpenAPI operation 必须声明 `security`、`x-required-permissions`、`x-resource-scope`、`x-required-capabilities`（如有）、`x-required-entitlements`（如有）、幂等性和审计等级。匿名接口只能存在于独立 allowlist。
3. **响应**：成功响应遵循 operation 的 schema；错误统一为 `{ code, message, details, requestId }`。`code` 为稳定业务错误码，`details` 可给字段级错误，不包含堆栈、密钥或内部路径。
4. **分页**：列表默认 pageSize 50，服务端最大值建议 200；只允许 allowlist 字段排序/过滤。持续写入的大表优先 cursor/keyset pagination；返回 `nextCursor`，避免 offset 翻页重复/漏项。超限返回明确错误。
5. **查询上限**：时间序列查询必须限制时间跨度、最大采样点数、过滤条件数和执行时间；导出走异步任务并限制并发、文件尺寸、保留时间和下载授权。
6. **并发控制**：配置/资源更新使用 `ETag` / `If-Match` 或显式 revision；基线冲突返回 409。客户端不得用无条件 PUT 覆盖未知版本。
7. **幂等**：升级、恢复出厂、备份创建/恢复、批量导入、配置 apply 等可能产生副作用的创建型操作使用 `Idempotency-Key`；相同主体、相同 key 和相同请求摘要返回同一结果，不重复执行。活动任务的幂等记录不得因普通 TTL 清理而提前过期。
8. **长任务**：升级、备份、日志打包、点表导入等返回 `202 + taskId`；任务输入摘要、创建者、授权信息、阶段、结果和错误持久化。执行前/关键阶段重验当前权限、能力、Entitlement 和互斥条件。
9. **限流与资源保护**：认证、重启、升级、恢复出厂、文件上传、导出和调试接口单独限流；按来源/主体/设备资源设置队列与并发上限。不可只依赖 Nginx per-IP 限流。
10. **HTTP 方法**：读取用 GET；创建用 POST；替换/更新用 PUT/PATCH；删除用 DELETE；动作型操作通过明确 action 子路径表达；不得用 GET 触发状态变更。
11. **审计**：高风险 operation 声明审计等级和字段脱敏策略；记录操作者、资源、请求 ID、结果、配置 revision/任务 ID 和失败原因摘要。

### 7.2 核心 API 清单与授权映射

下表是接口设计基线；最终以 OpenAPI 的 operation 级定义为准。表中权限 ID 必须存在于附录 A 权限目录。`登录`表示有效 Session；匿名只允许系统信息最小 allowlist。

| 分类 | 方法与路径（完整路径） | 认证 | 所需权限 / 约束 |
| --- | --- | --- | --- |
| 认证 | `POST /api/v1/auth/login` | 匿名 allowlist | 登录限流、CSRF/Origin/Host 校验；错误不泄露用户是否存在 |
| 认证 | `POST /api/v1/auth/logout` | 登录 | 撤销当前 Session |
| 认证 | `GET /api/v1/auth/me` | 登录 | 返回当前主体、权限/范围摘要和授权版本 |
| 认证 | `POST /api/v1/auth/password` | 登录 | 当前密码/强认证校验；修改后撤销其他会话 |
| 系统信息 | `GET /api/v1/system/info` | 匿名 allowlist | 仅返回登录页必需字段；固定 allowlist |
| 系统能力 | `GET /api/v1/system/capabilities` | 登录 | `system.capability:read`；敏感许可详情按字段授权 |
| 用户 | `GET/POST /api/v1/users` | 登录 | GET 使用 `user.account:read`；POST 使用 `user.account:admin`；创建时防止自授权提权 |
| 用户 | `GET/PATCH/DELETE /api/v1/users/{id}` | 登录 | GET 使用 `user.account:read`；PATCH/DELETE 使用 `user.account:admin`；逐对象授权 |
| 角色 | `GET/POST /api/v1/roles` | 登录 | GET 使用 `user.role:read`；POST 使用 `user.role:admin`；不得删除最后管理员 |
| 角色 | `GET/PATCH/DELETE /api/v1/roles/{id}` | 登录 | GET 使用 `user.role:read`；PATCH/DELETE 使用 `user.role:admin`；检查授予权限上限 |
| 会话 | `GET/DELETE /api/v1/sessions/{id}` | 登录 | `user.session:admin`；只能管理授权范围内会话 |
| API Token | `GET/POST/DELETE /api/v1/tokens` | 登录 | `user.token:admin`；创建只显示一次明文，服务端仅存哈希 |
| 配置事务 | `POST /api/v1/config/transactions` | 登录 | 按具体配置域计算权限；创建草稿并返回 transactionId |
| 配置事务 | `GET /api/v1/config/transactions/{id}` | 登录 | 事务创建者或有对应配置读取权限的主体 |
| 配置事务 | `PUT /api/v1/config/transactions/{id}/sections/{sectionId}` | 登录 | sectionId 必须映射到构建期注册的精确权限；不允许运行时通配符；If-Match/revision |
| 配置事务 | `POST /api/v1/config/transactions/{id}/validate` | 登录 | 对事务涉及的配置域检查权限/Schema/能力 |
| 配置事务 | `POST /api/v1/config/transactions/{id}/apply` | 登录 | section 写权限；幂等；高风险启动 watchdog |
| 配置事务 | `POST /api/v1/config/transactions/{id}/confirm` | 登录 | 对应写权限；绑定事务 ID、当前用户和会话 |
| 配置事务 | `POST /api/v1/config/transactions/{id}/rollback` | 登录/系统恢复 | 对应写权限；watchdog 可通过受限内部身份执行 |
| 网络 | `GET/PATCH /api/v1/network/interfaces/{id}` | 登录 | GET 使用 `network.interface:read`；PATCH 使用 `network.interface:write`；写操作走 R2 事务 |
| 网络 | `GET/PATCH /api/v1/network/routes` | 登录 | GET 使用 `network.route:read`；PATCH 使用 `network.route:write`；写操作走 R2 事务 |
| 网络 | `GET/PATCH /api/v1/network/firewall` | 登录 | GET 使用 `network.firewall:read`；PATCH 使用 `network.firewall:write`；管理访问规则变更需回滚保护 |
| 网络 | `GET/PATCH /api/v1/network/cellular` | 登录 | GET 使用 `network.cellular:read`；PATCH 使用 `network.cellular:write`；按实际模块能力限制 |
| 系统时间 | `GET/PATCH /api/v1/system/time` | 登录 | GET 使用 `system.time:read`；PATCH 使用 `system.time:write`；记录校时事件 |
| 证书 | `GET/POST/DELETE /api/v1/system/certificates` | 登录 | GET 使用 `system.cert:read`；POST/DELETE 使用 `system.cert:write`；私钥不可回显，替换流程可恢复 |
| 系统 | `POST /api/v1/system/reboot` | 登录 | `system.reboot:execute`；幂等任务 |
| 系统 | `POST /api/v1/system/factory-reset` | 登录 | `system.factory-reset:execute`；双重确认/审计/本地恢复规则 |
| 备份 | `POST /api/v1/system/backups`、`GET /api/v1/system/backups`、`GET /api/v1/system/backups/{id}`、`GET /api/v1/system/backups/{id}/download` | 登录 | 创建使用 `system.backup:execute`；列表/元数据/下载使用 `system.backup:read`；逐对象检查主体资源范围、文件状态、到期时间和一次性/短时下载授权 |
| 备份恢复 | `POST /api/v1/system/backups/{id}/restore` | 登录 | `system.backup:execute`；强认证/二次确认、型号/版本/签名和完整性校验；使用 `Idempotency-Key`；返回 `202 + taskId`，任务状态统一通过 `/api/v1/tasks/{id}` 查询 |
| 升级 | `POST /api/v1/system/upgrade` | 登录 | `system.upgrade:execute`；使用 `Idempotency-Key`；返回 `202 + taskId`；签名/型号/版本/回退能力检查；任务状态统一通过 `/api/v1/tasks/{id}` 查询 |
| 服务 | `GET /api/v1/system/services` | 登录 | `system.service:read` |
| 服务 | `POST /api/v1/system/services/{id}/actions/{action}` | 登录 | `system.service:execute`；action allowlist，不得传 shell 命令 |
| 日志 | `GET /api/v1/logs/system`、`POST /api/v1/logs/exports` | 登录 | GET 使用 `system.log:read`；导出使用 `system.log:export`；异步且受限 |
| 审计 | `GET /api/v1/logs/audit`、`POST /api/v1/logs/audit/exports` | 登录 | GET 使用 `core.audit:read`；导出使用 `core.audit:export`；保留策略不允许用户删除 |
| 诊断 | `POST /api/v1/diagnostics/bundles` | 登录 | `system.diagnostics:execute`；脱敏与文件限制 |
| 实时通道 | `GET /api/v1/ws`（WebSocket 握手） | 登录 | 握手校验 Host/Origin/Session；只允许 `subscribe` / `unsubscribe` |
| 实时通道 | WS `subscribe(topic, cursor)` / `unsubscribe(topic)` | 登录 | 每个 topic 绑定具体权限和资源范围；无 `publish` 操作 |
| 任务 | `GET /api/v1/tasks/{id}`、`GET /api/v1/tasks` | 登录 | `core.task:read` + 任务资源范围；默认只返回主体可见任务；升级/备份恢复等异步操作均以此为唯一任务状态查询 API |
| 任务 | `POST /api/v1/tasks/{id}/cancel` | 登录 | OpenAPI 必须声明 `x-authorization-policy: task-cancel-v1`；按 §7.5 的 owner 自取消或管理员跨用户取消策略决策，均须校验当前主体、scope、任务类型授权/能力/Entitlement 和可取消阶段；客户端不得选择授权分支 |
| 任务产物 | `GET /api/v1/tasks/{id}/artifacts/{artifactId}` | 登录 | `core.task:read` + 任务/产物资源范围；同时校验源任务类型的下载权限、产物归属、过期时间和一次性下载策略 |
| 产品 API | `/api/v1/iems/...` | 登录 | `iems.*` 具体权限 + 能力/Entitlement + 资源范围 |
| 产品 API | `/api/v1/dcu/frames` | 登录 | `dcu.frame:read` + 通道资源范围；具体协议报文接口由 Pack OpenAPI 定义 |
| 产品 API | `/api/v1/dcu/...` | 登录 | `dcu.*` 具体权限 + 能力 + 资源范围 |

**配置 API 权限解析**：通用配置事务端点不能只绑定一个宽泛的 `config:write`。服务端根据 sectionId 和 Schema 的可信映射，逐字段解析所需权限、风险等级、能力条件和事务策略；未知 section、未知字段或无法解析的权限映射必须拒绝。

### 7.3 webd 的职责边界与 IPC 安全

**webd 负责**：外部 API 路由、TLS/代理信任后的协议处理、Session、CSRF/Origin/Host 校验、授权策略决策、Capability/Entitlement 查询、审计、限流、Schema/参数校验、配置事务入口、任务 API、WebSocket 订阅和事件转发。

**webd 不负责**：采集、规则、协议栈、控制算法等业务逻辑。此类逻辑留在业务服务中；webd 通过受控 IPC 调用并传递可信授权上下文。

**业务服务约束**：

1. 只暴露受限 IPC，不提供外部 HTTP 入口；Socket owner/group/mode、peer credential 或总线 ACL 必须可测试。
2. IPC 方法定义调用方白名单、输入/输出 schema、消息尺寸上限、超时、取消语义、并发上限和错误码；拒绝未知方法和多余字段。
3. webd 传入的 actor、permissions、resource scope、requestId 和 policy revision 由受信任 IPC 通道承载；业务服务校验调用方确为 webd，并对业务对象执行必要的归属/状态检查。
4. 对可由其他本机服务直接调用的业务方法，单独定义调用方和操作 ACL；不能把 webd 的外部授权视为所有本机调用自动安全。
5. WebSocket 只承载被授权的只读事件；任何修改状态、下发控制命令、启动任务的动作必须经有 operationId、权限、幂等键和审计定义的 API。

### 7.4 OpenAPI、Manifest 与静态注册表

- OpenAPI 是 HTTP 路径、方法、请求/响应 schema、认证和 operation 安全元数据的唯一事实来源。
- 权限目录是权限 ID、动作语义、依赖关系和敏感等级的唯一事实来源。
- JSON Schema 是配置/导入数据结构及约束的唯一事实来源。
- `product.yaml` / `extension.yaml` 是构建输入，声明产品页面、Widget、能力/授权要求和 API extension 文件引用；它们不是运行时动态注册 API 的命令。
- 构建时生成 TS 客户端、Mock、路由/权限注册表、Manifest 合并结果和 API×权限测试；生成产物不得人工维护。
- CI 必须验证所有 operation 都有认证定义、权限映射或显式匿名 allowlist，引用的权限和能力均已声明，产品路径前缀匹配 Pack ID，registry 摘要与固件产物一致。

### 7.5 长任务执行模型

长任务必须与 HTTP 请求和 webd worker 生命周期分离，由设备端持久化任务执行器管理。执行器可以是 webd 内的受监督独立 worker，也可以是独立系统服务；无论采用哪种实现，任务状态和副作用进度必须持久化，webd 重启后可恢复查询。

**任务记录至少包含**：`taskId`、`taskType`、`idempotencyKey`、请求摘要、创建者、创建时授权/策略版本、目标资源范围、当前阶段、进度、创建/更新时间、结果摘要、错误码、取消能力、重启恢复策略和关联审计 ID。不得持久化明文密码、Token 或私钥。

**状态机建议**：

```text
PENDING → QUEUED → RUNNING → SUCCEEDED
                    ├──────→ FAILED
                    ├──────→ CANCELING → CANCELED
                    ├──────→ INTERRUPTED → RECOVERING → RUNNING / FAILED
                    └──────→ RECOVERY_REQUIRED
```

**执行规则**：

1. 任务创建时校验身份、权限、资源范围、能力、Entitlement、参数和幂等键；进入执行队列前再次检查。到达不可逆/高风险阶段前重新检查授权和设备状态。
2. 权限撤销后，未开始的任务不得继续执行；执行中的任务按任务类型定义安全停止/暂停/完成当前原子阶段策略。不可中断的阶段不得伪装成“已取消”，必须向用户说明任务仍在执行或进入恢复状态。
3. 定义互斥组。例如固件升级期间禁止备份恢复、恢复出厂、其他固件写入和冲突的分区操作；网络配置事务与设备重启/升级也需明确互斥。互斥关系由任务类型注册表声明并由执行器强制检查。
4. `Idempotency-Key` 与主体、任务类型和请求摘要绑定。相同键/相同摘要返回原 taskId；相同键/不同摘要返回冲突，不创建第二个副作用。
5. 设备重启后从持久化阶段恢复；若无法安全恢复，进入 `RECOVERY_REQUIRED`，禁止重复执行危险步骤并提供诊断信息。
6. 任务取消是请求而非保证。只有执行器声明当前阶段可安全取消时才进入 CANCELING；否则返回不可取消原因。

**任务取消策略（`task-cancel-v1`）**：

- **Owner 自取消分支**：仅当任务策略显式允许 `ownerCancel`、当前主体等于任务创建者、Session 有效、操作者当前仍满足原任务类型所需的 execute 权限/能力/Entitlement、对任务目标仍具备资源 scope，且任务阶段标记为可安全取消时允许。此路径不授予跨用户取消能力，也不要求把 `core.task:cancel` 泛化给普通用户。
- **管理员取消分支**：跨用户或非创建者取消必须同时具备 `core.task:cancel` 与目标任务 scope；仍须满足任务类型定义的安全取消阶段、二次确认/审计条件。管理员权限不允许强行中断不可安全中断的步骤。
- 未满足任一分支时拒绝并审计；只有执行器确认安全停止后才能进入 `CANCELED`。API/策略引擎统一实现该策略，不能由各页面或客户端分别判断。

7. 任务状态 API 以持久化记录为准；WebSocket 只推送通知。任务历史设定保留期限和数量上限，清理不得影响审计记录。
8. 任务互斥、授权重验、重启恢复、幂等和取消边界均纳入故障注入与真机测试。
9. 任务执行语义按“至少一次恢复尝试”设计，不承诺跨进程/设备重启的 exactly-once。每个可能产生外部副作用的步骤必须有稳定 `stepId`、执行前后持久化记录和可查询回执；步骤本身应幂等，或通过结果对账确认是否已完成。无法确认副作用结果时进入 `RECOVERY_REQUIRED`，禁止盲目重放。
10. 同一任务在任一时刻只能有一个有效执行者。任务领取必须通过持久化原子 claim/lease 完成，并产生单调递增的 fencing token；执行器每次提交阶段结果时校验当前 token，旧 worker 即使恢复运行也不能提交状态或发起新的受控副作用。
11. IPC/驱动调用超时不等同于副作用失败。下游操作应接收稳定 operation key 并提供结果查询/回执；无法查询或证明是否完成时进入 `RECOVERY_REQUIRED`，由受控对账流程处理。幂等键记录在任务活动期间不得过期；保留期限至少覆盖任务状态和副作用回执的保留期限。

## 8. 信息架构与页面设计

### 8.1 全局信息架构（Core 固定骨架）

导航分组固定为六组。表中权限均为规范权限 ID；页面数量由实际 Pack、能力和授权动态计算，表中“默认角色”仅表示预期，不是授权配置。

| 分组 | Core 通用页面 | 产品扩展页面（示例） | 最低权限示例 | 默认可见角色 |
| --- | --- | --- | --- | --- |
| 概览 | Dashboard、设备状态 | iEMS：能流图、收益概览；DCU：采集成功率、在线表计数 | `core.dashboard:read` | 全部具备权限者 |
| 采集与数据 | 实时数据、历史曲线 | iEMS：数据点表、设备档案；DCU：电表档案、抄表任务、冻结数据、事件记录 | `core.telemetry:read`、`iems.datapoint:read`、`dcu.meter:read` | Viewer 起 |
| 控制与策略 | 告警规则、定时任务 | iEMS：削峰填谷、需量控制；DCU：负荷控制、费控参数 | `core.alarm:read`、`iems.strategy:read`、`dcu.task:read` | Operator 起；写权限单独授予 |
| 网络与通信 | 以太网、蜂窝、Wi-Fi、路由、防火墙、DNS/NTP | iEMS：南向协议与北向云连接；DCU：RS485/PLC/RF 通道、主站通信 | `network.interface:read`、`network.route:read`、`iems.cloud:read`、`dcu.channel:read` 等产品权限 | 按授权开放，Engineer 常用 |
| 系统 | 用户与角色、证书、时间、备份恢复、固件升级、关于 | 产品授权（License） | `user.account:read`、`user.role:read`、`system.time:read`、`system.time:write`、`system.cert:read`、`system.cert:write`、`system.backup:read`、`system.backup:execute`、`system.upgrade:execute` 等具体权限 | Admin |
| 诊断 | 系统日志、审计日志、网络诊断、抓包 | 协议报文监视、采集诊断 | `system.log:read`、`core.audit:read`、`debug.frame:execute` | Operator 起；抓包需专门授权 |

**禁止简写**：本表及其他正文不得使用 `dashboard:read`、`*.data:read`、`system.*` 等未经目录解析的简写。展示文案可以使用中文分组名，鉴权必须使用附录 A 的精确 ID。

### 8.2 不同角色看到的页面数量（以 iEMS 为例，示意）

| 角色 | 概览 | 采集与数据 | 控制与策略 | 网络与通信 | 系统 | 诊断 | 合计 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Viewer | 2 | 3 | 1（只读） | 0 | 0 | 1 | 7 |
| Operator | 2 | 3 | 2 | 1（只读） | 0 | 2 | 10 |
| Engineer | 2 | 4 | 3 | 6 | 2 | 3 | 20 |
| Admin | 2 | 4 | 3 | 6 | 7 | 3 | 25 |

数量由实际构建清单、Capability、Entitlement、用户权限和资源范围共同决定；上表仅用于产品规划对齐，不能作为鉴权策略或固定验收数。

### 8.3 页面模板（统一版式）

| 模板 | 适用场景 | 结构 |
| --- | --- | --- |
| Dashboard 仪表盘 | 概览 | 卡片网格（12 列），卡片可显隐、拖拽排序，卡片由 Core 与 Pack 的 widgets 共同提供 |
| 列表-详情 | 用户、电表档案、数据点、告警 | 顶部筛选 + 工具栏，表格，行点击打开右侧抽屉查看 / 编辑，批量操作 |
| 设置页 | 网络、时间、云连接 | 左侧分区锚点 + 右侧表单卡片，底部固定操作条（保存草稿 / 应用 / 重置） |
| 向导 | 首次引导、固件升级、设备配对 | 步骤条，每步一个目标，可回退，结束页给出结果摘要 |
| 日志页 | 系统、审计、协议报文 | 时间范围 + 级别 + 关键字过滤，实时尾随开关，导出 |
| 监视页 | 协议报文、实时数据 | 暂停 / 继续、高亮、限制缓存条数 |

### 8.4 关键页面线框

**整体布局**

&#91;embedded content: 整体布局线框 · 顶栏、侧栏、内容区、操作条\]

左侧菜单由权限与能力自动生成；顶栏的待应用徽标与底部操作条是所有配置类页面共用的骨架。

**Dashboard（iEMS 示例）**

&#91;embedded content: Dashboard 线框 · 7 张卡片，2 张来自 Pack\]

蓝色卡片由 Product Pack 的 widgets 提供，灰色卡片来自 Core；用户可显隐和排序，卡片是否出现仍受权限和设备能力约束。

**设置页的待应用提示**

```
+--------------------------------------------------------------+
| 顶部横幅：你有 3 项未应用的修改   [查看差异]  [应用]  [丢弃] |
+--------------------------------------------------------------+
| 应用后：高风险配置出现 60s 倒计时「确认保留」，否则自动回滚  |
+--------------------------------------------------------------+
```

### 8.5 登录页与首次引导

- 登录页只展示产品显示名、有限型号信息、固件展示版本和语言切换；不得暴露用户是否存在、内部组件版本、网络拓扑、序列号或许可证秘密。
- 首次登录向导：①设置管理员密码 → ②设备名称与时区 → ③网络基本配置 → ④（可选）云平台连接 → ⑤完成摘要。可能使管理通道失联的网络步骤进入 §6.4 配置事务。
- 登录失败使用统一提示；账户退避不应通过响应内容泄露用户名状态。登录接口还须防登录 CSRF：校验 Host/Origin、限制内容类型，并拒绝不符合本站交互约束的跨站请求。账号恢复需有受控本地流程，不设置隐藏后门账号。

### 8.6 关键交互规范

| 场景 | 规范 |
| --- | --- |
| 危险操作（重启、恢复出厂、删除用户） | 二次确认并说明影响；恢复出厂需强认证/设备名确认；高风险操作显示不可逆后果和审计提示 |
| 保存 / 应用反馈 | 防重复提交；写操作使用幂等键；失败保留输入并展示字段错误/冲突 revision |
| 空状态 | 说明原因 + 给出下一步操作 |
| 无权限 | 菜单不显示；已知 URL 显示 403；只读页显示只读徽标；服务端仍检查 |
| 设备不支持/许可无效 | 明确区分硬件/软件不支持、许可证未授权和运行故障，不显示笼统错误 |
| 离线 / 断连 | 常驻横幅提示；断连期间禁用写操作；恢复后按 REST 快照和事件游标重新同步 |
| 长任务（升级、导出） | 任务面板显示持久化状态；刷新/重连后通过 taskId 恢复；可取消性由任务阶段定义 |
| 配置事务 | 展示草稿差异、风险等级、事务 ID、确认倒计时和服务端状态；倒计时不得仅依赖浏览器 |
| 表单校验 | 失焦 + 提交校验；后端校验为权威；错误说明修正方法 |
| 表格 | 服务端分页/排序 allowlist；支持列显隐和导出；导出异步、受授权和大小限制 |

### 8.7 产品页面示例

**iEMS（能源管理）**：数据点表（支持批量导入导出 CSV、公式点、单位与倍率）、设备档案、能流图、策略编排（削峰填谷、需量控制，含时间轴编辑器）、南向协议配置（Modbus、IEC104 通道）、北向云连接（MQTT / HTTP，证书与主题模板）、告警规则。

**DCU（数据集中器）**：电表档案（批量导入、搜索、分组）、抄表任务方案、实时 / 冻结 / 事件数据查询、通道配置（RS485、PLC、RF）、主站通信参数、负荷与费控参数、采集成功率统计、协议报文监视。

两者共享 Core 的网络、用户、升级、日志、审计、备份、证书等页面，仅在 Product Pack 内编写差异化的页面和卡片。

### 8.8 点位与串口调试页（现场调试）

现场调试分三层，层级越低风险越高。串口是独占资源，浏览器不直接接触串口；所有收发经设备端调试服务，由 webd 统一认证/授权/审计，调试服务再执行通道级租约和底层资源检查。

| 层级 | 页面 | 作用 | 与采集服务的关系 | 权限点 | 默认角色 |
| --- | --- | --- | --- | --- | --- |
| 点位级 | 点位读写调试 | 选通道、设备、点位，读取或写入，显示解码值与单位 | 走采集服务已有通道，不抢占串口 | `debug.point:read` / `debug.point:write` | Engineer 起 |
| 报文级 | 报文收发 | 手工发一帧并查看原始应答 | 申请通道租约，必要时暂停该通道轮询 | `debug.frame:execute` | Engineer 起 |
| 透传终端 | 串口终端 | 直接收发字节流 | 独占串口，该口采集停止 | `debug.serial:admin` | Admin / 临时 Service |

Modbus TCP、IEC 104 等网口协议只涉及点位级与报文级，是否租用网口连接由对应服务能力和资源模型决定。

**通道租约流程**

1. 用户选通道并请求调试会话；webd 检查权限、资源范围、设备能力和风险条件。
2. 调试服务校验调用方 IPC 身份，向采集服务申请通道租约；采集服务暂停该通道轮询并返回租约 ID/期限。
3. UI 显示常驻横幅、操作者、通道、剩余时间和续期状态；续期须重新校验当前权限。
4. 租约到期、服务端会话撤销、显式结束或连接中断达到策略阈值时，服务端释放租约并恢复采集。不能只依赖浏览器关闭事件；采集服务重启也必须清理过期租约。
5. 同一通道同一时刻只允许一个有效租约；租约存储/心跳及采集服务崩溃后的恢复由调试服务和采集服务共同实现。

**安全与可用性规则**

- 只能选择已登记串口；波特率/数据位/校验/停止位按白名单校验；不暴露底层 GPIO/RS485 DE 控制。
- 会话数、期限、缓存条数、帧尺寸、带宽、请求频率和单用户并发均有上限；慢客户端不能造成无界内存队列。
- 写寄存器、写线圈、遥控/控制类帧发送前二次确认；控制类操作使用 `execute` 或 `write` 的精确权限并二次检查资源范围；审计记录发送摘要和执行结果，不记录密钥/敏感载荷。
- 报文视图保留帧边界、顺序、校验结果和时间信息；支持暂停、过滤和限量导出。遥测可合并，调试帧不得合并丢帧。
- 采集服务重启、租约失效或会话撤销时，服务端停止发送并释放租约；UI 明确显示通道“调试中/采集中/状态未知”。
- 表中权限 ID 必须纳入附录 A 和对应预置角色授权测试。

## 9. 视觉与外观设计

### 9.1 设计风格定位

- 关键词：**专业、克制、清晰、可信赖**。工业现场看重信息密度与状态可读性，而非炫目效果。
- 参考风格：现代企业后台（Linear / Vercel / Grafana 的简洁排版）+ 工业设备的状态语义（明确的运行 / 告警 / 离线配色）。
- 布局：左侧固定导航（可折叠为图标栏）+ 顶部状态栏 + 内容区；内容区最大宽度 1600px，大屏居中。
- 同时提供**浅色 / 深色**主题，默认跟随系统，用户可手动切换；机房或夜间巡检场景深色更友好。

### 9.2 Design Token

全部视觉属性以 CSS 变量定义，分三层：**基础色板 → 语义 Token → 组件 Token**，产品换肤只覆盖白名单品牌 Token。

**颜色（语义）**

| Token | 浅色 | 深色 | 用途 |
| --- | --- | --- | --- |
| `--color-primary` | #0B6BCB | #4C9AFF | 主操作、选中态、链接（产品可覆盖，须重新计算衍生色） |
| `--color-success` | #1A7F4B | #3FBF7F | 成功/正常状态前景 |
| `--color-warning` | #8A4B00 | #F2A93B | 警告文字/图标前景；浅色值针对白底正文达到至少 4.5:1 |
| `--color-warning-bg` | #FFF4E5 | #3A2B18 | 警告底色；需与其前景 Token 成对检查 |
| `--color-danger` | #C62828 | #FF6B6B | 错误、告警、危险操作前景 |
| `--color-info` | #0F7C8C | #4FC3D6 | 提示、中性事件前景 |
| `--color-offline` | #6B7280 | #9AA4B2 | 离线、未启用、禁用前景 |
| `--bg-page` | #F5F7FA | #0F141A | 页面背景 |
| `--bg-surface` | #FFFFFF | #171D25 | 卡片、表格、抽屉 |
| `--border-default` | #D8DEE6 | #2A3340 | 分隔线、输入框边框 |
| `--text-primary` | #1B2430 | #E8EDF3 | 正文 |
| `--text-secondary` | #5B6677 | #A3AEBD | 辅助说明 |

- 对比度目标：普通文字 ≥ 4.5:1，大字 ≥ 3:1；图标/边界是否适用 3:1 按组件功能和 WCAG 对照矩阵判断。
- 语义色必须按**前景/背景组合**验证，而非单独检查色值。每个主题和组件状态至少覆盖文字、图标、边框、底色、hover、selected、disabled、focus、warning、danger。
- `#B26A00` 在白色背景上的对比度约 4.24:1，不能直接作为白底普通字号警告文字。v1.2 将警告前景改为 `#8A4B00`；实际组件若采用其他背景，必须重新计算成对对比度。
- 状态不单靠颜色表达，同时使用图标、文字或形状；图表序列使用线型/点型辅助区分。
- 产品主色可以覆盖，但生成色阶后须对所有使用组合执行 CI 对比度验证；不能只检查主色与白色。

**字体**

- 中文：系统字体栈优先；西文/数字内置小型拉丁子集；报文和日志使用等宽字体。
- 数值使用等宽数字特性（tabular-nums），字号体系沿用 12 / 13 / 14 / 16 / 20 / 24；标题字重 600。

**间距、圆角、阴影、动效**

| 项 | 规范 |
| --- | --- |
| 间距 | 4px 基准栅格：4 / 8 / 12 / 16 / 24 / 32 / 48 |
| 圆角 | 小 4px、中 8px、大 12px |
| 阴影 | 三级；深色主题优先用边框表达层级 |
| 动效 | 100–200ms；尊重减少动态效果设置 |
| 图标 | 线性图标、SVG 按需引入 |
| 触控 | 桌面可点击区域 ≥ 36px；平板/手机 ≥ 44px |

### 9.3 关键组件视觉规范

| 组件 | 规范要点 |
| --- | --- |
| 状态徽标 StatusBadge | 圆点 + 文字，运行 / 告警 / 离线 / 未配置四种状态；告警含数量角标 |
| 指标卡 MetricCard | 标题（次要色）、主数值（24px 粗体）、单位、同比趋势小图；数据过期时置灰并标注更新时间 |
| 表格 | 行高 40px（紧凑 32px），斑马纹可选，悬停高亮；操作列右固定；数值右对齐 |
| 表单 | 标签在上（窄屏）或左侧（宽屏），必填标红星，说明文字在输入框下方，错误文字用危险色并带图标 |
| 按钮 | 主按钮（实心主色）、次按钮（描边）、文本按钮、危险按钮（红色）；每个区域只出现一个主按钮 |
| 标签页 / 分段控件 | 用于同一对象的不同视图；不超过 6 个，超出折叠为「更多」 |
| 弹窗 / 抽屉 | 简单确认用弹窗；表单编辑和详情用右侧抽屉，保留列表上下文 |
| Toast / 横幅 | Toast 只用于瞬时反馈；需要用户处理的状态（待应用、断连、即将过期）使用常驻横幅 |
| 图表 | 调色板最多 6 色并对色盲友好；坐标轴与图例清晰；支持缩放、悬浮十字线、断点连线表示数据缺失 |

### 9.4 响应式策略

| 断点 | 设备 | 布局 |
| --- | --- | --- |
| ≥ 1280px | 笔记本、显示器 | 完整侧栏 + 多列卡片 |
| 768\~1279px | 平板、现场调试 | 侧栏折叠为图标栏，卡片两列 |
| < 768px | 手机 | 侧栏变抽屉；表格转卡片列表；复杂配置页给出「建议使用电脑」提示但仍可操作 |

桌面优先，保证 1366×768 的小屏笔记本无横向滚动；平板场景确保触控可用，是现场工程师最常用的设备之一。

### 9.5 无障碍与可用性

- 全键盘可达：焦点环清晰可见，Tab 顺序合理，弹窗焦点陷阱与 Esc 关闭。
- 语义化标签与 ARIA：表格、表单、状态变化（aria-live）、图标按钮提供可读名称。
- 不依赖颜色 / 声音传达关键信息；错误提示与字段通过 aria-describedby 关联。
- 支持 200% 缩放不破版；尊重系统深色 / 减少动态效果偏好。

### 9.6 品牌与多产品一致性

- 统一：布局骨架、组件形态、交互模式、状态语义色、图标风格、文案语气。
- 可变：产品名与 Logo、主色、登录页插图、Dashboard 默认卡片、产品专属页面。
- 登录页和关于页展示「产品名 + 公司统一标识」，既保持产品辨识度，也保持平台级品牌一致。
- 文案语气：简明、动词开头、避免技术黑话；错误文案遵循「发生了什么 + 为什么 + 怎么办」。

### 9.7 排版用法

| 用途 | 字号 / 行高 | 字重 | 说明 |
| --- | --- | --- | --- |
| 页面标题 | 20 / 28 | 600 | 每页一个，位于面包屑下方 |
| 区块 / 卡片标题 | 16 / 24 | 600 | 设置页分区、Dashboard 卡片 |
| 正文、表单文字 | 14 / 22 | 400 | 默认字号 |
| 表格内容 | 13 / 20 | 400 | 数值列右对齐，启用等宽数字 |
| 辅助说明、标签 | 12 / 18 | 400 | 使用次要文字色 |
| 指标主数值 | 24 / 32 | 600 | 启用等宽数字，单位用 12 号次要色 |
| 报文、日志 | 13 / 20 | 400 | 等宽字体，十六进制按字节分组 |

字重只用 400、500、600 三档；中文不使用斜体，强调用字重或颜色而不是下划线（下划线保留给链接）。

### 9.8 布局尺寸与层级

| 项 | 规格 |
| --- | --- |
| 顶栏 | 高度 56px，固定在顶部 |
| 侧栏 | 展开 220px、折叠 64px；1280px 以下默认折叠 |
| 内容区内边距 | 桌面 24px、平板 16px、手机 12px；最大宽度 1600px 居中 |
| 栅格 | 12 列，列间距 16px；Dashboard 卡片间距 16px |
| 页面标题区 | 面包屑 20px + 标题 28px，整体上下留白 16px |
| 卡片 | 内边距 16px，标题与内容间距 12px |
| 底部固定操作条 | 高度 56px，水平内边距 24px，按钮间距 8px，位于内容区之上不遮挡最后一行内容 |
| 抽屉 | 编辑 480px、含表格的详情 640px；手机全屏 |
| 弹窗 | 小 400px、中 560px、大 720px |
| 表单控件宽度 | 短 160px、中 320px、长 480px、满宽；宽屏标签列 160px |

层级（z-index）约定，避免各产品自行加数：

| 层 | 值 |
| --- | --- |
| 内容区 | 0 |
| 常驻横幅（待应用、断连、调试中） | 90 |
| 固定顶栏与侧栏 | 100 |
| 底部固定操作条 | 110 |
| 抽屉 | 200 |
| 弹窗 | 300 |
| Toast | 400 |
| 下拉、Tooltip 等浮层 | 500 |

### 9.9 控件尺寸与状态

| 控件 | 尺寸 | 状态与视觉 |
| --- | --- | --- |
| 按钮 | 默认 36px，紧凑 28px，触控 44px；水平内边距 16px | 悬停加深 8%，按下加深 14%；加载态显示图标并禁止重复点击 |
| 输入框 / 下拉 | 高度 36px，水平内边距 12px，边框 1px | 悬停边框加深；聚焦显示 2px 主色焦点环（外移 2px）；错误态边框用危险色并在下方显示文字；禁用态背景用页面底色、文字降为 60% 不透明 |
| 只读 | 与输入框同尺寸 | 去掉边框，直接显示为文本，区别于禁用态，并配合「只读」徽标 |
| 开关 | 36 × 20px | 开启用主色，关闭用离线灰；旁边必须有文字说明 |
| 标签 Tag | 高度 22px，圆角 4px，字号 12px | 语义色浅底加深色文字；不单靠颜色，需带文字 |
| 表格行 | 默认 40px，紧凑 32px；表头 40px | 悬停主色 4% 底色，选中主色 8% 底色，表头固定并加底边线 |
| Toast | 宽 360px，位于右上 | 成功 3 秒自动消失，错误需手动关闭 |
| 焦点环 | 2px 实线，主色，外移 2px | 所有可交互元素统一，不允许用 outline: none 而不提供替代 |

### 9.10 图表规范

序列色取自色盲友好的调色板，不使用状态色（成功、警告、危险），避免与告警含义混淆。

| 序列 | 浅色 | 深色 |
| --- | --- | --- |
| 1 | #0072B2 | #4DA3E0 |
| 2 | #E69F00 | #F0B429 |
| 3 | #009E73 | #2FBF9B |
| 4 | #CC79A7 | #E08FBC |
| 5 | #56B4E9 | #7CC8F2 |
| 6 | #D55E00 | #F2874B |

- 线宽 2px，数据点默认隐藏、悬停显示（半径 5px）；面积图填充 12% 透明度。
- 网格线 1px，使用边框色 50% 透明度；坐标轴文字 12px 次要色；图例放在图表上方，最多 6 项，超出提供筛选。
- 默认高度 240px，Dashboard 卡片内最小 200px；悬停显示十字线与提示框，提示框列出各序列当前值与单位。
- 数据缺失处断线，不做插值连线；时间轴按设备时区显示。
- 实时曲线每秒合并刷新一次；点数过多时服务端降采样，前端保留缩放与区间选择。

### 9.11 Token 清单与换肤约束

- 所有视觉属性集中定义在一份 Token 源文件中，构建时生成 CSS 变量与设计工具 Token；命名规则为 `--类别-语义-状态`。
- 产品只能提供品牌主色、Logo 和登录页插图；工具生成衍生色后，必须运行**前景/背景配对矩阵**检查。状态语义色、间距、圆角、字号、层级不可由 Pack 覆盖。
- 视觉 CI 覆盖浅色/深色、默认/hover/active/selected/disabled/error/loading/focus、文字/图标/边界/底色等真实组件组合；只检查 token 单值不算通过。
- 无障碍回归除色彩对比外，还包括键盘操作、焦点顺序、屏幕阅读器名称、状态播报、200% 缩放和减少动态效果。
- 图表状态不得只靠颜色区分；至少提供线型、点型、标签或数据表替代。

## 10. 安全设计

### 10.1 威胁模型与资产

安全设计至少覆盖以下资产：设备身份/私钥、账号/Session/API Token、配置与策略、固件与升级链路、遥测/控制数据、审计日志、许可证/Entitlement、备份和产品静态注册表。

威胁来源包括：不可信浏览器页面、同一局域网主机、现场维护人员、被攻陷的本机进程/业务服务、恶意或损坏的升级包、错误配置/时间、存储损坏和资源耗尽。每个资产需映射到入口、信任边界、缓解措施和测试证据。

### 10.2 安全控制基线

| 方面 | 措施 |
| --- | --- |
| TLS | 默认 HTTPS（TLS 1.2+，推荐 1.3）；HTTP 仅跳转；禁用弱套件。TLS 终止点唯一明确；反向代理到 webd 的链路和本机身份受限 |
| Host / Origin / DNS Rebinding | Host allowlist；状态变更 API 校验 Origin；WebSocket 握手校验 Host/Origin/Session；拒绝任意 Host 和未授权跨站访问 |
| CORS | 默认关闭；仅在确有跨源需求时配置精确 allowlist；禁止宽泛 `*` 与 credential 组合 |
| 代理信任 | 仅信任指定代理；清除客户端 Forwarded/X-Forwarded 头；不可信代理头不得参与身份、协议或来源地址判断 |
| 网络暴露 | 默认绑定管理接口/地址；WAN/蜂窝口访问默认拒绝，显式开放需权限、配置事务和审计；防火墙默认最小开放 |
| HTTP 安全头 | CSP 默认 `default-src 'self'`，脚本不使用 `unsafe-inline`/`unsafe-eval`（确有需要时用 nonce/hash）；配置 `object-src 'none'`、`base-uri 'self'`、`frame-ancestors 'none'`、X-Content-Type-Options、Referrer-Policy、Permissions-Policy；认证/能力/敏感 API 响应使用 `Cache-Control: no-store`；按目标浏览器验证。HSTS 仅在证书/主机名部署形态允许且经 ADR 验证后启用，不对所有私有 IP/自签场景强制假设 |
| 认证会话 | 服务端可撤销 Session；安全 Cookie；CSRF + Origin；登录限速；会话轮换/吊销；API Token 哈希存储和 scope 限制 |
| 授权 | 对每个 operation 做权限、资源范围、能力和 Entitlement 检查；页面显隐不构成安全控制；批量 API 逐对象鉴权 |
| IPC | Socket/总线 ACL、peer identity、服务用户隔离、方法 allowlist、消息大小/超时/取消、错误码和调用审计 |
| 前端防护 | 默认转义；禁止将不可信内容直接注入 HTML；本地依赖锁版本并扫描；CSP 与资源本地化 |
| 输入与命令 | 后端按 Schema 权威校验；限制长度/范围/类型；系统命令只允许固定方法/参数结构，禁止 shell 字符串拼接 |
| 上传/导入/导出 | 限文件类型/大小/总展开量/文件数/压缩比/处理时长；拒绝路径穿越、符号链接逃逸、设备文件；CSV 防公式注入；YAML/JSON 禁任意对象构造 |
| 固件升级 | 验证签名、产品/型号绑定、版本策略和制品摘要；anti-rollback 按型号配置；A/B 只有硬件/Bootloader 支持并验证后才能启用 |
| 敏感信息 | 密码、Token、私钥不回显；日志/诊断包脱敏；备份默认排除私钥，需导出敏感材料时强授权、加密和审计 |
| Service 访问 | 不预置通用后门；设备绑定的短期授权、明确范围/工单原因、到期撤销和全量审计；可由客户禁用 |
| 存储 | 数据目录最小权限；数据库/备份完整性检查；敏感材料使用设备安全存储能力（若有）或文件权限/加密保护并记录边界 |
| 审计 | 追加式写入、单调序号和哈希链/分段摘要完整性校验、保留周期/空间上限/自动轮转；可用时将摘要外部锚定/转发；应用层禁止管理员手工删除；高风险操作审计失败按策略 fail-closed。不得把本地哈希链表述为抵御已取得 root 权限攻击者的绝对防篡改 |
| 时间 | 证书与审计使用可信时间策略；单调时钟用于时长/超时；记录 bootId、时间同步状态和时钟变更事件 |
| 供应链 | 生成 SBOM、许可证清单、工具链版本、制品摘要和签名/来源证明；CI 进行依赖漏洞、SAST 和构建来源检查 |

### 10.3 证书生命周期

设计必须定义设备身份建立和首次信任流程，不能把“用户忽略浏览器告警”作为正常部署方法。证书生命周期至少覆盖：生成/导入 CSR、签发、安装、到期前告警、续期、替换、私钥轮换、证书链变更、失效后的本地恢复。私钥存储位置、访问进程、备份规则和是否有硬件 Key Store，按型号事实确认。

### 10.4 时间与离线设备安全

区分墙上时钟与单调时钟：证书/许可证有效期和审计时间依赖墙上时钟可信策略；配置回滚倒计时、租约和任务超时使用单调时钟。RTC 未同步、NTP 不可用或发生大幅回拨时，必须定义登录、Token、证书、许可证和升级验签各自的行为。审计同时记录 UTC wall-clock、monotonic 值、bootId 和事件序号，标明时间可信状态。

### 10.5 升级与合规边界

升级链路须定义签名信任根、型号/产品绑定、版本策略、防回退、断电恢复、迁移前备份、健康检查、失败回滚和物理/串口恢复。双分区 A/B 只是具备相应 Bootloader/分区能力型号的实现选项，不是平台默认假设。

对 IEC 62443、欧盟 CRA、EN 18031 等目标的具体适用范围、版本、责任主体和证据要求须由合规负责人确认；设计文档列出控制项不代表产品已经通过认证或达到合规结论。

### 10.6 安全生命周期

建立漏洞接收、分级、修复、回归、客户通知、安全更新支持周期和旧固件风险处置流程。安全需求应映射到设计章节、代码模块、测试用例和证据文件；无证据的控制项状态只能标记为“设计要求”或“待验证”，不能标记为“已验证”。

## 11. 构建、发布与升级

### 11.1 构建流水线

```text
提交代码
  → Lint / 类型检查 / 依赖边界检查（Core 不得依赖 Pack）
  → Manifest / 权限目录 / OpenAPI / Schema 校验
  → 检查所有页面、Widget、API、角色 preset 的引用完整性
  → 构建静态路由/权限/能力注册表并生成摘要
  → 单元测试 + 组件测试 + 授权策略测试
  → 按产品矩阵构建（iems、dcu…）→ 体积预算检查
  → OpenAPI 契约测试（Mock、webd、业务服务 IPC）
  → Playwright E2E（角色 × 资源范围 × 能力 × Entitlement）
  → 安全测试（SAST、依赖、SBOM、文件导入、Host/Origin、CSRF、WebSocket）
  → 故障注入（webd kill -9、watchdog、断电/重启、磁盘满、数据库迁移失败）
  → 真机性能/内存/并发测试
  → 产物压缩、rootfs/package 集成、签名、制品摘要与归档
```

### 11.2 目录与部署（设备侧）

以下为逻辑目录建议，最终路径/用户/组/只读挂载由目标系统集成 ADR 冻结：

```text
/usr/share/webui/                # 前端静态资源，只读
/usr/share/webui/registry/       # 构建生成的静态路由/权限/能力注册表，只读
/etc/webui/                      # 运维配置：监听地址、代理、日志保留策略等
/var/lib/webui/db/               # 用户、角色、Session 元数据、Token 哈希、偏好
/var/lib/webui/transactions/     # 配置事务 journal、快照索引和恢复状态
/var/lib/webui/tasks/            # 长任务状态、幂等键和结果摘要
/var/lib/webui/import-export/    # 受配额控制的临时/持久导入导出文件
/var/log/webui/runtime/          # 访问/运行日志，可轮转
/var/log/webui/audit/            # 安全审计，追加写/完整性校验/保留策略
```

`product.yaml` 和 `extension.yaml` 是构建输入，不作为可写运行时路由源；设备上只保存构建生成的只读 registry 与版本/摘要。证书私钥放置在系统密钥存储或受限目录，不能放入普通可下载配置文件。

**持久化边界**：用户/角色/Token 吊销、配置事务、任务状态和审计的权威存储必须明确；遥测缓存、用户偏好等可重建数据与安全数据分开。文件所有者/权限、Flash 写入频率、空间配额和数据恢复责任均需在目标设备验证。

### 11.3 升级、数据迁移与回滚

- 前端和 registry 随固件整体发布，不做运行时任意增量加载；前后端版本和 Core SDK 版本必须匹配。
- 用户/角色/权限、配置、任务、审计和产品数据分别具有 schema 版本和迁移策略；迁移开始前创建一致性快照/备份，迁移具备阶段日志和失败恢复。
- 备份应保证数据库、配置文件、证书引用和产品数据的一致性；不能简单逐文件复制正在变化的数据库。备份/恢复须验证摘要、版本和权限。
- 固件回滚与数据格式回滚必须一起设计。若新版本迁移不可逆，旧固件不能直接启动；需要兼容窗口、双写/分阶段迁移或恢复备份的方案，并在 ADR 中明确。
- A/B 分区只有在目标型号 Bootloader、分区布局、启动标志、健康检查和恢复机制验证通过后才启用。无 A/B 的型号必须定义其替代恢复路径。
- 升级包校验产品/型号绑定、签名、版本、防回退策略和制品摘要；升级中断电按 Bootloader 能力测试。升级后执行健康检查和配置迁移状态检查。
- 新增权限点时，预置角色按 Pack 版本规则补授并生成变更报告；自定义角色不自动授予新权限。用户被吊销的 Session/Token 不得因升级恢复。

### 11.4 数据库、备份和磁盘满策略

- 明确数据库引擎/文件格式、事务隔离和备份方式；数据存储按访问频率与恢复要求分层，减少 Flash 写放大。
- 数据迁移须可重复检测；中断后恢复到旧版本可启动状态或进入明确的维护恢复模式，不得留下半迁移状态。
- 磁盘/分区空间不足时停止非关键上传/导出、拒绝新增长任务、限制缓存；为事务回滚和安全审计预留空间。运行日志可以轮转，安全审计不可静默删除。
- 审计分区写满时，高风险操作按 fail-closed 处理；低风险运行日志按既定丢弃/轮转策略处理并记录告警。

## 12. 测试与质量保障

| 层次 | 内容 | 工具/方式 | 发布要求 |
| --- | --- | --- | --- |
| 单元测试 | 权限展开、角色合并、scope、能力/Entitlement 判定、Schema、事务状态机 | Vitest / 后端测试框架 | 关键策略 100% 分支或按项目覆盖率门槛 |
| Manifest/Registry | ID 唯一、权限引用、路由冲突、受控 override、版本兼容 | 构建工具校验 | 任一悬空引用或未批准覆盖阻断构建 |
| 契约测试 | OpenAPI 与 webd 响应、认证、权限、错误码一致 | Schemathesis / Dredd 等 | 所有 operation 有 security/权限定义 |
| IPC 测试 | peer identity、ACL、超时、取消、消息长度、非法调用 | 进程级测试/故障注入 | 非授权本机进程不能调用敏感方法 |
| E2E | 登录、改密、用户角色、页面呈现、配置事务、任务恢复 | Playwright + Mock | 至少覆盖每个产品和每个内置角色 |
| 权限矩阵 | API × 角色 × scope × 能力 × Entitlement | 由目录和 OpenAPI 生成 | 同时包含允许与拒绝用例；不可只测 UI 隐藏 |
| 配置事务 | 并发编辑、revision 冲突、重复 apply/confirm/rollback、webd 崩溃、重启恢复、watchdog 丢失 | 真机/进程故障注入 | R2/R3 流程必须可恢复；ROLLBACK_FAILED 可检测 |
| WebSocket | Origin/Host 拒绝、权限订阅、断线游标补发、重复/乱序、慢客户端、队列满 | 协议测试/压测 | 关键事件不静默丢失；过载行为可观察 |
| 长任务 | 幂等键重复、互斥组、重启恢复、授权撤销、取消边界 | 任务服务测试 | 不重复触发副作用；无权限任务不能继续执行 |
| 文件安全 | 路径穿越、符号链接、压缩炸弹、恶意 YAML、CSV 公式、超限文件 | 安全测试样本 | 拒绝或受限处理，不执行代码、不越界写文件 |
| 存储与迁移 | 数据库迁移中断、备份一致性恢复、磁盘满、审计写失败 | 故障注入 | 系统可启动且数据处于可解释状态 |
| 时间 | RTC 未同步、NTP 大幅校时、时间回拨、重启和 bootId 变化 | 模拟时钟/真机 | Session、证书、许可证、任务和审计行为符合策略 |
| 视觉/无障碍 | 前景/背景对比矩阵、深浅主题、键盘、焦点、ARIA、200% 缩放 | Playwright、axe 等 | 所有受支持 Token 配对通过；状态不只靠颜色 |
| 性能 | 首屏体积、P95/P99 API、RAM 峰值、WebSocket、分页/导出、任务并发 | 目标设备压测 | 按最低型号形成基线报告 |
| 安全 | 认证、越权、注入、CSRF、Host/Origin、DNS Rebinding、依赖/SAST | ZAP、Semgrep、依赖扫描和人工评审 | 高危问题清零或有经批准的风险接受记录 |
| 兼容性 | 目标浏览器最近 3 年主流版本、平板 Safari/Chrome | 本地矩阵/BrowserStack | 以产品声明的浏览器支持清单为准 |
| 真机回归 | iEMS/DCU 产品、最低配置型号、关键驱动和真实服务 | 台架/自动化测试 | 发布前通过产品级验收 |

**必须纳入的验收场景**：

| 编号 | 场景 | 预期结果 |
| --- | --- | --- |
| T-01 | 用户无某能力但直接调用对应 API | 服务端拒绝；不依赖菜单隐藏 |
| T-02 | 许可证过期、签名错误或设备绑定不匹配 | 受保护 API 拒绝，原因进入审计 |
| T-03 | 多角色叠加、直接授权到期、scope 合并 | 授权结果符合单一策略引擎，过期 grant 立即失效 |
| T-04 | 管理员删除最后一个管理员或授予自己更高权限 | 服务端拒绝并审计 |
| T-05 | DNS Rebinding、伪造 Host、跨站 WebSocket | Host/Origin/Session 校验拒绝，不泄露数据、不执行写操作 |
| T-06 | 两个管理员同时改同一网络配置 | 仅一个基线版本提交成功；另一方收到 409 |
| T-07 | webd 在配置事务倒计时期间 kill -9 | 独立 watchdog 继续运行，到期正确回滚 |
| T-08 | 配置应用后设备断电/重启 | 启动恢复先处理未确认事务，不进入不确定网络配置状态 |
| T-09 | WebSocket 断线期间发生告警/任务变化 | 重连后补发或要求快照同步；不静默丢失关键状态 |
| T-10 | 同一升级/恢复出厂 API 重复提交 | 返回同一 task/事务，不重复执行副作用 |
| T-11 | 数据库迁移中断电或磁盘空间不足 | 可启动并恢复到已定义版本；不出现半迁移数据 |
| T-12 | 时间回拨、RTC 未同步、NTP 大幅校时 | Session/证书/许可证/任务/审计行为符合分别定义的时钟策略 |
| T-13 | 压缩炸弹、路径穿越、恶意 YAML、CSV 公式 | 拒绝/限额处理；不越权写文件、不执行代码、不污染导出表格 |
| T-14 | 超大 pageSize、非法 sort、超长时间跨度 | 服务端按限额拒绝/裁剪，资源消耗可控 |
| T-15 | 审计分区写满后执行升级/恢复出厂 | 按风险策略拒绝或受控处理，不静默丢失关键审计 |
| T-16 | 关键事件写入后、发布前/发布中服务崩溃，或设备重启导致 bootId 变化 | 从持久 outbox/事件日志恢复；streamId/分区序号连续；允许重复投递但不丢失已提交事件，按 eventId 去重；游标过期/存储重建时返回 resync_required |
| T-17 | WS 连接期间用户登出、角色撤销或 scope 缩小 | 服务端撤销订阅/关闭连接；撤销后不再发送被禁止资源事件 |
| T-18 | 用户取消自己/他人任务，或在不同任务阶段取消 | owner 分支须满足 ownerCancel、创建者匹配、原任务执行授权/能力/Entitlement、scope 和安全阶段；管理员跨用户分支须 `core.task:cancel` + scope + 安全阶段；其他请求拒绝并审计 |
| T-19 | 审计记录被删除、插入、重排或哈希链损坏 | 完整性检查报警并保留证据；本地 root 攻击者能力边界如实记录，不宣称绝对防篡改 |
| T-20 | 任务步骤发生外部副作用后、状态 checkpoint 前崩溃 | 恢复时通过 stepId/回执对账；不确定时进入 `RECOVERY_REQUIRED`，不得盲目重复危险步骤 |

**Mock Server**：根据 OpenAPI 与各产品 fixtures 模拟设备，支持产品、型号、能力、Entitlement、角色和 scope 切换。Mock 用于前端开发，不替代 IPC、安全边界、watchdog 和真机性能测试。

## 13. 可观测与诊断

- **前端**：错误边界捕获渲染异常并提供诊断 ID；诊断信息包含产品、固件、前端版本、requestId 和错误码，不含 Token、密码、私钥、敏感配置或完整个人数据。
- **后端访问日志**：结构化记录 requestId、actor ID（可脱敏）、API operationId、资源 ID 摘要、状态码、耗时和结果；慢请求/5xx/限流触发诊断告警。
- **时间可信性**：事件同时记录 UTC wall-clock、monotonic timestamp、bootId、sequence 和 time-synchronized 标识；时区/DST 只影响展示，不改变事件排序依据。
- **时钟审计**：NTP 校时、RTC 异常、时间回拨/跳变、时区变更作为系统事件记录；不能仅凭墙上时间戳推断严格顺序。
- **运行日志与安全审计分离**：运行日志可按容量轮转；审计记录追加写、受访问控制和完整性校验，使用单调序号及哈希链/分段摘要发现删除、插入和重排；在有可信网络/平台时将摘要或审计副本外部锚定。自动轮转属于保留策略，不等于用户可删除。本地完整性机制不能抵御已取得设备 root 权限的攻击者重写全部存储，威胁边界必须如实说明。
- **审计失败策略**：高风险操作在审计无法持久化时 fail-closed；低风险操作是否允许缓冲须明确定义并暴露告警/计数，不得无声丢弃。
- **一键诊断包**：Admin / 经授权 Service 可导出；包括系统状态、脱敏配置副本、近期运行日志、必要的审计摘要和网络状态。导出文件按配额、过期、权限、完整性和敏感数据规则管理。
- **指标**：webd 内存、CPU、连接数、API P95/P99、IPC 延迟/错误、任务队列、WS 队列/丢弃计数、磁盘/数据库使用量、审计写入状态、配置事务/watchdog 状态。

## 14. 开发流程与团队协作

| 角色 | 职责 |
| --- | --- |
| 平台组（Core 团队） | 维护 core-sdk、core-ui、App Shell、通用模块、构建工具、权限策略、静态 Registry 与平台安全 |
| 产品组（Pack 团队） | 编写 product.yaml、产品页面/组件、i18n、能力/Entitlement 声明、产品 OpenAPI/Schema 和 Mock fixtures |
| 后端 / 系统 | webd、业务服务 API、IPC ACL、配置事务协调器/watchdog、任务执行器、持久化/迁移 |
| 设计 | 维护 Design Token、组件库与页面模板；负责语义色配对和无障碍验收 |
| 测试 / 安全 | 授权矩阵、E2E、故障注入、渗透、SBOM/供应链和合规追溯 |

**新产品接入清单**

1. 创建 `products/<id>/`，编写 `product.yaml`，声明模块、页面、权限、Capability/Entitlement、角色预置和主题。
2. 编写产品 OpenAPI extension 与 Schema；每个 operation 声明安全元数据、权限、scope、能力/授权条件和审计等级。
3. 构建期生成产品静态 Registry；禁止服务启动时向 webd 动态注册外部路由。
4. 实现产品页面和 Widgets，只通过受控扩展点复用 L0–L3 组件。
5. 补充 Mock fixtures、权限矩阵、对象级越权、E2E、导入导出安全和真机测试。
6. 通过依赖边界、权限引用、API 契约、视觉对比、体积/性能和安全评审后，纳入产品 CI 矩阵并随固件发布。

新增产品预期只需编写 Pack；若必须修改 Core，先判断是否缺少通用扩展点，并由平台组评审，不允许 Pack 直接复制或分叉 Core。

### 14.1 工程任务与需求追踪规则

- 每个可交付工作项必须分配唯一任务 ID（见附录 D），任务必须关联至少一个需求 ID（见附录 E）和一个验证项；禁止只有“开发完成”而没有验收证据的任务。
- 工作项状态统一为：`待确认`、`就绪`、`进行中`、`受阻`、`待评审`、`待验证`、`已完成`、`已取消`。`已完成`必须同时满足代码/配置交付、自动化测试、评审、文档和证据归档条件。
- 任务可以拆分，但不能改变其需求 ID、验收结果和风险级别；拆分后的子任务仍需可追溯到父任务。跨团队依赖必须显式记录，不通过口头约定隐式传递。
- P0 表示架构冻结或首个产品安全运行所必需；P1 表示首个产品发布必需；P2 表示后续增强。优先级是交付顺序，不代表可以跳过安全控制。
- P0/P1 的“首个产品发布”是指计划实际出货/交付的首个产品版本；只要该版本启用某项功能，该功能所需的条件性任务/ADR/测试必须在该版本门禁前完成。不得将 P1 仅因工程排期改称“首发后增强”；若经产品决策缩小首发功能范围，必须同步更新产品构建清单、需求矩阵和发布检查表。
- 每个产品/版本必须生成受版本控制的 `release-scope.yaml`（随对应 release tag 固化），最少包含 `schemaVersion`、`productId`、`releaseId`、功能 `enabled` 状态、关联 ADR/任务/需求、审批角色与构建摘要。功能最终是否暴露按 `product.yaml` 功能声明 AND `release-scope.yaml` 决定；任一方关闭即不得生成相关页面/路由/API/Registry/设备执行入口。条件性功能只有在发布清单明确关闭且最终制品确实未暴露时，才可标记为该版本“不适用”；仅隐藏菜单或禁用按钮不构成关闭。CI 必须校验 release scope 与最终 OpenAPI、静态 Registry、固件模块清单和发布检查表一致，产品 owner、平台 owner 与安全责任方按功能风险审批。

最低结构示例（字段值仅为格式示例，发布时必须替换为真实产品版本和审批记录）：

```yaml
schemaVersion: 1
productId: iems
releaseId: "2.3.0"
features:
  firmwareUpgrade:
    enabled: false
    adr: ADR-08
    tasks: [WBUI-062]
    requirements: [REQ-UPD-001]
approvals:
  productOwner: "<review-record-id>"
  platformOwner: "<review-record-id>"
  securityOwner: "<review-record-id>"
```

`enabled: false` 时，最终固件不能包含可调用的升级 API/路由/Registry/设备执行入口；`enabled: true` 时，WBUI-062 及关联需求必须通过型号级验证。
- 任务负责人使用角色/团队归属表达；个人执行人、计划日期和实际完成日期由项目管理系统维护，不在设计文档内虚构。

### 14.2 Definition of Ready（进入开发的条件）

任务进入“就绪”前，至少应具备：

1. 明确的任务 ID、需求 ID、优先级、责任团队和依赖任务；
2. 描述输入、输出、影响模块、调用边界和不在范围内的事项；
3. 对 API/IPC/Schema/权限/Capability/Entitlement 有影响时，已给出契约草案或明确由哪个 ADR 决策；
4. 定义正常路径、失败路径、超时/取消/重启行为，以及是否有副作用；
5. 明确可自动化的验收条件、所需测试环境和证据形式；
6. 涉及真机、Bootloader、证书、密钥、数据迁移或网络失联风险时，已列出设备型号/硬件前置条件。

条件不齐的任务保持“待确认”，不得用开发过程中的默认假设替代决策。

### 14.3 Definition of Done（完成的条件）

任务只有同时满足以下条件才能标记为“已完成”：

- 实现与已接受的 ADR、OpenAPI、权限目录、Schema 和 Manifest 一致；
- 正向、负向、边界和故障路径测试通过，结果关联任务 ID 与需求 ID；
- 对安全敏感接口完成未授权、越权、资源范围、能力缺失和授权撤销测试；
- 对持久化/任务/事务类逻辑完成重启恢复、重复请求和故障注入测试；
- 静态分析、依赖扫描、SBOM、制品摘要和许可检查达到对应门禁要求；
- 文档、运维/恢复说明、告警与可观测字段同步更新；
- 评审意见关闭，剩余风险有明确接受人、期限和缓解措施；
- 所需目标设备测试报告已归档；若尚未真机验证，只能标记“软件实现完成 / 真机待验证”，不能标记为产品发布完成。

### 14.4 接口与变更控制

- API、IPC 方法、权限 ID、Capability ID、Entitlement ID、Schema 字段和 Registry 结构均视为受控契约；变更必须更新版本/兼容说明、生成物和契约测试。
- 对外 API 变更必须先更新 OpenAPI，再更新实现和客户端；权限目录、Manifest 与 Registry 的生成结果由 CI 比对，禁止手工修改生成产物。
- 任何涉及信任边界、授权语义、配置事务、升级恢复、持久化格式或硬件能力的变更，必须新增/更新 ADR；普通代码评审不能替代架构决策。
- 运行中设备的数据库/配置迁移必须有向前迁移和失败恢复方案；不能证明可回滚时，必须明确采用备份恢复/维护恢复模式，而不是声称支持无损回退。

## 15. 里程碑与路线图

| 阶段 | 周期（参考） | 交付内容与退出条件 |
| --- | --- | --- |
| M0 设计与验证 | 3–4 周 | 冻结架构/权限/API/IPC/能力/配置事务 ADR；完成威胁模型；以一个产品跑通 Manifest → 静态 Registry → 页面 → API → RBAC/scope/Capability 的 PoC；验证 watchdog 独立运行 |
| M1 平台基座 | 6–8 周 | App Shell、Session、用户/角色/Token、权限策略引擎、静态 Registry、Schema、配置事务协调器/持久化 journal、Mock Server、基础 CI |
| M2 通用模块 | 6–8 周 | Dashboard、网络、时间、证书、备份恢复、日志/审计、诊断、任务执行器、WebSocket 游标恢复；固件升级仅对 release-scope 启用的型号实施；完成安全负向测试 |
| M3 首个产品落地（iEMS） | 6–8 周 | iEMS Pack、点位/能流/策略/南北向配置；验证能力/Entitlement、资源范围、配置事务和真机性能 |
| M4 第二产品（DCU）验证复用 | 4–6 周 | DCU Pack；统计复用率、Core 缺口和扩展点；通过同一策略引擎与产品级测试 |
| M5 增强 | 持续 | LDAP/RADIUS、TOTP、配置模板、审计转发、更多诊断能力；任何运行时插件/热插拔需求需重新做威胁模型和 ADR |

### 15.1 阶段进入与退出门槛

| 阶段 | 进入条件 | 退出条件（必须有证据） | 不满足时的处理 |
| --- | --- | --- | --- |
| M0 设计与验证 | 产品范围、至少一个目标型号和现有服务清单可获得 | P0 ADR 已接受；核心 PoC 跑通；配置 watchdog 故障演练通过；威胁模型和接口契约完成评审 | 不进入 M1；待决项转为阻塞任务，不以临时实现绕过 |
| M1 平台基座 | M0 退出；模块 owner 和 CI 环境落实 | 静态 Registry、Session、授权策略、API 契约、配置 journal 和基本恢复测试通过 | 仅允许在 Mock/开发设备继续，不发布设备版本 |
| M2 通用模块 | M1 退出；最低配置目标设备确定 | 网络/证书/时间/备份/日志/任务/WS 核心用例通过；升级核心用例仅对 release-scope 启用的型号强制；关键负向安全测试通过 | 受影响模块标记不可发布，不得由 UI 隐藏代替缺失的后端控制 |
| M3 iEMS 首产品 | M2 退出；iEMS 产品 API、数据对象和设备能力矩阵冻结 | iEMS Pack 构建、权限矩阵、配置事务、真机资源与现场流程验收通过 | 不进入 DCU 规模复用结论；记录 iEMS 特有问题 |
| M4 DCU 复用验证 | M3 退出；DCU 点表/表计/通道对象模型明确 | DCU Pack 不复制 Core；权限、scope、通信调试、性能和真机验收通过 | 重新评估扩展点和 Core 边界，不用产品分支掩盖抽象缺陷 |
| M5 增强 | 首发版本稳定性和客户需求有数据支撑 | 每项增强有单独 ADR、威胁模型、兼容和资源评估 | 未接受的增强保持关闭，不进入默认固件 |

**排期说明**：原路线图中的周数仅是规划估算，不构成承诺；硬件到位、ADR 结论、外部依赖和故障演练是实际关键路径。不可通过缩短测试或省略证据来满足日历日期。

**成功度量**：新产品接入周期、Core 复用比例、权限/越权缺陷数、未完成事务恢复成功率、关键事件恢复率、真机首屏/内存/P95/P99、资源耗尽行为、安全评审问题和现场任务完成时间。每项度量必须定义采集口径、基线型号、测试负载、数据保留位置和负责人；没有基线时只报告测量值，不给出改善率结论。

## 16. 风险与应对

| 风险 | 影响 | 应对 |
| --- | --- | --- |
| Pack 越过扩展点直接改 Core/全局 CSS | 平台分叉、升级困难 | 依赖边界 lint、CI 阻断；overrides 白名单评审 |
| 静态 Registry 与运行时注册混用 | API/权限不一致、攻击面扩大 | 只允许构建期生成；设备启动只校验完整性/版本 |
| 权限、能力、资源范围分别实现 | 越权或误拒绝 | 单一授权策略；API×role×scope×capability 自动测试 |
| 本机 IPC ACL 不完整 | 本机进程绕过 webd | peer identity、socket ACL、方法白名单和进程级负向测试 |
| DNS Rebinding/Host/Origin 缺失 | 局域网设备被跨站滥用 | Host/Origin allowlist、CORS 默认关闭、WS 握手校验和攻击测试 |
| 配置回滚依赖 webd/浏览器 | webd 崩溃或断电后设备失联 | 独立事务协调器/watchdog、持久化快照、启动恢复、带外恢复 |
| 多管理员并发改配置 | 静默覆盖和不可预测状态 | revision/ETag、锁租约、409 冲突和幂等操作 |
| 长任务重复或升级互相冲突 | 重复副作用、Flash/系统损坏 | Idempotency-Key、任务持久化、互斥组和执行前重验授权 |
| WebSocket 断线丢事件 | 告警/任务状态不连续 | 事件序号/游标、关键事件补发、REST 快照重同步 |
| 设备存储/时钟能力差异 | 数据损坏、证书/审计排序问题 | 按型号声明存储/时钟能力；迁移与时间异常故障注入 |
| A/B/Bootloader 假设不适用于所有型号 | 升级失败无法恢复 | A/B 按型号启用；无 A/B 型号定义替代恢复路线 |
| 视觉色值只检查单色 | 文字对比不达标 | 前景/背景/状态矩阵 CI；无障碍测试 |
| 前端/查询/WS 资源无上限 | 低配设备 OOM 或响应迟滞 | 分页/查询跨度/队列/连接数/文件配额；目标机压测 |
| 安全控制无证据 | 无法证明实际有效 | 安全需求→代码→测试→证据追溯；未验证不得标记通过 |

## 17. 待确认事项与设计冻结门槛

本节仅列出文档与现有输入无法确定的项目事实。任何默认建议都不是已验证事实；必须形成 ADR、负责人和验证证据。

| ADR / 决策项 | 需要确认的事实 | 推荐基线（待确认） | 责任方 / 退出条件 |
| --- | --- | --- | --- |
| ADR-01 webd 与部署 | webd 语言、Nginx/lighttpd 是否独立、TLS 终止点、进程管理方式 | 单一 TLS 终止点；业务 API 只经 webd | 平台/系统；完成部署图和端口/网卡清单 |
| ADR-02 IPC | 现有服务使用 ubus、D-Bus、MQTT 还是 UDS；peer identity/ACL 能力 | 首选 UDS；其他方案需等价 ACL | 系统/后端；完成 ACL 负向测试和接口契约 |
| ADR-03 设备资源 | 最低 RAM/Flash/CPU、存储介质、写入寿命、浏览器矩阵 | 以最低配置型号为发布基线 | 硬件/系统/测试；完成真机性能报告 |
| ADR-04 Capability/Entitlement | 能力服务来源、许可签名/信任根、设备绑定、时钟回拨策略 | capabilityRevision；许可校验 fail-closed | 产品/安全；完成能力 schema 和失效策略 |
| ADR-05 配置事务 | watchdog 承载进程、启动顺序、持久化分区、回滚与带外恢复 | 独立于 webd 的协调器；未确认事务重启后回滚 | 系统/后端；完成 kill -9/断电/回滚失败演练 |
| ADR-06 存储与迁移 | 数据库选型、备份一致性、迁移和回滚、磁盘满策略 | 安全数据与可重建缓存分离 | 后端/系统；完成迁移中断/空间不足测试 |
| ADR-07 时钟与证书 | RTC 精度、NTP 可用性、首次信任、私钥存储、证书更新渠道 | monotonic 用于超时；wall-clock 带可信标识 | 系统/安全；完成无时钟/回拨/过期恢复验证 |
| ADR-08 升级能力 | 各型号 Bootloader、分区、A/B、可信启动、硬件密钥、防回退 | 按型号声明；不具备则提供替代恢复方案 | Bootloader/硬件/安全；型号能力矩阵与断电测试 |
| ADR-09 RBAC 产品资源模型 | iEMS/DCU 设备/通道/表计/点位层级和默认 scope | 权限与 scope 分离，授权并集合成 | 产品/后端/安全；API×scope 测试通过 |
| ADR-10 现场访问策略 | 管理口、WAN/蜂窝口是否开放；客户网络隔离要求 | 默认仅管理网；WAN 显式开放 | 产品/安全；网络威胁评审通过 |
| ADR-11 合规目标 | IEC 62443 安全等级、CRA 适用范围、EN 18031 等适用版本/产品边界 | 由合规负责人出具适用性判断 | 合规/产品；完成追溯矩阵和证据计划 |
| ADR-12 Service 流程 | 厂商授权发放者、客户同意、工单、到期、审计和撤销 | 设备绑定、短期 scope token、客户可禁用 | 售后/安全/产品；流程和审计验收完成 |
| ADR-13 外部身份源 | 是否需要 LDAP/RADIUS/TOTP、离线情况下管理员恢复 | 首发本地账号；外部身份源后续评估 | 产品/安全；需求和故障降级方案确认 |
| ADR-14 白标/主题 | 客户是否可自定义 Logo/主色，是否有租户差异 | 只允许品牌 Token 白名单 | 产品/设计；Token 覆盖清单冻结 |

### 17.1 设计冻结条件

**平台架构冻结前必须完成（P0）**：

1. Core/Pack 构建边界、静态 Registry、扩展 ID 和构建流程冻结。
2. 权限目录完整；RBAC 合成、scope、额外授权、管理员保护和 API operation 授权映射清楚。
3. Capability、Entitlement、Runtime Health 的类型与服务端校验路径清楚。
4. TLS/Host/Origin/CORS/WS/IPC 信任边界及本机服务 ACL 清楚。
5. 配置事务状态机、watchdog、持久化快照、启动恢复、并发控制和失败恢复可测试。
6. API 全部使用 `/api/v1`；OpenAPI、权限目录、Schema、Manifest 和生成产物的单一事实来源边界清楚。

**首个产品发布前必须完成（P1，按该产品实际启用功能判定）**：

1. 长任务执行器、幂等键、互斥组、任务重启恢复和撤权/取消行为。
2. WebSocket 关键事件持久化、游标补发、快照重同步、订阅撤权、慢客户端和过载行为。
3. 数据库迁移、备份/恢复一致性、磁盘满、审计完整性和故障恢复验证。
4. 证书生命周期、设备时钟不可信、时间回拨与事件排序策略。
5. 上传/导入/导出安全、分页/查询上限、视觉对比矩阵和无障碍检查。
6. 最低配置真机性能、安全测试、故障注入和发布证据归档。
7. 如果首发固件提供升级功能，WBUI-062/ADR-08 的型号级签名、兼容性、回退与断电恢复验证是升级功能上线的前置条件；不能以“升级能力后续完善”替代门禁。

**平台增强阶段（P2）**：LDAP/RADIUS、TOTP、配置批量下发、审计外发增强，以及经过重新威胁评估的第三方扩展能力。

### 17.2 ADR 状态与决策流程

ADR 状态只能按以下流程变化：

```text
待提出 → 评审中 → 已接受 → 实施中 → 已验证
             └──→ 已拒绝（记录理由）
已接受/已验证 → 被替代（必须引用替代 ADR）
```

- **待提出**：问题、决策范围和责任角色尚未形成完整方案。
- **评审中**：至少有两个可比较选项，已经评估安全、资源、兼容、运维和失败恢复影响。
- **已接受**：决策人/评审角色、日期、选择理由、代价、迁移与回退计划已记录；这只表示决策获批，不表示实现完成。
- **实施中**：相关任务已进入工程计划，契约和代码开始按决策落地。
- **已验证**：对应验收用例和目标环境证据已归档。
- **已拒绝/被替代**：不得删除历史记录；必须记录理由、影响范围及替代决策。

**决策权限**：平台架构由平台负责人牵头；设备资源、启动恢复和存储由系统/硬件负责人共同确认；安全边界由安全负责人评审；产品行为和角色模型由产品 owner 确认；适用法规由合规负责人给出书面判断。单一团队不得代替其他责任方确认其未掌握的事实。

### 17.3 ADR 决策工作包

以下内容用于组织决策，不预先替项目选择尚未验证的技术方案。每项 ADR 的“输出物”是关闭条件；详细任务 ID 见附录 D。

| ADR | 必须回答的问题 | 比较选项/评估维度 | 必须输出的决策产物与验证证据 | 优先级 |
| --- | --- | --- | --- | --- |
| ADR-01 部署与 TLS | TLS 由代理还是 webd 终止；两者是否同进程/主机；代理身份如何可信；监听哪些网卡和端口 | 代理终止 vs webd 终止；进程合并 vs 分离；证书/私钥存储、Host allowlist、故障域、资源占用 | 部署拓扑、端口/接口清单、进程用户/组、代理头处理规则、Host/Origin 测试报告 | P0 |
| ADR-02 IPC | webd 到业务服务采用何种 IPC；如何认证 peer、限制方法和约束消息 | UDS、ubus、D-Bus 或现有受控总线；ACL 能力、取消/超时、消息尺寸、可观测性、集成成本 | IPC 方法目录、调用方 ACL、消息 Schema、错误码/超时语义、非授权进程负向测试 | P0 |
| ADR-03 最低硬件基线 | 最低型号、RAM/Flash/CPU/存储介质、浏览器和并发目标是什么 | 各候选型号实测；冷启动、并发用户、WS、图表、数据库、导入导出、日志增长 | 型号能力矩阵、最低配置定义、资源预算和真机压测报告 | P0 |
| ADR-04 Capability/Entitlement | 能力数据由哪个可信服务生成；许可验证密钥/信任根如何维护；设备绑定/撤销/回拨怎么处理 | 本地可信服务、签名 License、离线宽限与无效策略；字段可信来源和版本兼容 | Schema、签名与密钥生命周期、revision 语义、失效策略、篡改/过期/回拨测试 | P0 |
| ADR-05 配置事务/watchdog | watchdog 由谁托管；如何跨 webd 崩溃、系统重启和断电继续恢复；带外恢复是什么 | 独立守护进程、系统 supervisor/timer 或现有可靠机制；持久化 journal、启动排序、分区故障 | 状态机实现说明、journal 格式、恢复顺序、互斥策略、kill -9/断电/回滚失败证据 | P0 |
| ADR-06 数据库与迁移 | 用户/权限/任务/审计/产品数据使用什么存储；如何保证备份一致性和升级回退 | 现有数据库/SQLite 等候选；WAL、写放大、事务隔离、备份、迁移兼容窗口 | 存储分区图、Schema 版本策略、备份恢复演练、迁移中断与磁盘满测试 | P1 |
| ADR-07 时钟与证书 | 无 RTC/未同步/回拨时，Session、证书、License、任务超时和审计如何处理 | RTC/NTP/可信时间源；墙上时钟与 monotonic 的职责；首次信任和证书轮换渠道 | 时间信任状态机、证书首次信任/更新/恢复流程、回拨和过期测试 | P1 |
| ADR-08 Bootloader/升级 | 各型号是否支持 A/B、启动确认、签名验证、防回退和硬件根信任 | A/B 与非 A/B 替代恢复；Bootloader 能力和应用层更新的责任分界 | 型号级升级能力矩阵、包格式、回退策略、升级断电与健康检查报告 | P0（升级上线前） |
| ADR-09 RBAC 资源模型 | iEMS/DCU 的资源树、父子继承、批量操作和 scope 合成规则是什么 | gateway/device-group/device/channel/point-group 等实际模型；并集/最小权限边界 | 资源关系 Schema、grant 示例、对象归属规则、授权矩阵和横向越权测试 | P0 |
| ADR-10 现场网络访问 | WebUI 允许从哪些网口/网络访问；蜂窝/WAN 是否允许；维护入口如何隔离 | 管理网专用、多个可信管理接口、受控 WAN 访问；防火墙和现场部署差异 | 网络访问矩阵、监听策略、防火墙规则、Host/Origin 与跨网段测试 | P0 |
| ADR-11 合规适用性 | 哪些产品、市场、版本和责任主体适用哪些法规/标准 | 按产品/销售区域/部署形态逐项适用性判断；不以通用清单代替法规意见 | 适用性书面判断、控制映射、责任分工和证据计划 | P1（对外发布前） |
| ADR-12 Service 访问 | 厂商服务账号/授权由谁发放；客户同意、工单、时限、scope、撤销和审计如何闭环 | 现场临时授权、签名短期授权、客户审批；离线发放与撤销 | 售后流程、授权数据模型、用户界面、到期/撤销/离线场景测试 | P1（启用 Service 前） |
| ADR-13 外部身份源 | 首发是否需要 LDAP/RADIUS/TOTP；身份源不可用时如何登录和恢复 | 本地身份为主、外部目录、MFA；离线 fallback、账号锁定和恢复安全 | 身份源范围、故障降级、管理员恢复和安全测试 | P2（需求确认后） |
| ADR-14 白标与主题 | 客户能否替换 Logo/品牌色/文案；有哪些 Token 可覆盖；是否有多客户差异 | 受限 Token 白名单 vs 固定品牌；安全提示与状态色不可覆盖边界 | Token 白名单、对比度验证矩阵、Pack 示例和品牌验收 | P2 |

**ADR 关闭检查**：每条 ADR 必须记录状态、决策人/责任角色、评审日期、涉及任务 ID、受影响需求 ID、选项比较、选择理由、风险与代价、实施/回退计划、验证报告链接。没有证据的 ADR 不能进入“已验证”。

### 17.4 ADR 依赖顺序与阻塞规则

1. ADR-03（最低硬件基线）和 ADR-01（部署/TLS）是资源预算、部署和性能验证的前置输入。
2. ADR-02（IPC）与 ADR-09（资源模型）是 webd/业务服务授权链和 API 实现的前置输入。
3. ADR-04（能力/许可）必须与 ADR-09 一起验证；不能在权限策略之外单独实现客户端许可判断。
4. ADR-05（配置事务）依赖 ADR-01、ADR-02 和 ADR-03；未验证独立 watchdog 前，不得将 R2 配置功能标记为可发布。
5. ADR-06/07/08 是升级、证书、审计和任务恢复的前置输入；涉及升级的产品发布不能以“后续再确认”作为通过条件。
6. ADR-11 由适用性判断触发证据计划；本设计不对具体法规适用性作事实性结论。
7. ADR-12/13/14 只在对应功能进入产品范围时阻塞该功能，不应无理由拖延首发范围内无关模块。

## 18. 工程实施与交付治理

### 18.1 工程任务组织

附录 D 是工程任务基线。项目管理系统应以任务 ID 为主键导入/维护负责人、状态、计划日期、实际日期、阻塞原因和证据地址；本设计文档不假定具体项目管理工具。

任务拆分必须遵循：

- 每个任务有一个可验收的主要交付物；跨团队任务应拆分为契约/平台/业务/测试子任务。
- 依赖关系使用任务 ID 表达；任务不能通过“并行开发”绕过未冻结的 ADR。
- 对硬件相关任务，必须指定具体产品型号、硬件版本、Bootloader/分区信息和测试环境；没有设备时只能完成模拟/软件阶段。
- 任务阻塞要归类为“决策缺失、硬件缺失、外部依赖、实现问题、测试环境问题”，并指定解除阻塞所需证据。
- 任务关闭后，保留测试报告、日志、制品摘要、设备型号/版本和复现步骤。

### 18.2 交付分层

| 交付层 | 允许状态 | 解释 | 能否用于产品发布 |
| --- | --- | --- | --- |
| 文档/设计 | 已编写、待评审、已评审 | 设计说明和 ADR 决策记录存在 | 否，除非实现/验证也完成 |
| 软件实现 | 已实现、单元测试通过 | 代码可构建，模块级测试通过 | 否 |
| 集成验证 | 契约/E2E/进程级测试通过 | 组件已在目标组合中集成 | 仍需真机/安全要求验证 |
| 目标设备验证 | 指定型号测试通过 | 资源、故障恢复、硬件接口和现场行为符合要求 | 需满足发布门禁 |
| 产品发布就绪 | 全部适用门禁通过 | 风险、证据、运维、升级和回滚完整 | 是 |

**禁止状态混淆**：Mock 通过不等于 IPC 真机通过；进程 kill 测试不等于断电恢复通过；签名校验单测不等于 Bootloader 可信启动；SAST 无高危不等于渗透测试通过；设计声明不等于法规认证。

### 18.3 缺陷分级与风险接受

- **Blocker / Critical**：可绕过认证/授权、远程或本地未授权控制、密钥泄露、配置事务失控、升级导致不可恢复、关键数据损坏。发布阻断，不允许一般项目经理豁免。
- **High**：重要安全边界缺失、可靠性/数据完整性问题、核心功能不可用或资源耗尽风险。原则上阻断；仅允许经产品 owner、安全负责人和技术负责人联合书面接受，并有临时缓解与修复期限。
- **Medium / Low**：按产品影响和风险处理，必须记录，不得隐藏在“已知问题”文本中。
- 风险接受必须包含缺陷 ID、影响型号、触发条件、潜在后果、缓解措施、接受人、失效/复审日期和修复任务 ID。过期风险自动重新进入评审。

## 附录 A：权限点目录（基线）

以下是 Core 与首批 iEMS/DCU 的权限 ID 基线。产品实现可扩展，但不得复用含义不同的 ID。所有 API、Manifest、页面、Widget、角色和测试引用必须从该目录解析。

```yaml
core:
  - core.dashboard:read
  - core.telemetry:read
  - core.alarm:read
  - core.alarm:write
  - core.task:read
  - core.task:cancel
  - core.audit:read
  - core.audit:export

network:
  - network.interface:read
  - network.interface:write
  - network.route:read
  - network.route:write
  - network.firewall:read
  - network.firewall:write
  - network.cellular:read
  - network.cellular:write

system:
  - system.capability:read
  - system.time:read
  - system.time:write
  - system.cert:read
  - system.cert:write
  - system.backup:read
  - system.backup:execute
  - system.upgrade:execute
  - system.reboot:execute
  - system.factory-reset:execute
  - system.service:read
  - system.service:execute
  - system.log:read
  - system.log:export
  - system.diagnostics:execute

user:
  - user.account:read
  - user.account:admin
  - user.role:read
  - user.role:admin
  - user.session:admin
  - user.token:admin

debug:
  - debug.point:read
  - debug.point:write
  - debug.frame:execute
  - debug.serial:admin

iems:
  - iems.datapoint:read
  - iems.datapoint:write
  - iems.telemetry:read
  - iems.strategy:read
  - iems.strategy:write
  - iems.cloud:read
  - iems.cloud:write

dcu:
  - dcu.meter:read
  - dcu.meter:write
  - dcu.task:read
  - dcu.task:write
  - dcu.channel:read
  - dcu.channel:write
  - dcu.frame:read
```

**目录治理规则**：

- 本附录为阅读版基线；实际工程中由机器可解析的权限目录文件维护，附录由其生成，避免双份人工维护。
- 权限目录只定义“权限 ID 与语义”，不等于授予任何角色。Core 角色与 Pack 角色的精确授权集合必须在独立、版本化的角色策略源文件中定义（建议 `access-control/role-presets.yaml`），生成角色矩阵并接受安全评审；未声明的权限默认不授予。
- `core.task:cancel` 默认仅授予设备 Admin 的 Core 角色预置；普通任务创建者仅能在任务类型策略允许、任务仍处于可取消阶段且其当前身份/资源范围仍有效时取消自己创建的任务。跨用户/管理员取消必须具备 `core.task:cancel` 和对应资源范围。具体首发矩阵由 WBUI-025 验收冻结。
- `core.telemetry:read` 与 `iems.telemetry:read` 的资源域不同，不得互相替代；某个 API 必须绑定一个明确权限。
- 页面、Widget、设置分区引用权限必须能解析；API operation 未绑定权限则构建失败，匿名 API 必须显式进入匿名 allowlist。
- `admin` 不代表隐含 `read/write/execute`；需在目录依赖中显式表达。
- 通配符仅允许在构建输入出现，最终 Registry 和运行时请求全部展开为具体权限 ID。

## 附录 B：Core 页面模块清单（建议首批）

| 模块 | 页面 | 说明 |
| --- | --- | --- |
| core:auth | 登录、首次引导、修改密码、会话管理 | 强制改密、锁定提示、会话列表 |
| core:dashboard | Dashboard 框架、设备状态卡、告警卡、链路卡 | 支持 Pack 注册 widgets |
| core:network | 以太网、蜂窝、Wi-Fi、路由、防火墙、DNS / NTP、诊断工具 | 高风险项走回滚机制 |
| core:users | 用户、角色、权限、API Token、会话 | 自定义角色、依赖提示 |
| core:system | 设备信息、时间、证书、备份恢复、固件升级、重启、恢复出厂 | 升级向导、签名校验 |
| core:logs | 系统日志、审计日志、诊断包导出 | 实时尾随、字段过滤 |
| core:about | 版本、许可证、开源声明 | 满足开源合规披露 |

## 附录 C：术语

| 术语 | 含义 |
| --- | --- |
| Core | 所有产品共享的 WebUI 平台部分（Shell、通用模块、组件库、SDK、授权与构建工具） |
| Product Pack | 单个产品的构建扩展包：页面、组件、主题、权限、能力/Entitlement 声明、i18n、API/Schema 输入 |
| Manifest | 声明 Pack 内容与依赖的构建清单（`product.yaml`），不是运行时任意插件加载指令 |
| Capability | 设备硬件/软件功能能力；与许可证 Entitlement 和 Runtime Health 分离 |
| Entitlement | 经可信验证、对特定设备/功能有效的许可证或功能授权 |
| Runtime Health | 服务/链路当前健康状态，不表示设备永久具备某项能力 |
| RBAC | 基于角色的访问控制；结合独立资源范围和能力/授权策略形成最终授权 |
| Resource Scope | 主体可访问的资源对象集合/层级，如设备组、设备、通道、点位组 |
| webd | 设备侧 Web 网关，负责外部 API 认证、授权、审计、校验、聚合与实时订阅代理 |
| IPC | 设备本机进程间通信；需要 peer identity、ACL、方法约束和消息限制 |
| Registry | 由构建输入生成并随固件只读交付的路由、权限、Schema 和能力需求注册表 |
| Config Transaction | 具备 revision、持久化状态机、watchdog、确认/回滚和审计的配置变更事务 |
| Watchdog / Coordinator | 独立于 webd 请求生命周期的设备端事务协调者，负责超时回滚和启动恢复 |
| Idempotency-Key | 用于避免相同副作用请求被重复执行的幂等键 |
| Design Token | 以变量形式抽象的设计属性（颜色、字号、间距等）及其允许的前景/背景配对 |

---

## 附录 D：工程任务分解（实施基线）

> 状态初始值统一为“待确认”。任务表定义范围、主责角色、前置依赖、交付物和可验证的完成条件；个人执行人、日期和实时状态由项目管理系统维护。任务完成必须符合 §14.3。

### D.1 决策与设备基线 EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-001 | P0 | 建立产品/型号/硬件版本矩阵 | 产品、硬件、系统、测试 | 无 | 列出 iEMS/DCU 首发型号、RAM/Flash/CPU、存储介质、网口、RTC、Bootloader/分区、浏览器与现有服务；每项标注来源/待确认状态 |
| WBUI-002 | P0 | 关闭首发 P0 ADR（01–05、09、10；ADR-08 按升级功能适用性决策） | 平台、系统、硬件、后端、安全、产品 | WBUI-001 | 每项 ADR 有选项比较、接受人、受影响需求、任务链接和验证计划；ADR-08 必须形成各型号“启用/关闭升级”的明确结论；若启用，WBUI-062 在升级功能上线前关闭；若关闭，release-scope.yaml、OpenAPI、Registry 和固件执行入口均不得暴露升级能力 |
| WBUI-003 | P0 | 建立威胁模型与安全资产清单 | 安全、平台、系统、产品 | WBUI-001 | 覆盖账号/Token/证书/配置/任务/升级包/审计/IPC/现场网络；威胁与控制映射至 REQ-SEC 项 |
| WBUI-004 | P1 | 完成 P1/P2 ADR 适用性评审（06、07、11–14） | 后端、系统、安全、合规、产品、设计 | WBUI-001 | 对进入首发范围的 ADR 形成决策；不适用项有明确范围和理由；未决增强项保持关闭 |

### D.2 构建系统与 Core/Pack 注册表 EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-010 | P0 | Monorepo 包边界与依赖约束 | 平台 | WBUI-002 | Core 不依赖 Pack；Pack 只依赖 core-sdk/core-ui；CI 对违规依赖失败 |
| WBUI-011 | P0 | Manifest/Release Scope Schema 与校验器 | 平台、产品 | WBUI-010 | `product.yaml` 与 `release-scope.yaml` 字段、ID、引用、路径、版本、功能开关、主题/扩展点约束可机器校验；无效输入失败并给出定位 |
| WBUI-012 | P0 | 权限目录与 API operation 注册生成 | 平台、后端、安全 | WBUI-011 | 从权威权限目录/OpenAPI 生成权限映射；所有非匿名 operation 有认证、精确权限与 scope 元数据 |
| WBUI-013 | P0 | 静态 Registry 生成与制品完整性 | 平台、系统、安全 | WBUI-011、WBUI-012 | 生成确定性 Registry，含 release-scope 摘要、版本/制品摘要；功能关闭时不存在对应路由/API/执行入口；设备启动校验版本和完整性；运行时不能由业务服务增加路由 |
| WBUI-014 | P1 | Core/Pack 产品矩阵构建 | 平台、产品 | WBUI-013 | iEMS、DCU 可独立构建；Pack 引用缺失、冲突 override、权限悬空及 release-scope/最终制品不一致均阻断构建 |

### D.3 身份、RBAC 与策略引擎 EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-020 | P0 | 本地账号、登录、首次改密和 Session | 后端、前端、安全 | WBUI-002、WBUI-012 | 登录/登出/改密/会话撤销可用；口令安全存储；限流和错误信息防枚举；会话撤销即时生效 |
| WBUI-021 | P0 | 统一授权策略引擎 | 后端、安全 | WBUI-012 | 实现身份、权限、scope、能力、Entitlement、环境条件合取；默认拒绝；HTTP/API、任务和 WS 调用统一策略接口 |
| WBUI-022 | P0 | 产品资源模型与 scope resolver | 产品、后端 | WBUI-002、WBUI-021 | iEMS/DCU 资源树、对象归属和父子解析明确；客户端不能自行定义 scope；逐对象授权测试通过 |
| WBUI-023 | P0 | 角色、grant、委派和管理员保护 | 后端、安全、前端 | WBUI-021、WBUI-022 | 多角色并集合成、直接授权到期、授予者权限上限、最后管理员保护和审计通过负向测试 |
| WBUI-024 | P0 | Capability/Entitlement 服务接入 | 系统、后端、安全 | WBUI-002、WBUI-021 | 服务端读取可信能力/许可；签名/设备绑定/版本/失效策略按 ADR 实现；前端缓存不参与授权 |
| WBUI-025 | P0 | 权限矩阵自动生成 | 测试、平台、安全 | WBUI-021 至 WBUI-024 | 从权限目录、OpenAPI、Core/Pack 精确角色策略、scope、Capability、Entitlement 生成 allow/deny 用例；明确每个角色的权限集合与资源范围；所有核心 API 有正向/负向覆盖，新增权限默认不授予 |

### D.4 webd、API 与 IPC EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-030 | P0 | TLS/反向代理/Host/Origin 与浏览器安全策略 | 系统、后端、安全 | WBUI-002 | 只有确定的入口暴露 API；代理头可信规则、Host allowlist、Origin/CSRF/WS 检查、CSP/安全响应头/敏感响应 no-store 可通过攻击与浏览器兼容测试 |
| WBUI-031 | P0 | webd OpenAPI 路由与统一错误模型 | 后端、平台 | WBUI-012、WBUI-021、WBUI-030 | 实现核心 operation；错误码/requestId/schema 与 OpenAPI 一致；无未登记路径或匿名端点 |
| WBUI-032 | P0 | IPC transport、peer identity 与方法 ACL | 系统、后端、安全 | WBUI-002、WBUI-030 | IPC 方法目录、peer 验证、socket/bus ACL、消息上限、超时/取消语义实现；非授权本机进程负向测试通过 |
| WBUI-033 | P1 | 审计、限流和敏感字段脱敏 | 后端、安全、系统 | WBUI-031、WBUI-032、WBUI-060 | 高风险操作审计可持久化；具备序号/哈希链完整性检查、保留/空间策略与可选外部锚定；审计故障按风险策略 fail-closed；日志不含密码/Token/私钥 |

### D.5 配置事务与设备恢复 EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-040 | P0 | 配置事务 journal 与状态机 | 后端、系统 | WBUI-002、WBUI-032 | DRAFT/VALIDATING/APPLYING/PENDING_CONFIRM/CONFIRMED/ROLLING_BACK/ROLLBACK_FAILED 状态持久化；重复请求幂等 |
| WBUI-041 | P0 | 独立 Watchdog/Coordinator | 系统、后端 | WBUI-040、WBUI-001 | 不依赖浏览器/webd 请求生命周期；deadline、bootId、启动恢复策略可验证 |
| WBUI-042 | P0 | 配置并发锁、revision 和恢复流程 | 后端、系统 | WBUI-040、WBUI-041 | 并发修改返回 409；kill -9、重启、断电和回滚失败进入预期状态；提供带外恢复说明 |
| WBUI-043 | P1 | 配置事务 UI、差异预览和确认 | 前端、设计、后端 | WBUI-040 至 WBUI-042 | 草稿/校验/应用/倒计时/确认/回滚状态可视；页面关闭不影响事务；错误有恢复指导 |
| WBUI-044 | P1 | R0–R5 风险分类和操作策略 | 产品、后端、安全 | WBUI-040、WBUI-042 | 各配置字段有服务端风险分类、权限、审批/确认和回滚策略；客户端不能降低风险等级 |

### D.6 长任务与实时事件 EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-050 | P1 | 持久化任务执行器 | 后端、系统 | WBUI-032、WBUI-033、WBUI-060 | taskId/状态/阶段/结果持久化；webd/worker 重启后可继续查询；原子 claim/lease 与 fencing token 防止双 worker；定义至少一次恢复、stepId/回执/幂等步骤、下游结果对账和不确定副作用的 RECOVERY_REQUIRED 处理；不得宣称 exactly-once |
| WBUI-051 | P1 | 幂等、互斥组、授权撤销与取消 | 后端、安全、产品 | WBUI-050、WBUI-021 | 相同 key 同摘要返回同任务，不同摘要冲突且活动任务键不可过期；升级/备份/恢复/重启互斥按注册表强制；ownerCancel 与 core.task:cancel 管理员分支按 task-cancel-v1 实施；撤权与取消阶段有明确行为 |
| WBUI-052 | P1 | WebSocket 订阅、持久事件发布与游标恢复 | 后端、前端 | WBUI-021、WBUI-030、WBUI-060 | 仅订阅/退订；topic 绑定权限/scope；关键状态由权威服务先持久化再发布；streamId/partitionId/sequence 跨重启稳定，bootId 仅作诊断；事件去重、补发、订阅撤权、`resync_required` 和 REST 快照一致；跨服务不宣称全局原子提交 |
| WBUI-053 | P1 | WS 队列、连接和慢客户端保护 | 后端、测试 | WBUI-052、WBUI-001 | 连接数/消息/排队字节/速率有上限；过载策略和 drop/gap 指标可观测 |

### D.7 存储、升级和系统集成 EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-060 | P1 | 数据库/文件存储和 Schema 版本策略 | 后端、系统 | WBUI-004 | 权威数据、缓存、事务、任务、审计分区明确；Schema 版本迁移可重复检测 |
| WBUI-061 | P1 | 备份一致性、恢复和磁盘满处理 | 后端、系统、测试 | WBUI-060、WBUI-033 | 数据库与配置一致性备份；恢复校验；磁盘满/写失败测试通过；审计保留规则生效 |
| WBUI-062 | P0（升级功能开放前） | Bootloader/固件升级适配 | Bootloader、硬件、系统、安全 | WBUI-001、ADR-08 | 仅对 release-scope.yaml 明确启用升级的型号/版本强制；按型号选择 A/B 或替代方案，签名/型号/版本/防回退/断电恢复/健康检查测试通过；未启用升级的型号必须证明相关 API、Registry 与执行入口均未暴露 |
| WBUI-063 | P1 | 证书、RTC/NTP 和时间异常处理 | 系统、安全、后端 | WBUI-004、WBUI-060 | 首次信任、更新、过期、时间回拨和无 RTC 策略实现；审计使用 wall-clock 与 monotonic/bootId 信息 |

### D.8 前端 Core 与产品 Pack EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-070 | P1 | App Shell、登录引导与启动流程 | 前端、平台、设计 | WBUI-020、WBUI-013 | 匿名最小系统信息、登录后 auth/capability 初始化、强制改密、401/403/能力缺失状态一致 |
| WBUI-071 | P1 | 菜单/路由视图和动态能力呈现 | 前端、平台 | WBUI-021、WBUI-024、WBUI-070 | 页面由静态 Registry 与授权/能力呈现；直接 URL 和 API 均由后端重新授权 |
| WBUI-072 | P1 | Core 通用页面与 L0–L3 组件 | 前端、设计 | WBUI-070、WBUI-071 | 网络、用户、系统、日志等模块复用 Core 组件；无 Pack 直接依赖/覆盖越界 |
| WBUI-073 | P1 | Schema 表单和导入导出 UI | 前端、后端、设计 | WBUI-040、WBUI-060 | Schema 版本一致；差异预览、逐行错误、异步导出和安全限制在 UI/API 同步呈现 |
| WBUI-074 | P1 | iEMS/DCU Product Pack 实现 | 产品、前端 | WBUI-014、WBUI-022、WBUI-071 | 产品页面、能力/许可要求、API/Schema/权限引用完整；构建期静态注册，不加载任意运行时代码 |
| WBUI-075 | P1 | Design Token、对比度和无障碍 | 设计、前端、测试 | WBUI-072 | 前景/背景配对、深浅主题、键盘、焦点、ARIA 和 200% 缩放验收通过 |

### D.9 安全、测试与发布 EPIC

| 任务 ID | 优先级 | 工作项 | 主责角色 | 前置依赖 | 交付物与验收条件 |
| --- | --- | --- | --- | --- | --- |
| WBUI-080 | P1 | 文件上传/导入/导出安全 | 后端、安全、测试 | WBUI-031、WBUI-060、WBUI-073 | 路径穿越、符号链接、压缩炸弹、恶意 YAML、CSV 公式、配额和超时测试通过 |
| WBUI-081 | P1 | 自动化 CI 门禁和证据归档（分阶段建设） | 平台、测试、安全 | WBUI-003、WBUI-010、WBUI-011、WBUI-012 | 先建立可运行的 G0–G2 基础流水线，再随各模块逐步增加 G1/G3/G4/G5 覆盖；不能等待 WBUI-010～075 全部完成后才开始 CI；测试报告/制品摘要/设备矩阵可追溯 |
| WBUI-082 | P1 | 最低配置真机性能与稳定性测试 | 测试、系统、硬件 | WBUI-001、WBUI-074、WBUI-081 | 首屏、RAM、CPU、P95/P99、WS、长任务、磁盘写入和资源耗尽数据归档 |
| WBUI-083 | P1 | 产品安全验证与渗透测试 | 安全、测试、平台 | WBUI-003、WBUI-081 | 认证、越权、Host/Origin、CSRF、IPC、导入导出和 release-scope 启用功能的攻击面验证；关闭的功能须验证最终制品确实未暴露；严重问题关闭或按规则阻断 |
| WBUI-084 | P1 | 产品发布候选验收 | 产品、平台、系统、测试、安全 | WBUI-081 至 WBUI-083 及所有首发任务 | 发布检查表全部适用项通过；未通过项形成正式风险接受记录且不包含 Blocker/Critical |
| WBUI-085 | P1 | 工程元数据与附录追踪自动生成 | 平台、测试、安全 | WBUI-010、WBUI-011、WBUI-012、WBUI-081 | 权限目录、角色策略、release-scope、任务、需求、ADR 与测试引用由机器可解析源维护；附录 A、D、E 和引用报告自动生成/校验；release-scope 与 OpenAPI/Registry/固件模块不一致、悬空、重复、未追踪项阻断 G0 |

**D.10 任务依赖的关键路径（概念）**

```text
WBUI-001 设备/产品矩阵
   ├─→ WBUI-002 P0 ADR ─→ WBUI-030 TLS/入口 ─→ WBUI-031 API ─→ WBUI-032 IPC
   ├─→ WBUI-010/011 Manifest ─→ WBUI-012 权限/OpenAPI ─→ WBUI-013 Registry
   ├─→ WBUI-021 策略引擎 ─→ WBUI-022 Scope ─→ WBUI-023 RBAC ─→ WBUI-025 授权矩阵
   └─→ WBUI-040 事务 journal ─→ WBUI-041 Watchdog ─→ WBUI-042 故障恢复

上述能力集成后：WBUI-060 存储/事件持久化基线
   ├─→ WBUI-033 审计持久化
   ├─→ WBUI-050 任务执行器
   └─→ WBUI-052 WS 事件日志/outbox 与游标恢复

WBUI-081 CI 基础门禁从 WBUI-003/010/011/012 契约就绪后尽早建立，并随功能逐步扩展
   → WBUI-085 元数据/附录一致性自动检查
   → WBUI-082 真机性能 → WBUI-083 安全验证 → WBUI-084 发布候选
```

> 该图表达关键依赖，不代表所有任务必须串行。可并行工作的前提是输入契约和 ADR 已冻结。

## 附录 E：需求追踪矩阵

需求 ID 是跨设计、任务、测试和发布证据的稳定索引。新增需求不得只追加到正文而不登记；废弃需求必须保留状态和替代关系。

| 需求 ID | 需求/控制目标 | 设计位置 | 实施任务 | 必须验证的测试/证据 | 发布门禁 |
| --- | --- | --- | --- | --- | --- |
| REQ-ARCH-001 | Core/Pack 单向依赖；运行时不增加外部路由 | 4.1–4.4、7.4 | WBUI-010、WBUI-011、WBUI-012、WBUI-013、WBUI-014 | 依赖边界、Manifest 冲突、Registry 摘要/完整性测试 | G0/G2 |
| REQ-AUTH-001 | 所有 API 服务端鉴权；匿名接口显式 allowlist | 5.1–5.2、7.1–7.2 | WBUI-012、020–021、031 | OpenAPI 安全元数据检查、未登录/未授权直接 API 调用 | G0/G2/G3 |
| REQ-AUTH-002 | RBAC 角色合成、授权委派和管理员保护 | 5.3–5.5 | WBUI-021、023、025 | 多角色/直接 grant 到期/提权/最后管理员负向测试 | G1/G3 |
| REQ-AUTH-003 | Resource Scope 服务端解析与对象归属校验 | 5.4–5.5 | WBUI-022、025 | 横向越权、批量对象逐项授权、非法 scope 测试 | G3 |
| REQ-CAP-001 | Capability/Entitlement/Runtime Health 分离 | 4.5、7.2 | WBUI-024、025 | 能力缺失、许可篡改/过期/绑定错误、状态未知测试 | G1/G3 |
| REQ-SEC-001 | Host/Origin/CSRF/代理头/DNS Rebinding 防护 | 3.1、5.6、10.1–10.2 | WBUI-003、030、083 | 恶意 Host、跨站 WS、伪造代理头和 CSRF 测试 | G4 |
| REQ-IPC-001 | IPC peer identity、方法 ACL、消息/超时限制 | 3.1、7.3 | WBUI-032 | 非授权本机进程调用、超长消息、超时/取消测试 | G3/G4 |
| REQ-AUD-001 | 高风险操作审计持久化和失败策略 | 3.1、7.1、10、13 | WBUI-033、061 | 审计分区写满、审计关联 taskId/transactionId、脱敏测试 | G3/G4 |
| REQ-CFG-001 | 配置事务持久化状态机、revision 和幂等 | 6.4、7.2 | WBUI-040、042–044 | 并发编辑 409、重复 apply/confirm/rollback 测试 | G1/G3 |
| REQ-CFG-002 | Watchdog 独立于 webd，支持重启恢复 | 6.4、17、11.2 | WBUI-041–042 | webd kill -9、系统重启、断电、回滚失败演练 | G5 |
| REQ-TASK-001 | 长任务持久化、幂等、互斥、撤权和取消 | 7.5 | WBUI-050–051 | 重复 key、不同请求摘要、并发冲突、重启/取消边界测试 | G3/G5 |
| REQ-WS-001 | WS 只读订阅；关键事件可游标恢复 | 6.3、7.2 | WBUI-052–053 | 订阅越权、断线补发、resync、慢客户端和队列满测试 | G3/G4 |
| REQ-DATA-001 | 数据 Schema 版本、迁移和恢复 | 11.3–11.4 | WBUI-060–061 | 迁移中断、回滚兼容、备份/恢复一致性测试 | G5 |
| REQ-UPD-001 | 升级按型号验证签名、兼容、回退能力 | 10.5、11.3 | WBUI-062 | 错误型号/签名/版本、防回退、断电与健康检查测试 | G5 |
| REQ-TIME-001 | RTC/NTP/时间回拨对 Session/License/证书/任务的影响明确 | 10.4、13 | WBUI-063 | 无 RTC、未同步、回拨/跳变和重启测试 | G3/G5 |
| REQ-FILE-001 | 文件导入导出限制、解析安全和幂等 | 6.5、7.1、10.2 | WBUI-073、080 | 路径穿越、压缩炸弹、恶意 YAML、CSV 公式、配额测试 | G4 |
| REQ-PERF-001 | 前端/分页/查询/WS/任务资源有界 | 6.8、7.1、12 | WBUI-053、082 | 最低配置压力、资源耗尽、P95/P99 报告 | G5 |
| REQ-UX-001 | Design Token 对比度和无障碍 | 9.2–9.11 | WBUI-075 | 前景/背景矩阵、键盘/焦点/ARIA/缩放报告 | G2/G3 |
| REQ-OBS-001 | requestId、streamId、partition sequence、bootId 和诊断信息可追踪 | 6.3、7.1、13 | WBUI-033、050、052、081 | 日志字段/事件游标跨重启连续性/诊断包脱敏和关联测试 | G3 |
| REQ-REL-001 | 最低配置真机矩阵和产品级验收 | 12、15、17 | WBUI-001、082–084 | 具体型号测试报告、缺陷清单和发布签核 | G5 |
| REQ-GOV-001 | 产品/硬件事实、威胁模型和 ADR 决策可追踪 | 14、17、18 | WBUI-001、WBUI-002、WBUI-003、WBUI-004 | 型号矩阵、威胁模型、ADR 记录和评审签核 | G0/G5 |
| REQ-BUILD-001 | Manifest Schema、静态 Registry 和产品矩阵构建可靠 | 4.1–4.4、7.4 | WBUI-010、WBUI-011、WBUI-012、WBUI-013、WBUI-014 | 依赖边界、非法 Manifest、确定性构建、Registry 完整性测试 | G0/G2 |
| REQ-IDENT-001 | 本地身份、密码、首次引导、Session 绝对生命周期和订阅撤销安全 | 5.6、6.1、6.3、7.2 | WBUI-020、070 | 首登改密、登录限流、Session 空闲/绝对超时、撤销即时生效、WS 订阅撤销、账号枚举和切换账号测试 | G1/G3/G4 |
| REQ-ROLE-001 | 角色管理、授权委派、最后管理员与权限矩阵完整 | 5.3–5.5 | WBUI-023、WBUI-025 | 授权者上限、最后管理员保护、自定义角色升级迁移测试 | G1/G3 |
| REQ-API-001 | webd API 与入口安全边界一致 | 3、7.1–7.4 | WBUI-030、WBUI-031、WBUI-032、WBUI-033 | OpenAPI/实现一致性、TLS/代理信任、IPC ACL、审计/限流测试 | G2/G3/G4 |
| REQ-CFG-003 | 配置事务冲突、界面确认和风险策略闭环 | 6.4、6.5、8.6 | WBUI-042、WBUI-043、WBUI-044 | 多管理员冲突、倒计时确认、R0–R5 服务端风险映射、恢复 UI 测试 | G1/G3/G5 |
| REQ-TASK-002 | 长任务的互斥、撤权、取消和恢复策略完整 | 7.5 | WBUI-051 | 各任务阶段取消矩阵、撤权、互斥锁和重启恢复测试 | G3/G5 |
| REQ-STORE-002 | 备份、恢复、空间耗尽和审计存储行为受控 | 11.2–11.4、13 | WBUI-061 | 备份一致性、恢复、磁盘满、审计空间保护测试 | G3/G5 |
| REQ-UI-001 | App Shell、路由与 Core 通用模块遵循平台策略 | 6、8、9 | WBUI-070、WBUI-071、WBUI-072、WBUI-075 | 首屏流程、直接 URL、无权限/无能力状态、组件/无障碍测试 | G2/G3 |
| REQ-PACK-001 | 产品 Pack 通过声明式扩展接入，不产生 Core 分叉 | 4、8 | WBUI-074 | iEMS/DCU Pack 构建、扩展点、权限/能力声明与依赖检查 | G2/G3 |
| REQ-SEC-002 | 上传/导入/导出输入安全和资源限额有效 | 6.5、10.2、11.4 | WBUI-080 | 恶意文件语料、路径边界、解压配额、CSV/XLS 公式注入测试 | G4 |
| REQ-RELEASE-001 | CI、真机、安全验证与发布签核形成完整证据链 | 12、15、18、附录 F | WBUI-081、WBUI-082、WBUI-083、WBUI-084 | 所有门禁报告、真机数据、渗透报告、发布检查表和风险签核 | G0–G5 |
| REQ-EVENT-002 | 关键事件在权威服务持久化后发布；游标跨重启连续；不可恢复时重同步；授权撤销后订阅失效 | 6.3、7.5、13 | WBUI-052、WBUI-060、WBUI-081 | 状态与 outbox 事务关系、跨服务发布、outbox 重放、重复事件去重、bootId 变化、streamId 改变、订阅撤权和 cursor gap 测试 | G1/G3/G5 |
| REQ-TASK-003 | 任务取消具备 owner/admin 两条明确定义的授权分支、scope 和阶段语义；执行器单实例 claim；不确定副作用不盲目重放 | 5.2、7.2、7.5 | WBUI-021、WBUI-025、WBUI-050、WBUI-051 | owner/admin 任务取消、权限撤销、双 worker 竞争/fencing、各阶段取消边界、下游回执缺失与 RECOVERY_REQUIRED 测试 | G1/G3/G5 |
| REQ-AUD-002 | 审计具备可验证完整性、保留/容量策略和明确的本地防篡改能力边界 | 10.2、11.2、11.4、13 | WBUI-033、WBUI-060、WBUI-061 | 审计篡改/重排检测、分区写满、轮转、外部摘要锚定和 root 威胁边界评审 | G3/G4/G5 |
| REQ-ROLE-002 | Core 与 Pack 的角色权限集合/资源范围明确且可追踪；新增权限默认不授予 | 5.3–5.5、附录 A | WBUI-023、WBUI-025、WBUI-085 | Core/Pack 角色矩阵、scope 合成、权限新增迁移、自我提权负向测试 | G0/G1/G3 |
| REQ-GOV-002 | 权限、角色、release-scope、任务、需求、ADR 与附录引用具备机器可检查的单一事实来源 | 14、17、附录 A、D、E、G | WBUI-085 | 生成文件可重复、release-scope 与制品功能开关一致、ID 唯一、引用全解析、任务/需求双向追踪检查 | G0/G2 |

**追踪完整性规则**：附录 D 的每个 WBUI 任务 ID 必须至少出现在本矩阵一行；每条首发需求必须反向关联至少一个任务和测试用例。CI 可以据项目内机器可读版本自动检查任务引用完整性；本表是文档视图，不作为替代数据源。

## 附录 F：CI/CD 门禁与发布验收

### F.1 自动化门禁

| 门禁 | 阶段 | 检查内容 | 阻断条件 | 证据产物 |
| --- | --- | --- | --- | --- |
| G0 契约/静态检查 | 每次提交 | lint、类型、依赖边界、YAML/JSON Schema、权限/角色 ID、API 路径、安全元数据、任务/需求/ADR 引用、Registry 与附录可重复生成 | 悬空权限/能力/角色/任务/需求引用、未声明匿名接口、重复 ID、动态路由输入、API 前缀不一致、生成产物不一致、任务无测试映射 | lint 报告、Manifest/角色校验报告、Registry 摘要、权限与追踪引用报告 |
| G1 单元/组件测试 | 每次提交 | 授权引擎、scope、角色矩阵、能力、Schema、事务/task 状态机、幂等步骤、事件 outbox、前端组件 | 核心策略负向用例失败；状态机非法迁移未拒绝；取消策略/幂等步骤/事件持久化测试失败；关键逻辑无测试 | 单测报告、覆盖率报告、策略决策样例 |
| G2 构建/契约测试 | 每个产品构建 | iEMS/DCU 产物、OpenAPI、Mock、静态 Registry、release-scope 与页面/权限注册 | operation 缺少安全元数据；release-scope 与最终路由/API/Registry 不一致；Mock/实现 schema 不一致；Pack 引用悬空 | 产品构建报告、OpenAPI diff、契约报告、包体积报告 |
| G3 集成/E2E 测试 | 合并主干/候选版本 | 登录/Session 绝对期限、角色/scope、配置、任务取消/恢复、WS outbox/订阅撤权、IPC、审计和用户流程 | 关键业务或越权测试失败；服务重启后状态/关键事件丢失；任务步骤可能重复造成危险副作用；错误码/任务语义不一致 | E2E 报告、IPC 测试日志、任务/事务恢复报告、事件连续性报告 |
| G4 安全验证 | 合并主干/候选版本 | SAST、依赖/CVE、SBOM、CSP/安全响应头、CSRF/Host/Origin、上传、注入、权限负向、审计完整性和基础渗透 | Blocker/Critical；未批准的 High；密钥泄露；安全边界可绕过；审计完整性校验失效 | SAST/依赖报告、SBOM、HTTP 安全头报告、审计完整性报告、渗透报告、风险接受记录 |
| G5 目标设备验证 | 产品候选发布 | 最低配置型号、冷启动、RAM/CPU、P95/P99、断电、watchdog、release-scope 启用的升级/恢复能力、磁盘满、真实 IPC/驱动 | 任一适用的 P0/P1 验收失败；无目标设备证据；无法恢复或数据完整性不明 | 设备型号/版本记录、测试脚本、原始日志、性能报告、恢复演练报告 |

### F.2 不可豁免的发布条件

1. Blocker/Critical 安全、授权、数据完整性以及**已在该 release-scope 启用**功能的恢复缺陷必须关闭。
2. 首发范围内所有 P0 任务已“已验证”；所有适用 P1 任务已完成并通过所需设备级验证。P1 的定义仍是“首个产品发布必需”；若功能从首发清单中移除，必须同步删除其 UI/API/Registry 暴露并更新需求矩阵，不能仅把任务延期。
3. 需求追踪矩阵每条首发需求均可关联设计位置、任务、测试用例和证据文件。
4. 构建产物、静态 Registry、OpenAPI、权限目录、SBOM 和固件版本/摘要一致并归档。
5. 最低配置真机性能达到经 ADR 冻结的预算；若预算尚未冻结，产品发布门禁不通过。
6. 配置事务、备份恢复、证书/时间、审计和 IPC 的关键故障恢复演练有明确结果；固件升级恢复演练仅在该型号/版本开放升级功能时强制，关闭时须有 API/Registry/执行入口未暴露的证据。
7. 所有残余风险均有风险接受记录；风险接受不得用于豁免 Blocker/Critical 或代替硬件恢复验证。

### F.3 发布候选检查清单

- [ ] 产品型号、硬件版本、Bootloader、分区布局、固件与 WebUI 版本已经记录。
- [ ] P0 ADR 已接受；所有首发范围 ADR 已关闭或明确标记为不适用。
- [ ] 构建/依赖/权限/API/Schema/Registry 门禁通过。
- [ ] 认证、Session、RBAC、scope、Capability、Entitlement 的允许与拒绝用例通过。
- [ ] TLS/Host/Origin/CSRF/WebSocket/IPC 安全测试通过。
- [ ] 配置事务 kill -9、重启、断电和回滚失败验证通过。
- [ ] 长任务幂等、互斥、撤权、取消权限/阶段边界、重启恢复和不确定副作用处理测试通过。
- [ ] WebSocket 关键事件持久化后发布、断线补发、快照重同步、订阅撤权和过载行为验证通过。
- [ ] 数据库迁移、备份下载/恢复、磁盘满、审计写失败和审计完整性策略验证通过。
- [ ] 若该型号/版本在 release-scope 启用升级：固件升级签名、型号匹配、防回退、断电恢复和健康检查通过；若关闭升级：确认最终 API、Registry、UI 和设备执行入口均未暴露。
- [ ] 文件上传/导入/导出安全、分页查询上限和资源配额通过。
- [ ] 最低配置真机性能、安全、兼容性和无障碍报告归档。
- [ ] 运行手册、现场恢复手册、日志/告警、支持边界和已知问题发布说明齐备。
- [ ] 风险接受和发布签核由规定责任角色完成。

## 附录 G：ADR 记录模板

每条 ADR 使用独立文件或项目管理记录，至少包括：

```text
ADR ID / 标题：
状态：待提出 | 评审中 | 已接受 | 实施中 | 已验证 | 已拒绝 | 被替代
提出日期 / 评审日期 / 决策日期：
提出人 / 责任团队 / 决策角色：
涉及需求 ID：
涉及工程任务 ID：
问题背景与必须解决的约束：
不可改变的项目事实（来源/证据）：
候选方案 A：优点 / 缺点 / 资源 / 安全 / 运维 / 兼容影响
候选方案 B：优点 / 缺点 / 资源 / 安全 / 运维 / 兼容影响
其他备选方案及排除原因：
决策结果与理由：
接受的代价和剩余风险：
安全/产品/系统/硬件评审意见：
实施计划与依赖任务：
验证计划、目标设备和通过标准：
回退/迁移方案：
证据位置：
被替代 ADR（如有）：
```

---

## 附录 H：v1.4 评审意见与实施建议（历史记录）

> 本附录记录 v1.4 发布时追加的评审意见。其“正文未改”仅描述 v1.4 当时的修订范围，不代表后续版本仍保持正文未改。v1.5 已将当时采纳项回填；v1.6 再次收口后，本附录不具有当前规范效力，仅用于历史追溯。

### H.1 总体评价

1. **架构边界已有清晰基线**：Core + Product Pack + 静态 Registry + 构建期裁剪，适合资源受限、固件整包升级场景；运行时禁止任意插件注册是当前合理的安全取舍，仍需通过产品矩阵和真机验证确认边界完整。
2. **安全与授权设计覆盖较广**：页面显隐不构成安全边界；权限、资源范围、Capability、Entitlement、Runtime Health 严格分离；配置事务独立 Watchdog、fail-closed、幂等与审计失败策略已有设计要求，但是否有效仍须由目标设备实现、故障注入和安全测试证明。
3. **工程治理已有基线**：ADR 生命周期、任务 ID（WBUI-xxx）、需求追踪矩阵（REQ-xxx）、CI 门禁（G0–G5）、Definition of Ready/Done 已形成较完整框架；仍需依靠机器可检查的权威源、角色授权矩阵和真机证据闭环，不能仅凭文档结构认定治理已完成。
4. **风险意识强**：明确“待决事项”“不可用假设替代事实”，并要求真机证据，避免设计文档沦为纸上合规。

### H.2 文档可维护性建议

| 建议 ID | 建议内容 | 建议落地方式 | 优先级 |
| --- | --- | --- | --- |
| H-DOC-01 | 单文件过长，后续变更与评审成本高 | 将主设计（架构+核心决策）与附录（权限目录、任务、需求矩阵、CI、ADR 模板）拆为独立文件或目录；主文档保留交叉引用 | P2 |
| H-DOC-02 | 章节决策意图不够“一眼可见” | 各主要章节开头增加 3–5 行决策摘要卡片：为什么这样设计 / 关键约束 / 当前状态（已冻结/待 ADR） | P2 |
| H-DOC-03 | 附录与正文双源维护风险 | 权限目录、任务表、需求矩阵以机器可读文件为权威源，附录由生成结果渲染；CI 校验一致性 | P1 |

### H.3 实施优先级与首发范围建议（v1.4 原建议的修正说明）

v1.4 原建议将部分 P1 功能列为“首发后 1–2 个版本”，与正文 §14.1、§17.1 及附录 F.2 的定义冲突：这些位置均将适用 P1 定义为首个产品发布必需。因此，该排期建议在 v1.5 中撤销，不再作为有效发布范围依据。

| 范围 | 应遵循的基线 | 说明 |
| --- | --- | --- |
| 架构/安全前置（P0） | WBUI-001～003、010～013、020～025、030～032、040～042；WBUI-062 在升级功能上线前必须完成 | 先冻结设备事实、P0 ADR、信任边界、策略引擎和配置恢复；不能绕过目标设备验证 |
| 首个产品发布（全部适用 P0 + P1） | 依首发产品实际开放的页面/API/能力，完成附录 D 中全部适用 P1 任务、附录 E 对应需求及附录 F 门禁 | 包含适用的持久任务、WS 事件连续性、存储迁移、证书/时间、文件安全、无障碍和真机/安全验证；不能把适用 P1 延后到发布后 |
| 条件性功能 | 仅当产品固件开放对应功能时触发相应任务/ADR/验证，例如升级、Service 远程维护、特定网络接口 | 未完成条件性门禁时，应从构建 Registry 和 API 暴露中移除该功能，而非仅在 UI 隐藏 |
| 后续增强（P2） | LDAP/RADIUS、TOTP、深度白标、审计外发增强等 | 只有在单独 ADR、威胁模型、兼容和资源评估完成后进入产品范围；第三方运行时代码/热插拔不属于当前范围，不能仅作为普通 P2 开关启用 |

**产品顺序**：路线图将 iEMS 作为 M3 首个产品、DCU 作为 M4 复用验证产品；这表示 DCU Pack 可在第二产品阶段交付，不表示首个 iEMS 发布可以省略任何适用 P1 平台能力。若产品管理决定更改首发产品或缩小首发功能范围，必须同步更新 ADR、Pack 构建清单、需求追踪矩阵、任务优先级和发布检查表。

**建议动作**：项目管理系统应增加“首发是否启用此功能”和“是否阻塞本产品发布”两个字段；该字段必须与产品 Manifest / OpenAPI / Registry / REQ 矩阵一致。

### H.4 配置事务与 Watchdog 细化建议

| 建议 ID | 细化点 | 建议 |
| --- | --- | --- |
| H-CFG-01 | 确认窗口 | 60 秒仅作初始值；按配置类别（网络、防火墙、证书等）给出不同默认窗口；真机验证须覆盖改 IP 后浏览器断连、手机热点、有线切换等场景 |
| H-CFG-02 | 带外恢复 | 串口/物理按键等恢复路径写成可执行运维手册（步骤、前置条件、回滚后校验），不仅作为设计要求 |
| H-CFG-03 | 故障注入时机 | 在 M0/M1 即开展 `webd kill -9`、系统重启、断电、回滚失败演练，不放到发布前集中补测 |
| H-CFG-04 | 客户端风险声明 | 服务端按 Schema/section 解析风险等级；客户端自行声明的 risk 一律忽略 |

对应 ADR：ADR-05；对应任务：WBUI-040～044；对应需求：REQ-CFG-001/002/003。

### H.5 Capability / Entitlement 与授权链细化

| 建议 ID | 细化点 | 建议 |
| --- | --- | --- |
| H-CAP-01 | 能力快照来源 | 明确由哪个可信进程生成、是否签名/带摘要、启动时如何校验；禁止前端缓存参与授权 |
| H-CAP-02 | revision 语义 | `capabilityRevision` 变化时前端仅刷新呈现；写 API 必须服务端重验；未知/服务不可用时写操作 fail-closed |
| H-CAP-03 | 与 scope 联测 | Capability/Entitlement 不得在权限策略之外单独做客户端判断；与 ADR-09 资源模型一并做 API×scope×capability 负向测试 |

对应 ADR：ADR-04、ADR-09；任务：WBUI-024、025、022。

### H.6 WebSocket、长任务与状态一致性

| 建议 ID | 细化点 | 建议 |
| --- | --- | --- |
| H-WS-01 | 写库先于推送 | 关键状态变更先持久化再推事件，避免“推了但库未写”的竞态 |
| H-WS-02 | 任务权威源 | 任务状态以持久化任务 API 为准；WS 仅通知；重连后 gap/`resync_required` 必须可观测 |
| H-TASK-01 | 互斥与撤权 | 升级/备份/恢复出厂/冲突网络事务的互斥组在任务类型注册表声明并由执行器强制；撤权后未开始任务不得执行，执行中按阶段策略安全停止 |

对应任务：WBUI-050～053；需求：REQ-WS-001、REQ-TASK-001/002。

### H.7 性能与资源预算

| 建议 ID | 细化点 | 建议 |
| --- | --- | --- |
| H-PERF-01 | 体积与运行时 | 首屏 gzip ≤ 300KB 为预算目标；同步给出最低配置设备 RAM 峰值、冷启动时间、P95/P99 基线，避免“体积达标但运行卡顿” |
| H-PERF-02 | 图表选型 | ECharts vs uPlot 等通过真机点数/内存压测后写入 ADR 附件再冻结 |
| H-PERF-03 | 查询与 WS 上限 | 分页、时间跨度、采样点、WS 连接/主题/队列字节数必须有服务端硬上限；超限返回明确错误并计量 |

对应任务：WBUI-053、082；需求：REQ-PERF-001。

### H.8 权限目录与角色迁移

| 建议 ID | 细化点 | 建议 |
| --- | --- | --- |
| H-PERM-01 | 新增权限 | 必须同步更新角色 preset 迁移脚本与变更报告；自定义角色默认不授予新权限 |
| H-PERM-02 | 废弃权限 | 至少一个主版本 deprecation 周期；构建期对废弃引用给出警告，最终移除前阻断未迁移引用 |
| H-PERM-03 | 矩阵自动化 | 从权限目录 + OpenAPI + 角色 + scope + Capability 生成 allow/deny 用例；每次 PR 跑负向用例，禁止只测“能登录” |

对应任务：WBUI-012、023、025；需求：REQ-AUTH-002、REQ-ROLE-001。

### H.9 现场可用性与调试页

| 建议 ID | 细化点 | 建议 |
| --- | --- | --- |
| H-UX-01 | 首次引导/改密 | 触控友好；复杂配置页给出“建议使用电脑”提示时机；强制改密路径全键盘可达 |
| H-UX-02 | 断连体验 | 断连常驻横幅并禁用写操作；明确已打开配置页脏数据策略（允许本地草稿继续编辑或强制只读） |
| H-DBG-01 | 通道租约 | 租约被强制释放时的用户提示文案；采集恢复后的状态展示（调试中/采集中/未知） |
| H-DBG-02 | 串口终端 | 最大缓存条数、帧尺寸、带宽与导出限制；防止低配设备 OOM |
| H-DBG-03 | 控制类写 | 写寄存器/线圈/遥控等二次确认 + 精确权限 + 资源范围重验 + 审计摘要（不记录敏感载荷） |

### H.10 测试与证据前置建议

| 建议 ID | 细化点 | 建议 |
| --- | --- | --- |
| H-TEST-01 | 黄金测试机 | 尽早锁定 1–2 款最低配置代表型号；硬件未到位前仅允许标记“软件实现完成 / 真机待验证” |
| H-TEST-02 | 故障注入前置 | kill -9、断电、时钟回拨、磁盘满、审计写满在 M1/M2 即纳入，不集中在发布前 |
| H-TEST-03 | 视觉与无障碍 | 前景/背景配对矩阵 CI；深色主题告警/离线专项；登录→改密→网络→升级主路径键盘覆盖 |

对应门禁：G3–G5；任务：WBUI-081～084。

### H.11 合规与对外表述

| 建议 ID | 细化点 | 建议 |
| --- | --- | --- |
| H-COMP-01 | 设计 ≠ 认证 | 对外材料明确区分“平台设计满足某类控制目标”与“特定产品在某市场已完成合规评估” |
| H-COMP-02 | 证据引用 | 避免在产品手册中直接引用本设计文档章节号作为合规证据；以适用性判断（ADR-11）与证据计划为准 |

### H.12 建议的近期行动清单（可直接排期）

**近期（按项目资源和设备到位情况排期）**

1. 完成 WBUI-001（产品/型号/硬件矩阵），锁定最低配置代表机型。  
2. 关闭/推进 ADR-01、02、03、05、09、10（部署与 TLS、IPC、硬件基线、配置事务、资源模型、现场网络）。  
3. 任务导入项目管理系统，增加「是否阻塞本产品发布」和「首发功能是否启用」标记。

**中期（M1–M2）**

1. 打通静态 Registry + 授权策略引擎 + 配置事务 Watchdog（含 kill -9 / 重启演练）。  
2. 建立权限/角色矩阵、任务/需求追踪和附录一致性自动生成/负向测试流水线。  
3. 最低配置真机：首屏体积、RAM、冷启动、P95/P99 基线归档。

**长期**

1. 文档模块化维护（H-DOC-01/03）。  
2. 视觉对比度与无障碍 CI 落地（H-TEST-03）。  
3. 按首发反馈决定 P2 增强是否进入主线。

### H.13 本附录与现有体系的关系

- 上述“不修改 v1.3 正文、附录 A–G”仅描述 v1.4 的修订边界；v1.5 已将选定建议纳入正文/附录 A–G。
- v1.5 采纳状态、对应任务和测试以附录 I、附录 D–F 为准；未采纳项仍需 ADR/责任方确认。
- 建议 ID（H-xxx）仅用于 v1.4 历史意见追溯；工程任务仍以 WBUI-xxx、需求以 REQ-xxx、门禁以 G0–G5 为准。

### H.14 使用说明（v1.4）

1. 本条描述 v1.4 发布时的使用方式；v1.5 发布后，以 v1.5 正文、附录 A–I、已接受 ADR 和真机证据为准。
2. 本附录是历史评审输入，不单独产生强制需求。
3. v1.4 附录 H 与 v1.5 正文/附录 A–G 冲突时，以 v1.5 为准；后续决策仍须通过 ADR 正式记录。

---

## v1.4 修订记录与使用说明（历史记录）

本版在 **不修改 v1.3 正文与附录 A–G** 的前提下，仅做以下变更：

1. 文档版本号更新为 v1.4；文档状态说明中注明“附录追加评审意见与实施建议”。  
2. **新增附录 H：v1.4 评审意见与实施建议**，内容包括总体评价、文档可维护性、MVP 边界、配置事务/Watchdog、Capability 与授权链、WebSocket/长任务、性能预算、权限目录、现场可用性与调试页、测试证据前置、合规表述、近期行动清单及与现有体系的关系说明。  
3. 保留完整的 v1.3 修订记录（见下），便于历史追溯。

### v1.3 修订记录（原文保留）

本版继承 v1.2 全文设计基线，重点增加：

1. 第 14 章增加工程任务追踪、Definition of Ready/Done 和接口变更控制；  
2. 第 15 章增加阶段进入/退出门槛，强调计划周期不替代证据；  
3. 第 17 章增加 ADR 状态生命周期、14 项决策工作包、依赖顺序和阻塞规则；  
4. 新增第 18 章工程实施与交付治理；  
5. 新增附录 D 任务分解、附录 E 需求追踪矩阵、附录 F CI/CD 门禁及发布检查表、附录 G ADR 记录模板；  
6. 修正 v1.2 第 5.1 节重复的页面级授权行。

**使用方式**：先完成 WBUI-001 产品/硬件矩阵，再处理 P0 ADR；同时可开展不依赖未决决策的 Manifest/权限目录工具化工作。按附录 D 将任务导入项目管理系统，按附录 E 建立需求追踪，按附录 F 建立阻断式 CI 门禁。只有在附录 F 的适用门禁通过、目标设备证据归档后，才能将对应功能标记为产品发布就绪。评审改进意见见 **附录 H**，采纳前须确认并不替代 ADR 与门禁。

**文档边界**：v1.4 与 v1.3 相同——提供工程执行框架和可验收任务，不代表任务已分配、实现已完成或真机已通过。任务负责人、日期、状态、具体硬件数值、技术选型和 ADR 结论必须由项目团队补录；文档中未给出的事实保持待确认状态。附录 H 为建议性内容，不自动提升为强制需求。



---

## 附录 I：v1.5 第二轮审核、问题闭环与一致性结果（历史记录）

> 本附录记录 v1.5 发布时的审核结果。其静态统计与结论仅对 v1.5 当时的文件内容负责；v1.6 当前检查结果和规范解释以附录 J 为准。

### I.1 审核范围与结论

本轮按“正文架构与安全边界、API/权限映射、事务与任务恢复、事件连续性、工程依赖、需求追踪、发布门禁、文档可维护性”复核 v1.4。对照 v1.3 逐行比较后，确认 v1.4 对原文档的差异仅为版本头、文档状态/使用说明和新增附录 H；正文及附录 A–G 未变更，与 v1.4 的变更声明一致。

v1.4 已有较强的结构化设计基础，但附录 H 的首发范围建议与正文的 P1 定义冲突；另有事件持久化与推送之间的一致性缺口、任务取消授权不够精确、任务持久化依赖未显式表达、CI 基础设施依赖设置过晚等问题。v1.5 已将这些问题回填对应正文与附录，而不是仅追加建议。

### I.2 本轮问题清单与处理结果

| ID | 级别 | v1.4 问题 | v1.5 处理位置 | 结果 |
| --- | --- | --- | --- | --- |
| I-01 | P0 | 附录 H.3 把多项 P1 延后首发后，与 §14.1、§17.1、附录 F.2 冲突；还将 WBUI-062 列入后续项，但其为升级上线前 P0 | §14.1、§17.1、附录 H.3、F.2、I | 已统一：适用 P1 属首个产品发布要求；升级功能开放前必须完成 WBUI-062/ADR-08 |
| I-02 | P1 | WBUI-081 依赖 WBUI-010 至 WBUI-075，意味着 CI 可能要等被它验证的模块全部开发完才启动 | 附录 D.9、D.10、F.1 | 已改为尽早建立 G0–G2，再逐步扩展 G1/G3/G4/G5 |
| I-03 | P1 | 关键事件有持久化游标要求，但没有明确权威状态写入与事件推送的原子/恢复关系 | §6.3、附录 D、E、F | 已加入事务事件表/outbox/持久事件日志、先持久化后发布、重复投递去重与缺口重同步 |
| I-04 | P1 | 任务取消仅写“创建者或有对应执行权限”，未定义跨用户取消的独立权限和 scope | 附录 A、§7.2、§7.5、附录 D–F | 已增加 `core.task:cancel`；任务创建者与管理员路径分开，必须检查当前身份、scope 和任务阶段 |
| I-05 | P1 | 长任务持久化执行器未显式依赖数据库/存储策略；危险步骤恢复语义易被误解为 exactly-once | §7.5、附录 D.6/D.7/D.10、E/F | 已增加存储依赖、stepId/回执/幂等步骤和 `RECOVERY_REQUIRED`；明确不承诺 exactly-once |
| I-06 | P1 | 备份 API 清单缺少显式下载/恢复端点和下载授权规则 | §7.2、附录 F | 已补充下载/恢复 API、权限、完整性/版本检查和短时下载授权要求 |
| I-07 | P1 | 审计完整性要求较抽象，容易被理解为本地记录绝对不可篡改 | §10.2、§13、附录 E/F | 已明确序号/哈希链/外部锚定选项，并说明不能抵御已取得 root 权限者重写全部本地数据 |
| I-08 | P1 | Core 角色与 Pack 角色的精确权限集合缺少统一、机器可读的权威来源 | 附录 A、WBUI-025、REQ-ROLE-002 | 已规定独立版本化角色策略源，缺省拒绝和新增权限默认不授予 |
| I-09 | P1 | Session 只有空闲超时，没有绝对生命周期上限；WS 订阅与权限撤销的即时关系不够明确 | §5.6、§6.3、附录 E/F | 已增加绝对生命周期要求与订阅主动撤销；具体时长仍由 ADR 冻结 |
| I-10 | P1 | v1.4 文档只有 43 个任务和 32 个需求，附录/索引仍需人工维护，长期可能漂移 | 附录 D/E/F、WBUI-085、REQ-GOV-002 | 已增加机器可解析权威源、附录自动生成和引用一致性阻断任务 |
| I-11 | P2 | H.12 的 1–2 周行动时间无团队容量/硬件到位依据 | 附录 H.12 | 已删除无依据的日历承诺，改为按资源和设备条件排期 |
| I-12 | P2 | v1.4 的总体评价把“设计覆盖较完整”表述成可能被误解为“安全体系已完成” | 附录 H.1 | 已调整为架构/治理已有基线，明确仍需 ADR、实现和真机证据 |

### I.3 权限、任务与需求追踪的静态检查

- v1.4 附录 A 原有 54 个权限 ID，正文和附录引用均可解析；v1.5 新增 `core.task:cancel`，权限目录合计 55 个 ID。
- v1.4 附录 D 原有 43 个任务 ID，均在附录 E 中出现；v1.5 新增 WBUI-085，用于元数据/追踪附录一致性自动检查，任务合计 44 个。
- v1.4 附录 E 原有 32 个需求 ID，无重复定义；v1.5 新增 REQ-EVENT-002、REQ-TASK-003、REQ-AUD-002、REQ-ROLE-002、REQ-GOV-002，需求合计 37 个。
- v1.5 新增任务/需求均具备明确引用；附录 D 的任务仍需在实际项目中补充个人负责人、计划日期、状态和证据地址。
- 本检查是文档层静态引用核验，不代表 OpenAPI、实际权限实现、代码测试或真机行为已通过。

### I.4 发布范围的唯一解释

1. §14.1 的优先级定义、§17.1 的设计冻结/首发要求、附录 D 的任务优先级、附录 E 的需求门禁和附录 F 的发布条件共同构成规范基线。
2. P0 是架构冻结或首个产品安全运行必需；P1 是首个产品发布必需；P2 是经 ADR 评审后可选的后续增强。
3. 任何“条件性任务”只有在该产品构建与 API/Registry 中不开放对应功能时，才可不阻塞该产品发布。仅隐藏菜单不构成移除功能。
4. iEMS 是路线图中的首个产品，DCU 是第二产品复用验证。DCU 的后续交付不允许削弱 iEMS 首发所需的平台 P0/P1 门禁。
5. 若确需更改发布范围，必须同时更新产品 Manifest、OpenAPI、需求矩阵、任务表和发布检查表，并经产品、安全、平台责任方签核。

### I.5 尚未关闭的事项

以下项目不能仅通过文档编辑关闭，仍必须按现有 ADR/设备事实完成：

- ADR-01～ADR-14 的最终选型、决策人、日期、方案权衡和验收证据；
- 最低目标型号的 RAM/Flash/CPU、存储介质、RTC、网口、Bootloader/分区及浏览器矩阵；
- webd 语言/部署方式、TLS 终止点、IPC transport/peer ACL、任务执行器进程边界；
- 许可证信任根、设备绑定、密钥轮换、时钟回拨策略；
- 数据库及事务 journal 的具体存储、掉电原子性、审计外部锚定能力；
- 任务类型的逐阶段互斥/取消/恢复矩阵，配置 R0–R5 的产品级风险映射；
- 产品市场/法规适用性书面判断，以及目标硬件上的安全、故障注入和性能证据。

**设计状态**：v1.5 当时可作为下一轮 ADR 评审和工程任务导入基线；该结论已由 v1.6 当前基线更新，不能据此宣称系统已实现、已通过产品级安全测试或已完成合规认证。

---

## v1.5 修订记录与使用说明（历史记录）

以下仅记录 v1.5 当时的修订内容；当前规范与使用方式以 v1.6 正文和附录 J 为准。

本版基于 v1.4 完成第二轮审核整合，主要变更如下：

1. 更新版本号为 v1.5；明确本版对正文和附录 A–G 做了实质性回填，附录 H 仅保留 v1.4 历史评审意见。
2. 统一 P0/P1/P2 与首发发布边界，修正附录 H.3 将 P1/升级上线前任务不当延期的冲突。
3. 明确 WebSocket 关键事件先持久化后发布、outbox/事件日志恢复、订阅撤权和游标语义。
4. 增加任务取消权限 `core.task:cancel`，明确 owner/admin 两条授权路径；增加任务步骤幂等、回执和 `RECOVERY_REQUIRED`，不承诺 exactly-once。
5. 补充备份下载/恢复 API、Session 绝对生命周期、HTTP 安全头/敏感响应缓存策略和审计完整性边界。
6. 修正 WBUI-081 CI 任务依赖，使 CI 可早期建立、逐步扩展；补充任务存储依赖和关键路径。
7. 增加 WBUI-085 与 REQ-GOV-002，推动权限/角色/任务/需求/ADR 索引及附录机器生成/一致性检查。
8. 增补事件、任务取消、审计完整性、角色矩阵和治理追踪的需求与测试用例。

**使用方式**：以 v1.5 正文、附录 A–I 和已接受 ADR 作为设计基线；先完成 WBUI-001 与首发 P0 ADR，再按依赖图实施。P1 仍表示首个产品发布必需，不能因项目排期而自动延后。任何缩小首发功能范围的决定，都必须同步修改构建 Manifest、OpenAPI、需求矩阵和发布清单。

**文档边界**：本版完成的是设计审查和文档级闭环，不表示新增功能已编码、权限实现已通过、IPC/Bootloader 已验证、真机故障演练已完成或产品获得任何安全认证。项目团队仍须填写 ADR 决策、责任人、硬件事实、实际测试证据和风险接受记录。


---

## 附录 J：v1.6 最终复核、规范解释与设计冻结记录

### J.1 本版定位与规范优先级

v1.6 是本设计文档的最终设计基线版本。这里的“最终”表示当前文档版本完成了正文、权限、接口、任务、需求追踪和发布门禁的统一；不代表实现、设备适配、安全测试或合规认证完成。

当前规范的解释顺序如下：

1. **本产品/版本经批准的 `release-scope.yaml`**：只用于决定条件性功能是否纳入某个具体产品构建，不能降低通用安全基线。关闭功能必须同时从页面/路由、OpenAPI operation、静态 Registry、固件执行入口和发布测试范围中移除。
2. **已接受的 ADR**：用于冻结本文明确留待决策的技术选项和设备差异；不得以口头决定覆盖强制安全要求。若 ADR 需要变更规范性要求，必须同步修订正文、需求、任务和测试映射，并发布新版本。
3. **v1.6 正文第 1–18 章、附录 A–G 与本附录 J**：当前规范性设计基线。
4. **附录 H（v1.4）与附录 I（v1.5）**：仅作为历史评审记录，不是当前规范源；冲突时不具有覆盖效力。

### J.2 最终收口的关键问题

| ID | 问题 | v1.6 最终处理 | 关联位置 |
| --- | --- | --- | --- |
| J-01 | 条件性升级能力可能导致 ADR-08/WBUI-062 被误认为所有型号的首发阻塞项 | WBUI-002 必须完成每型号升级适用性决策；只有 release-scope 启用升级时，WBUI-062 才是该型号上线前强制门禁；关闭时必须从 API/Registry/执行入口移除 | §14.1、§17.1、WBUI-002、WBUI-062、附录 F |
| J-02 | WebSocket 游标把 bootId 放入主键，难以定义设备重启后的持续恢复 | 游标改为持久 streamId + partitionId + sequence；bootId 作为诊断元数据；存储重建或游标失效时 resync | §6.3、WBUI-052、REQ-EVENT-002 |
| J-03 | 跨服务事件一致性容易被误读为全局数据库原子事务 | 明确每个权威服务在自身事务内记录 outbox/事件日志；异步发布；不承诺跨服务全局原子提交 | §6.3、WBUI-052 |
| J-04 | 同一持久任务可能由多个 worker 并发恢复 | 增加原子 claim/lease、fencing token、旧 worker 提交阻断和下游操作结果对账 | §7.5、WBUI-050、REQ-TASK-003 |
| J-05 | 任务取消“owner 或 admin”可能被误实现成无边界 OR | 定义 task-cancel-v1 的 owner 自取消和管理员跨用户取消两条独立授权分支，并要求 OpenAPI 元数据声明策略名 | §7.2、§7.5、WBUI-051、T-18 |
| J-06 | 备份清单 API 不完整，升级状态 API 与通用 task API 重复 | 增加备份列表接口；异步恢复/升级统一返回 taskId 并使用 `/api/v1/tasks` 查询状态 | §7.2 |
| J-07 | “最终版”容易被误读为设备交付完成 | 明确文档冻结、ADR 关闭、软件实现、集成验证、目标设备验证和发布批准是不同状态 | §14、§17、J.4 |

### J.3 静态一致性复核清单

本次文件级复核针对 v1.6 文档本身执行以下检查。实际工程仓库仍须由 WBUI-085 以机器可解析事实源重复执行：

- [x] 权限 ID：唯一、被引用的 ID 均已定义；未登记权限不得进入 Manifest/OpenAPI/角色策略。
- [x] 任务 ID：唯一；每项任务具备依赖、验收条件、至少一个需求追踪和验证证据映射。
- [x] 需求 ID：唯一；每条首发需求都映射到设计、任务、测试和门禁。
- [x] 任务依赖：所有前置任务 ID 存在，不存在依赖环；条件性任务标记清楚。
- [x] API：所有 HTTP 路径统一 `/api/v1`；写操作有权限、scope、幂等和审计定义；匿名接口仅来自 allowlist。
- [x] 异步任务：升级、备份恢复、导出和诊断等长任务统一采用 `202 + taskId` 与通用任务状态 API。
- [x] WebSocket：只读订阅；游标跨重启语义明确；授权撤销和 resync 行为明确。
- [x] 历史附录：H/I 已标记为历史材料，不得被生成工具当作当前规范来源。
- [x] 代码示例：YAML/JSON 示例可解析；Markdown 代码围栏配对。

**检查边界**：静态文档检查只能证明文本标识和引用结构一致，不能证明权限策略真实执行、数据库事务具备掉电原子性、IPC ACL 有效、任务副作用幂等或目标硬件恢复成功。上述行为必须由实现测试和真机证据证明。

### J.4 最终设计冻结门槛与状态定义

项目状态必须区分，不得把“文档已冻结”直接记为“产品已完成”：

| 状态 | 进入条件 | 可以对外声称的范围 |
| --- | --- | --- |
| 文档基线已冻结 | v1.6 已批准；变更控制生效 | 设计规则、契约边界和验收口径已确定 |
| ADR 已关闭 | 所有首发适用 ADR 已接受或有正式 N/A 记录 | 技术选型和型号差异有可追溯决策 |
| 软件实现完成 | 任务满足 §14.3 的代码、评审、自动化测试和文档条件 | 软件实现达到定义的完成条件；不代表真机通过 |
| 集成验证通过 | G0–G4 适用门禁通过；故障/越权/接口测试有证据 | 集成与安全测试范围内通过 |
| 目标设备验证通过 | 最低配置及所有首发型号的 G5 证据通过 | 对已测试型号/硬件/固件组合有效 |
| 产品发布批准 | 适用 P0/P1、风险接受、支持/恢复文档和责任人签核全部完成 | 仅对明确的产品版本与型号范围批准发布 |

最终发布前至少必须确认：

1. WBUI-001 的型号/硬件矩阵已由产品、硬件、系统和测试责任方签核。
2. 所有 P0 ADR 已接受；条件性 ADR 对每个型号明确启用或 N/A 的理由。
3. `release-scope.yaml` 与产品 Manifest、OpenAPI、权限策略、静态 Registry、固件模块和测试范围一致；关闭的功能在最终制品中没有可调用入口。
4. 所有首发适用 P0/P1 任务、需求和门禁均已完成；升级功能仅在 WBUI-062 的型号级证据通过后开放。
5. 配置事务/watchdog、数据库迁移/备份恢复、长任务 worker 恢复、事件游标连续性、IPC ACL、审计和时间可信性完成故障注入/真机验证。
6. 所有 Blocker/Critical 缺陷关闭；High 风险按 §13/附录 F 的规则形成有效签署，不得豁免不可豁免门禁。
7. 制品、OpenAPI、Registry、SBOM、测试报告、设备/固件版本和风险接受记录具备相同的发布标识与可追溯摘要。

### J.5 v1.6 文件级复核结论

本版复核包括权限与任务/需求标识、任务依赖、API 前缀、关键异步 API、事件游标/跨重启语义、任务执行者互斥和历史附录规范地位。文件级检查确认：权限 ID 55 个且引用全部解析；工程任务 44 个且无重复、无缺失依赖或依赖环，全部映射到需求；需求 ID 37 个且无重复；抽取的 API 路径中未发现缺少 `/api/v1` 的路径；Markdown 代码围栏 38 个且配对；4 个 YAML 示例与 1 个 JSON 示例解析通过。工程仓库必须再由 WBUI-085 自动化复核，不得将人工文档检查替代 CI。

**结论**：v1.6 可作为当前唯一设计基线用于 ADR 关闭、实施任务导入和验收条件冻结。仍未关闭的硬件事实、技术选型、真机测试和认证适用性继续保持待决，不得将其表述为“已验证”。

---

## v1.6 最终修订记录与使用说明

本版基于 v1.5 完成最终设计收口，主要变更如下：

1. 将文档状态明确为“最终设计基线（文档冻结版）”，并明确文档冻结不等于 ADR、实现、真机验证或认证完成。
2. 建立当前规范与历史附录的优先级：正文、附录 A–G、J 为当前基线；H/I 仅作历史审计记录。
3. 将每产品/版本的条件性功能纳入 `release-scope.yaml`，并要求与页面/API/Registry/固件执行入口及发布门禁一致。
4. 将 ADR-08/WBUI-062 修订为按型号和升级功能启用范围适用，避免未开放升级功能的型号被误判为必须具备 A/B 能力；已开放升级仍必须完成型号级安全与恢复验证。
5. 明确 WebSocket 游标使用持久 `streamId + partitionId + sequence`，`bootId` 只作诊断；跨服务事件采用各权威服务自己的 outbox/持久日志，不宣称全局原子提交。
6. 补齐任务执行器的原子 claim/lease、fencing token、下游结果对账和活动任务幂等记录保留要求。
7. 将任务取消策略固化为 `task-cancel-v1`，定义 owner 自取消与管理员跨用户取消的不同授权条件。
8. 补齐备份列表 API，统一异步升级/恢复任务状态查询入口。
9. 增加文档状态、工程实现、集成验证、目标设备验证和产品发布批准的区分及最终冻结门槛。

**使用方式**：从 v1.6 开始作为唯一当前设计基线；先签核产品/型号矩阵与 `release-scope.yaml`，再关闭适用 P0 ADR，随后按附录 D 依赖实施、按附录 E 追踪、按附录 F 进行门禁验收。H/I 不再作为当前需求源。任何架构/契约变化必须通过 ADR 并发布新版本。

**版本变更规则**：仅文字勘误且不改变语义，可发布 v1.6.x 修订并保留差异记录；改变安全边界、权限语义、API/IPC 契约、数据模型、事务/恢复或发布门槛时，必须新增 ADR 并发布新的次版本设计基线（如 v1.7），不得静默改写 v1.6。

**最终边界声明**：v1.6 是文档与设计层面的最终基线，不是产品最终验收报告。实际产品是否可发布，只能以对应型号/固件组合的 ADR、代码、CI、真机、安全和发布签核证据为准。
