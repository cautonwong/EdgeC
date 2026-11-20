# EdgeOS / EdgeC

EdgeC 是面向工业边缘设备的 C11 运行时与 SDK，提供产品进程、工业通信接入、数据处理、REST 管理接口和可选的 WASM 应用宿主。仓库同时包含 IEMS、DCU、SmartBox 等产品代码，以及 Vue Web 前端、Go 工具/服务和跨语言应用 SDK；这些部分并不都作为一个必需的生产部署单元运行。

> 当前代码和文档仍在演进。架构图描述的是仓库可见的实现边界，不代表所有产品功能或部署组合都已达到生产验收状态。

## 架构一览（C4）

下面的图按系统、容器、组件、代码和运行时视角逐层放大。产品进程是运行时核心；外部现场设备和北向平台通过产品适配器交互。Go 服务只在相应工具或部署组合中使用，不能据此推断为所有设备的必需生产服务。

### L0：系统全景

```mermaid
graph LR
    Dev[产品 / 应用开发者] -->|构建产品、SDK 应用| EdgeC[EdgeC 设备运行时]
    Operator[操作员 / 运维] -->|浏览器访问管理界面| EdgeC
    Field[现场设备<br/>表计、PLC、BMS、PCS] <-->|工业协议与控制| EdgeC
    EdgeC <-->|遥测、告警、控制| Platform[SCADA / 云平台]
    CI[构建与测试环境] -->|软件包 / 前端资源| EdgeC
```

### L1：系统上下文

```mermaid
C4Context
    title EdgeC 系统上下文

    Person(operator, "操作员 / 运维", "使用产品提供的本地管理接口与 Web UI")
    Person(developer, "产品 / 应用开发者", "构建产品、驱动或符合 SDK ABI 的应用")
    System(edgec, "EdgeC 设备运行时", "C11 产品进程及其按配置启用的模块")
    System_Ext(field, "现场设备", "表计、PLC、BMS、PCS 与传感器")
    System_Ext(supervisory, "SCADA / 云平台", "接收上送数据或发送控制请求")
    System_Ext(browser, "浏览器", "运行产品 Web 前端")

    Rel(operator, browser, "操作")
    Rel(browser, edgec, "管理与查询", "HTTP / REST")
    Rel(developer, edgec, "交付产品或应用包")
    Rel(field, edgec, "采集、状态与控制", "已配置的工业协议")
    Rel(edgec, supervisory, "北向数据与控制", "已配置的传输协议")
```

### L2：容器与部署单元

图中 DataHub 是产品进程使用的数据通道和共享内存区域，不是独立服务进程。WASM `app-host` 是可选的独立进程；产品代码通过 socketpair 与其通信。Web 前端是独立构建的静态资源，由具体部署方式提供。

```mermaid
C4Container
    title EdgeC 设备容器视图

    Person(operator, "操作员", "浏览器用户")
    System_Boundary(device, "边缘设备") {
        Container(product, "产品进程", "C11 / libuv", "组合产品领域逻辑、驱动、DataHub、流引擎和服务接口；每台设备运行所选产品")
        Container(apphost, "app-host", "C / WAMR", "可选：在独立进程中承载 WASM 应用")
        ContainerDb(datahub, "DataHub / SHM", "C 数据结构 / POSIX 共享内存", "产品使用的消息环与影子数据")
        ContainerDb(sqlite, "SQLite 数据库", "SQLite", "产品使用的本地参数、注册表或历史数据")
        Container(web, "Web 前端资源", "Vue / Vite", "按产品构建的静态资源；具体托管方式由部署集成决定")
    }
    System_Ext(field, "现场设备", "工业设备与传感器")
    System_Ext(cloud, "SCADA / 云平台", "北向系统")

    Rel(operator, web, "打开界面", "浏览器")
    Rel(operator, product, "管理与查询", "HTTP / REST")
    Rel(field, product, "采集与控制", "工业协议 / 驱动")
    Rel(product, cloud, "遥测、事件与控制交互", "配置的北向协议")
    Rel(product, datahub, "发布、读写影子数据", "进程内 API / SHM")
    Rel(product, sqlite, "读写产品数据", "SQLite API")
    Rel(product, apphost, "可选应用 Host ABI", "socketpair")
    Rel(apphost, datahub, "经产品提供的 Host ABI 访问", "由产品进程代理")
```

### L3：IEMS 产品进程组件

这是 `cmd/iems/iems_app.c` 的装配视图。不同产品有各自的组合根和模块组合，不能把此图直接套用到 DCU 或 SmartBox。

