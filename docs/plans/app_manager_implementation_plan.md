# EdgeOS WASM 应用管理器 (App Manager) 实施计划与任务分解表

## 📋 项目概览 (Overview)
实现一个高可靠的 EdgeOS 应用管理器 (`edge_app_mgr`)，支持：
1. **双格式应用交付**：单 `.wasm` 字节码文件与包含 `app.json` 的标准归档应用包；
2. **SQLite 持久化与开机自启**：开机读取 `app_registry` 表，无缝复原已安装应用；
3. **配置化故障隔离**：支持自动重启 (`auto_restart`) 与隔离告警 (`isolate_and_alarm`) 两种策略。

每个子任务粒度控制在 **2~5 分钟** 内完成，前台负责验证与更新任务状态。

---

## 阶段一：SQLite 注册表与数据持久化 (Phase 1: DB & Storage)

- [x] **Task 1.1**: 在 `modules/edge_core/include/edge_app_mgr.h` 中定义应用元数据 `app_manifest_t` 与状态枚举
- [x] **Task 1.2**: 在 `sqlite_service.h` 与 `sqlite_service.c` 中添加 `app_registry` 表建表与 CRUD 持久化 API
- [x] **Task 1.3**: 编写并执行 SQLite 应用注册表 C 单元测试 `tests/test_sqlite_app_registry.c`

---

## 阶段二：双格式应用解析与 Manifest 引擎 (Phase 2: Package Resolver)

- [x] **Task 2.1**: 实现 `app.json` 描述符解析器 (JSON Parser) 及单 `.wasm` 默认配置自动构造器
- [x] **Task 2.2**: 实现应用归档解压与文件持久化存放管理 (`/var/edgeos/apps/<app_id>/`)
- [x] **Task 2.3**: 实现 `fault_policy` 故障策略分配逻辑 (`auto_restart` / `isolate_and_alarm`)

---

## 阶段三：生命周期引擎与开机自动复原 (Phase 3: Lifecycle Engine)

- [x] **Task 3.1**: 实现 `edge_app_mgr_install()`、`start()`、`stop()`、`uninstall()` 核心状态机 API
- [x] **Task 3.2**: 实现 `edge_app_mgr_boot_restore()` 开机扫描数据库并自动载入 C 流引擎管线
- [x] **Task 3.3**: 在节点控制面 `service_t` / `rest_server` 中完整集成 `edge_app_mgr`
---

## 阶段四：REST API & 网关接口接入 (Phase 4: REST API & Mesh)

- [x] **Task 4.1**: 重构 `modules/edge_io/src/rest_server.c` 中的 `/api/v1/apps/upload` 接口，接入应用管理器
- [x] **Task 4.2**: 在 `rest_server.c` 中新增 `/api/v1/apps/list`、`/api/v1/apps/start`、`/api/v1/apps/stop` 控制接口

---

## 阶段五：系统级构建与端到端物理验证 (Phase 5: Build & Verification)

- [x] **Task 5.1**: 运行 `./build.sh` 与 `./test.sh` 确保全量 C 单元测试通过
- [x] **Task 5.2**: 启动全网关服务，测试端到端拖拽上传、在线热重载、开机断电复原与异常自动重启