```mermaid
C4Component
    title IEMS 产品进程组件

    Container_Boundary(iems, "cmd/iems：IEMS 产品进程") {
        Component(root, "iems_app_assemble / main", "组合根", "装配、启动与释放产品组件")
        Component(node, "iems_node", "产品领域", "IEMS 聚合根与产品配置")
        Component(bus, "bus_orchestrator + drivers", "南向适配", "按产品配置连接现场总线")
        Component(hub, "DataHub", "数据通道", "产品消息与影子数据 API")
        Component(stream, "Stream Engine", "计算", "加载产品管线并运行计算线程")
        Component(db, "sqlite_svc", "持久化", "本地 SQLite 服务")
        Component(north, "northbound_mgr", "北向适配", "通过注入的数据源和传输端口上送")
        Component(rest, "REST / UDS 服务", "接口", "产品管理与本机控制接口")
        Component(appmgr, "app manager / runtime", "可选应用管理", "管理 WASM 应用与 app-host 生命周期")
    }
    System_Ext(field, "现场设备", "按配置连接")
    System_Ext(cloud, "北向平台", "按配置连接")
    Container(apphost, "app-host", "独立进程", "仅在启用应用运行时使用")

    Rel(root, node, "创建与管理")
    Rel(root, bus, "装配驱动和总线")
    Rel(root, hub, "创建并注入")
    Rel(root, stream, "创建、加载管线并启动")
    Rel(root, db, "装配本地持久化")
    Rel(root, north, "装配北向管理器")
    Rel(root, rest, "启动接口")
    Rel(root, appmgr, "可用时装配应用管理")
    Rel(field, bus, "工业通信")
    Rel(bus, hub, "采集数据")
    Rel(hub, stream, "消息 / 唤醒")
    Rel(stream, hub, "读取或发布数据")
    Rel(stream, node, "经控制端口提交产品控制")
    Rel(north, hub, "读取数据源")
    Rel(north, cloud, "北向通信")
    Rel(rest, node, "产品查询与控制")
    Rel(rest, appmgr, "应用管理接口")
    Rel(appmgr, apphost, "启动 / 停止及 Host ABI", "socketpair")
```

### L4：关键代码关系

```mermaid
classDiagram
    class main_c {
        +main(argc, argv)
        +读取环境配置
        +启动 libuv 服务循环
    }
    class iems_app_t {
        +loop
        +node
        +hub
        +orch
        +stream_eng
        +north
        +db_service
        +app_runtime
    }
    class iems_node_t {
        +产品领域状态
        +产品配置
    }
    class datahub_t {
        +publish(msg)
        +write_shadow(msg)
        +read_shadow(tag_id)
        +subscribe_filtered(filter)
    }
    class stream_engine_t {
        +create_datahub(hub)
        +load_static_metadata()
        +start()
        +destroy()
    }
    class stream_bus_datahub_adapter {
        +stream_bus_datahub_port(hub)
    }
    class iems_app_runtime_t {
        +start_ex(app_id, wasm_path)
        +stop(app_id)
        +destroy()
    }
    class app_host_server_t {
        +create_ex(loop, fd, ports)
        +post_platform_event(kind, payload)
        +destroy()
    }

    main_c --> iems_app_t : assemble / start / run / destroy
    iems_app_t --> iems_node_t : owns product aggregate
    iems_app_t --> datahub_t : owns runtime data channel
    iems_app_t --> stream_engine_t : creates and starts
    stream_bus_datahub_adapter ..> datahub_t : implements stream bus port
    stream_bus_datahub_adapter ..> stream_engine_t : bridges product composition
    iems_app_t --> iems_app_runtime_t : optional app runtime
    iems_app_runtime_t --> app_host_server_t : one ABI server per app host
```

### 运行时交互：南向采集到流计算

```mermaid
C4Dynamic
    title IEMS 遥测处理路径

    Container_Ext(field, "现场设备", "设备", "响应产品采集请求")
    Container(bus, "bus_orchestrator / driver", "C", "协议驱动与总线编排")
    Container(hub, "DataHub", "C / SHM", "消息与影子数据")
    Container(stream, "Stream Engine", "C", "监听数据并执行管线")
    Container(node, "iems_node", "C", "产品领域逻辑")

    Rel_Index(1, bus, field, "发起采集 / 接收响应", "工业协议")
    Rel_Index(2, bus, hub, "发布采集值", "DataHub API")
    Rel_Index(3, hub, stream, "更新消息并通知监听者", "ring / libuv async")
    Rel_Index(4, stream, node, "向产品控制端口提交管线结果", "注入的回调")
    Rel_Index(5, node, bus, "经产品控制路径发起执行", "产品端口 / 驱动")
```

### Deployment：单设备逻辑部署

```mermaid
C4Deployment
    title EdgeC 单设备逻辑部署

    Deployment_Node(device, "边缘设备", "Linux") {
        Container(product, "iems / dcu / smart_box", "C11 产品二进制", "按目标构建和配置选择一个产品")
        Container(datahub, "DataHub", "产品进程内 API + SHM", "由产品进程初始化")
        ContainerDb(db, "产品数据库", "SQLite", "路径由产品配置决定")
        Container(apphost, "app-host", "独立进程 / WAMR", "可选应用运行时")
        Container(web, "Web 静态资源", "Vue 构建产物", "由具体设备集成提供")
    }
    Deployment_Node(operator_pc, "操作员终端", "浏览器") {
        Container(browser, "浏览器", "Web Browser", "访问产品管理界面")
    }
    System_Ext(field, "现场设备", "表计、PLC、BMS、PCS 等")
    System_Ext(platform, "北向平台", "SCADA / 云端")

    Rel(field, product, "采集与控制", "配置的工业协议")
    Rel(product, datahub, "读写")
    Rel(product, db, "读写")
    Rel(product, apphost, "可选 Host ABI", "socketpair")
    Rel(product, platform, "北向交互", "配置的传输协议")
    Rel(browser, web, "加载静态资源", "HTTP(S)，依部署配置")
    Rel(browser, product, "产品 API", "HTTP / REST")
```

## 主要目录

| 路径 | 用途 |
| --- | --- |
| `cmd/` | 产品与程序入口，包括 `iems`、`dcu`、`smart_box`、`app-host` 和 launcher。 |
| `modules/` | 可复用运行时模块，例如 `edge_core`、`edge_io`、`edge_stream`、`edge_api`、`edge_app_host`。 |
| `adapters/` | 将模块端口连接到平台、驱动、SQLite、REST 和 MQTT 等实现。 |
| `foundation/` | 通用基础类型、算法、协议编解码与缓冲池。 |
| `sdk/` | WASM 应用 ABI/WIT 与 C、Python、Go、JavaScript SDK。 |
| `web/` | Vue/Vite 前端、产品包、共享 Core、构建工具和前端测试。 |
| `tests/`、`fuzz/`、`acceptance/` | 单元/集成测试、模糊测试与产品验收台架。 |
| `docs/` | 设计、ADR、开发手册和协议资料。 |
| `go/` | Go 工具和服务代码；具体服务用途以各自文档和启动方式为准。 |

## 构建与测试

核心使用 CMake 构建，默认启用 Web 前端构建。需要 CMake、Ninja、C11 编译器、Python 3，以及 `CMakeLists.txt` 中查找的本地/第三方依赖（包括 libuv、cJSON、SQLite、ZLIB、zlog、Paho MQTT、CURL、QuickJS、iwasm、cmocka 和 OpenSSL）。依赖安装与目标平台有关。交叉编译请使用 `cmake/toolchains/` 中对应的工具链文件。

```bash
# 配置并构建 C 核心；关闭前端可避免此步骤调用 Web 构建
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_WEB_BUILD=OFF
cmake --build build -j2

# 运行 CTest 测试
ctest --test-dir build --output-on-failure

# 运行架构与卫生守卫（正反夹具 + 活树检查）
python3 ci/scripts/run_guard_selftest.py
```

若要运行 Web 前端测试或构建，请进入 `web/` 并用仓库锁文件安装依赖：

```bash
cd web
npm ci
npm run test:unit
npm run build:all
```

`./test.sh` 提供已配置 `build/` 的 C 单元测试入口。产品验收由独立的 `test-platform` 按 `acceptance/testplatform.sha` 固定版本驱动：

```bash
./test.sh
./test.sh --omnis
```

## 产品与架构约定

- 产品组合根位于 `cmd/<product>/`；IEMS 的入口与装配分别见 `cmd/iems/main.c`、`cmd/iems/iems_app.c`。
- 共享模块依赖通过消费者定义的端口和 `adapters/` 注入，产品领域逻辑由产品自身维护。
- ADR-0010 已决定废除全局中心化契约编译器和全局数字 TagID 的架构方向；`contracts/` 不应被当作新的全局产品点表来源。参见 [`docs/adr/0010-eliminate-global-contracts-and-tagid.md`](docs/adr/0010-eliminate-global-contracts-and-tagid.md)。
- WASM 应用 ABI 的当前约定和已知限制见 [`sdk/README.md`](sdk/README.md) 与 [`docs/wasm_micro_app_guide.md`](docs/wasm_micro_app_guide.md)。不要将设计目标或未接通的 ABI 能力当作已交付功能。

## 进一步阅读

- [开发手册](docs/EDGEOS_DEV_HANDBOOK.md)
- [IEMS 上下文](cmd/iems/CONTEXT.md)
- [Web 前端说明](web/README.md)
- [SDK ABI 与一致性测试](sdk/README.md)
- [设计决策（ADR）](docs/adr/)
- [开发路线图](ROADMAP.md)
