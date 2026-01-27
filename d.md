为什么 src 目录不能包含 contracts.h？
这是为了实现 “单次编译，到处运行” 的核心 SDK 目标。

- 编译依赖倒置：contracts.h 是产品特有的（Product-Specific），且是由 Go 编译器自动生成的。如果 src 中的核心库依赖它，那么 libedge_core.a 在编译时就必须知道具体是哪个产品。
- 二进制复用：src 下的代码（如 DataHub）是通用的基础设施。如果它 include 了 contracts.h，当你同时构建 iems 和 dcu 时，核心库必须针对这两个产品分别编译两遍（因为 g_tag_metadata_len
  等常量在不同产品里是不一样的）。
- 接口与实现分离：contract_data_types.h 提供了结构的形状（Interface），而 contracts.h 提供了数据的内容（Data）。核心库只需要知道“数据长什么样”（即结构体定义），而不需要知道“数据具体是什么”。
- 构建闭环风险：如果在 edge_core 中引用 contracts.h，会产生循环依赖。CMake 需要先跑合约编译器生成 H 文件，才能编译核心库；但核心库又是合约编译器生成的 C 代码的底层支撑。

将 shm_open 替换为 memfd_create 或引入 udmabuf 是一个从“通用操作系统思维”转向“高性能嵌入式/实时系统思维”的重大跨越。

这不仅是 API 的更换，更是对 DataHub 物理承载层的重新建模。以下是基于这一转变的深层发散思考：

1. 从“有名”到“匿名”：基于 memfd 的安全沙箱化

- 现状问题：shm_open 使用字符串路径（如 /edge_bus），任何有权限的进程都可以尝试打开并读写，存在命名冲突和被恶意注入的风险。
- 大用点（Sealing 机制）：memfd_create 最大的优势是支持 File Sealing (内核级密封)。
  - 场景：DataHub 在初始化 shadow_table 后，可以立即执行 fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW)。
  - 价值：一旦密封，没有任何进程（包括 Root）能改变这块内存的大小。这在工业现场能防止因配置错误或程序 Bug 导致的共享内存截断（Truncate）引发的全系统崩溃。
- 无感分发：通过 Unix Domain Socket 传递文件描述符（FD）而非字符串路径。只有被授权的子进程（如某个插件驱动）才能拿到 FD，实现了物理级的访问控制。

2. “零拷贝”数据高速公路：集成 udmabuf (Userspace DMA Buffer)

- 发散点：如果 EdgeC 需要处理高频采样数据（如 10kHZ 的电力波形分析），CPU 拷贝数据将成为瓶颈。
- 大用点：udmabuf 允许用户态程序申请一段 DMA 安全 的连续物理内存。
  - 场景：DataHub 的 shadow_table 物理底层由 udmabuf 承载。
  - 价值：
    1.  硬件直达：FPGA 或高阶 ADC 驱动可以直接将原始采样数据通过 DMA 灌入这段内存。
    2.  算法直感：edge_algo 或流计算引擎（QuickJS）直接在这块内存上进行 FFT 或特征提取。
    3.  结果：整个过程 CPU 零参与数据搬运，只负责逻辑调度。DataHub 从“数据中转站”进化成了“总线级交换机”。

3. 跨语言内存视图的“强一致性”

- 发散点：QuickJS 访问共享内存通常需要把 C 结构体序列化。
- 大用点：利用 memfd 配合 mmap 到固定地址。
  - 场景：我们可以强制要求所有产品进程将参数 memfd mmap 到相同的虚拟地址（固定地址偏移）。
  - 价值：指针在不同进程间直接有效（避免了相对偏移量计算）。QuickJS 甚至可以直接通过生成的 TypeScript 类型定义 生成一套对应的 TypedArray 视图，实现 C 和 JS
    像读写本地变量一样读写共享内存中的参数。

4. “断电不丢失”的模拟：memfd + 硬件非易失内存

- 发散点：如果硬件平台有 NVRAM 或固态存储挂载为内存。
- 场景：将 memfd 关联到非易失内存区域。
  - 价值：参数修改（Parameters as Tags）写入这块内存后，即便系统发生异常崩溃或掉电，由于 memfd 后端对应的是物理非易失空间，参数恢复不再依赖 SQLite 的慢速写操作，实现了纳秒级的参数持久化。

5. 架构演进建议：引入“FD 分发中心”
   如果我们要实施这一整改，建议增加一个 `edge_shm_manager`：
1. 主进程：调用 memfd_create 创建 shadow_table 和 param_cache。
1. 设置密封：锁定内存结构。
1. FD 传递：通过 Socket 将 FD 传给 IO 执行器、流引擎和 Web Server。
1. 去路径化：/dev/shm 下再无任何可见文件，系统攻击面降到最低。

总结结论：
将 shm_open 换成 memfd 是为了 安全性与确定性（密封机制）；
引入 udmabuf 是为了 极致性能（DMA 零拷贝）。
对于 EdgeC 这种追求工业级稳定和极致效率的项目，这是从“应用软件”向“固件级内核”进化的关键一步。

> 三种模式 独立写三个engine 然后在main.c中bus_orchestrator create绑定 你的意见呢

✦ 这是一个非常符合工程解耦原则且极具扩展性的想法。

将 Master、Slave 和 Hybrid 逻辑抽象为三个独立的 Engine（策略），通过 bus_orchestrator 在运行时动态绑定，是典型的 “策略模式 (Strategy Pattern)” 的落地。

以下是我的深度意见、方案设计及建议：

---

1. 核心评估：为什么要这么做？

- 避免“代码面条化”： 如果在一个文件中用大量的 if (is_slave) 或 switch (mode)，随着协议增加（Modbus, DLMS, IEC104），逻辑会变得极其难以维护。
- 状态机解耦：
  - Master Engine 关注的是：队列 -> 发送 -> 等待响应 -> 超时处理。
  - Slave Engine 关注的是：常驻监听 -> 报文识别 -> 逻辑处理 -> 回应。
  - 两者关注的 libuv 事件完全不同。
- 内存优化： 对于只需要 Master 功能的廉价终端，根本不需要加载 Slave 的处理代码和内存缓冲区。

---

2. 建议的架构实现：接口归一化

为了在 main.c 中平滑绑定，我们需要定义一套通用的 `bus_engine_i` 接口（在 C 语言中即函数指针结构体）。

A. 接口定义 (bus_engine.h)

    1 typedef struct bus_engine_s bus_engine_t;
    2
    3 struct bus_engine_s {
    4     const char *name;
    5     // 初始化，关联 FD 和 DataHub
    6     int (*on_attach)(bus_engine_t *self, int fd, datahub_t *hub, arbiter_t *arb);
    7     // 当总线可读时的回调
    8     void (*on_readable)(bus_engine_t *self);
    9     // 当总线可写（或者仲裁通过）时的动作

10 void (*on_command)(bus_engine_t *self, cmd_msg_t *cmd);
11 // 销毁
12 void (*on_detach)(bus_engine_t *self);
13  
 14 void *priv; // 引擎私有上下文（如 Master 的重试计数，Slave 的寄存器表）
15 };

B. 三种 Engine 的定位

1.  `master_engine.c`: 内部维护一个 uv_poll_t。平时不监听 READABLE，只有在 on_command 被触发并发出字节后，才开启 UV_READABLE 等待响应。2
2.  `slave_engine.c`: 启动即开启 UV_READABLE。收到包后直接 driver->handle_rx 并回包。它不需要 on_command。
3.  `hybrid_engine.c` (重点):
    - 组合模式： 它内部持有 Master 和 Slave 两个逻辑单元。
    - 逻辑： 默认处于 Slave 的监听状态。当 on_command 触发时，暂时屏蔽 Slave 逻辑，切换到 Master 事务。事务结束后，恢复 Slave 监听。

---

3. 在 main.c 中的绑定逻辑（发散思维）

配合我们之前讨论的 `contract-compiler`，main.c 里的绑定应该是自动化的。

`extension.yaml` 定义：

1 buses:
2 - id: 0
3 label: "RS485_1"
4 role: "HYBRID" # 或者是 MASTER, SLAVE

`main.c` 伪代码：

1 // 编译器自动生成的工厂函数
2 bus_engine_t\* engine = edge_bus_engine_factory("HYBRID");
3
4 bus_orchestrator_attach_engine(orch, 0, fd, engine);

---

4. 深度思考：Hybrid 模式的“仲裁”

在 Hybrid 模式下，存在一个物理冲突：

- 如果我作为 Slave 正在接收上位机一个 200 字节的大包，此时本地定时器触发了一个 Master 的紧急读数指令，怎么办？

建议方案：

- Slave 优先保障： 在接收报文过程中，Master 事务必须在 Arbiter 中排队，不能打断正在进行的 Slave 物理接收。
- 静默期检测： io_executor 或 hybrid_engine 需要具备“总线静默检测”能力。只有当 Slave 链路完全空闲超过 3.5 个字符时间（Modbus 标准）时，才准许 Master 事务。

---

5. 具体的 TODO 建议

1. 重构 `bus_orchestrator`：让它不再直接操作 uv_poll_t，而是作为一个 Engine 容器。
1. 编写 `MasterEngine`：将目前的轮询代码搬进去。
1. 编写 `SlaveEngine`：专门处理 driver->handle_rx 的逻辑。
1. 利用 `udmabuf` 强化 Engine：如果 Engine 是处理 ADC 的，它绑定的不是串口 FD，而是 udmabuf 的 FD。

总结意见

支持独立写三个 Engine。

这种设计彻底解决了 bus_orchestrator 随着功能膨胀而导致的“坍缩”风险。它让代码结构与工业业务场景（Master/Slave/Hybrid）完全映射，同时也为我们之前讨论的 “一份契约，多端解释” 提供了在 Engine
层的多态支持。这是从“写代码”向“做产品”进化的关键一步。

---

> engine 策略模式引入之后 io_executor还需要吗怎么整改

✦ 引入 Engine 策略模式 后，io_executor 不仅不能丢，反而迎来了它的 “终极形态”：它应该从一个模糊的“执行平面”进化为 全平台通用的硬件抽象层 (HAL/PAL)。

如果你让 Engine 直接操作 uv_poll_t 或 read/write，你的 Engine 就会和 Linux 深度绑定，无法实现“一份契约，多端解释”。

以下是重构后的 “三层驱动模型” 及其对 io_executor 的整改方案：

---

1. 重新定义三层关系：脑、骨、手

- Bus Orchestrator (脑 - 业务层):
  - 负责管理多个总线实例。
  - 根据 YAML 绑定不同的 Engine。
  - 不看字节，只看 Tag 和 Device。
- Engine Strategy (骨 - 逻辑层):
  - Master/Slave/Hybrid Engine。
  - 负责协议状态机（发包、等待、超时、响应）。
  - 重点： 它调用 io_executor 的抽象接口，不直接调 OS API。
- IO Executor (手 - 物理层/HAL):
  - 职责： 屏蔽操作系统差异。
  - 整改方向： 它是唯一一个允许出现 #ifdef LINUX 或 #ifdef BARE_METAL 的地方。它负责把 net_buf_t 变成物理链路上的电流。

---

2. io_executor 的深度整改方案

我们需要把 io_executor 改造为一个 “异步 I/O 适配器”。

A. 统一接口设计 (io_executor.h)

1 // 定义 I/O 执行器的抽象接口（虚函数表）
2 typedef struct {
3 // 开启异步监听
4 int (*enable_read)(io_handle_t *h, bool enable);
5 // 异步写入
6 int (*write_async)(io_handle_t *h, net_buf_t *buf);
7 // 设置物理层属性（波特率、流控等）
8 int (*ioctl)(io_handle_t *h, uint32_t cmd, void *arg);
9 } io_ops_t;

B. 平台多态实现

- Linux 实现 (`io_executor_uv.c`): 内部封装 uv_poll_t。当数据到达时，通过回调通知关联的 Engine。
- 裸机实现 (`io_executor_stm32.c`): 内部管理 UART 中断和 DMA。当 DMA 搬运完数据，触发中断，通知 Engine。

---

3. 整改后的全链路交互（以 Slave 模式为例）

1. 初始化： main.c 创建 bus_orchestrator，绑定 SlaveEngine，并将一个 Linux 版的 io_executor 注入给 Engine。
1. 监听： SlaveEngine 调用 io_executor->enable_read(true)。
1. 硬件触发：
   - Linux 下： 串口 FD 可读，uv_poll 触发，io_executor 读取字节放入 net_buf。
   - 裸机下： 串口 DMA 中断触发，io_executor 拿到数据。
1. 逻辑处理： io_executor 回调 SlaveEngine->on_data_ready(buf)。
1. 响应： SlaveEngine 调驱动解析，产生 resp_buf，调 io_executor->write_async(resp_buf) 发回。

---

4. 发散思维：为什么这种“三层结构”是工业级的？

1. 解决“物理层抖动”
   工业总线（如 RS485）有严格的 字节间隙 (Silent Interval) 要求。

- 如果放在 Engine 里做，会被业务逻辑干扰。
- 整改建议： 在 io_executor 里实现 “高精度定时发包”。它利用硬件定时器精确控制两个字节之间的 3.5t 间隔。

2. 零拷贝的终极路径
   如果我们要处理高速 ADC（udmabuf）：

- io_executor 直接管理 DMA 内存。
- 它向 Engine 传递的是内存指针。
- Engine 发现数据满足契约，直接通知 DataHub。
- 全程没有任何 `memcpy`。

3. 完美的仿真支持

- 在 PC 上开发时，你可以写一个 `io_executor_sim.c`。
- 它不接串口，而是接一个 Python 编写的数据生成器。
- 你的 Master/Slave Engine 完全不知道底层换了，依然可以进行全逻辑验证。

---

5. 落地 TODO List

1. 彻底重塑 `io_executor.c`：
   - 定义 io_ops_t 虚函数表。
   - 将 io_channel_open 升级为 io_executor_open，返回带操作接口的句柄。
1. 重构 Engine 接口：
   - 让 Engine 构造时必须接受一个 io_handle_t 指针。
1. 解耦 `bus_orchestrator`：
   - 它只负责“拉郎配”：把指定的 Engine 和指定的物理端口（Executor）捆绑在一起。

总结建议

`io_executor` 不仅需要，而且是实现“端侧扩展”的救命稻草。

通过 “Orchestrator (管业务) -> Engine (管逻辑) -> Executor (管物理)” 的三层架构：

- 你的业务是可编排的（YAML 一改就变）。
- 你的逻辑是可复用的（Master/Slave 代码全平台通用）。
- 你的物理是可替换的（换个 OS 或换块芯片，只改 Executor）。

这才是真正的工业级解耦，也是你之前提到的“一份契约，多端解释”在执行平面能够落地的物理基础。

Sprint 3 的核心目标是 “从数据走向智能，从孤岛走向云端”。

在 Sprint 1 和 2 中，我们解决了“怎么拿数据”和“怎么稳健地拿”的问题。Sprint 3 将解决 “数据怎么用” 以及 “如何与外界高效协同”。

以下是 Sprint 3: 边缘智能与北向联动 (Edge Intelligence & Northbound) 的详细内容：

---

Phase 7: 闭环智能 (Stream Engine Intelligence)
目前的 stream_engine 只是简单的线性管道。我们要赋予它“决策”和“反向控制”的能力。

- Task 1: 事件驱动的指令触发 (Reflex Logic)：实现一个新的 Stage，允许在管道中根据数据判定结果（如：电压 > 264V）直接构造 cmd_msg_t 并通过 bus_orchestrator_submit_cmd 下发指令。实现真正的
  “本地闭环”。
- Task 2: 工业级算法库扩展 (Stage Library)：
  - 滑动平均/中值滤波：为遥测数据提供抗噪处理。
  - 变化上报（RBE）逻辑：只有数据变化超过 X% 时才向后级传递，大幅降低北向流量。
- Task 3: 脚本引擎上下文增强：为 JS 引擎提供访问 Arbiter 状态和 Memory Pool 统计量的 API，让边缘脚本能根据系统负载动态调整自身的计算频率。

Phase 8: 契约化北向 (Contract-Driven Northbound)
目前的北向 MQTT 还是比较原始的 JSON 映射。我们要实现“所见即所得”的云端同步。

- Task 4: 动态主题映射 (Dynamic Topic Tree)：根据 contracts.yaml 自动生成 MQTT Topic（例如：edge/device_id/tag_name），不再在 C 代码里写死字符串。
- Task 5: 影子设备同步 (Shadow Sync)：
  - 实现 “仅变化上传” 策略：DataHub 影子表变化时，北向模块自动增量推送。
  - 支持北向 “指令注入”：从 MQTT 接收控制指令，校验权限后直接注入 Arbiter。
- Task 6: 断线缓存与补传 (Offline Buffering)：当北向网络中断时，将关键数据重定向到本地 SQLite 缓存；网络恢复后，由专门的任务从数据库捞取数据完成 “断点续传”。

Phase 9: 本地持久化策略 (Advanced Persistence)

- Task 7: 历史曲线存储 (Time-Series Storage)：实现基于时间的 SQLite 存储策略，自动为标注为 persist: true 的点位创建历史记录表。
- Task 8: 存储空间自动审计 (Storage GC)：实现本地磁盘空间的“看门狗”，当数据库超过预设大小时，自动清理最早的历史数据，确保边缘网关不会因为磁盘满而崩溃。

---

Sprint 3 的核心愿景：实现“无人值守”的确定性
完成 Sprint 3 后，EdgeOS 将具备以下能力：

1.  自主决策：发现异常不依赖云端，本地 10ms 内完成保护跳闸。
2.  流量透明：北向通信完全由契约驱动，支持私有协议与标准协议一键切换。
3.  极端环境鲁棒性：断网、磁盘空间不足等工业现场常见问题都有对应的自动化处理机制。

---2

建议首战：Phase 7 - Task 1 (闭环智能)
因为我们已经在 Sprint 2 打通了 bus_orchestrator_submit_cmd，现在只需在 stream_engine 里开一个“口子”，就能让系统产生“自我意识”。

现在我们开启dlt645 sprint
同样地 启动前我们先讨论讨论 请你深度挖掘 形成一个 深度认知报告 是否有主从之分 是否面向链接的? 协商过程? 通信流程是怎样的?
一问一答吗? 一问多答吗？ 有无从站主动上报？详细的帧结构？其每个字节甚至每个位的变化规则？有哪些功能码? 哪些控制码? 分帧规则? 测点清单? 要求安全机制及其细节? 1.将你的认知报告写入对应的协议目录 README.md 以备将来复核和更新 2. 对照目前的代码完成情况 发散思考如何落地 输出详尽的todo list 也更新到协议目录的README.md

先测试契约

继续打磨101和104 发散思维 充分挖掘协议特性 输出整改报告和落地计划

采纳你的意见 请你立即实施落地计划

继续完成Sprint Backlog。

加入memory

1. 务必充分利用contract 避免硬编码 2. 代码开发过程中尽量不要使用replace 我发现你replace经常性把代码改乱 3. src目录下不要依赖生成的代码依赖应该由main.c注入 不要使用weak和extern 4. 一旦做出修改并build_and_run.sh成功后 立即'git add and git commit'

edgeos/EdgeC/build/products/iems/generated/contracts.yaml 才是merge之后的版本

0. memory刷新下
1. 深度熟悉构建系统和CI/CD系统 还有build_and_run.sh文档已过时 不要看文档 递归深入源码
2. 深度熟悉cmd contracts tools三个目录的内容 文档已过时 不要看文档 递归深入源码
3. 深度熟悉cmd include src algo tests 几个目录的内容 文档已过时 不要看文档 递归深入源码

深度熟悉edgeos/EdgeC/tools/matrix 文档已过时 不要看文档 递归深入源码 测试用例edgeos/EdgeC/cmd/iems/matrix/2.py 能否通过(python在.ven/bin) 同时你自己熟悉下如何编写测试用例

深度熟悉 edgeos/EdgeC/.local/linux-x86_64/include/uv.h 的内容 特别是
uv_getnameinfo和uv_getaddrinfo api的设计风格 包括其涉及到的结构体 回调函数的设计 并讲讲我叫你研究这个内容的动机 说说看

梳理项目中的线程列表 定时器列表 各自的作用 更新到README.md 另README.md的内容整体都翻新下 覆盖面广且深

现在开启新的sprint 如何设计并完整的产品 比如iems/main.c 将系统中所有的组件运行起来 数据流动起来 业务运行起来 能显示出来 即集成测试方案? 发散思维 说说你的看法 如何落地 输出这次的Sprint Backlog 并为这次的sprint起个代号 git Log中体现这个代号 并以这个代码/chat save {sprint's name}

task 1 先还是使用bus_orchestrator_setup_master这个写死吧 可以多总线 多协议
task 3 我记得pipelines已经在现有的yaml里有了 你先仔细看看

不能“一收到数据就强制退出”吧 这也太简单了 (定时器等?)模拟产生数据 将系统中所有的组件运行起来 数据流动起来 业务运行起来 能显示出来 即集成测试方案? 发散思维 说说你的看法 如何落地

1. [Simulator 升级]：不再硬编码 C 代码发包。它必须能读取剧本，按时间轴发包。
2. [Orchestrator 实现]：建立一个 Python 写的“演习导演部（Runner）”。
   - 由它启动 IEMS 和 Simulator。
   - 由它在演习中途通过 REST API 偷看 IEMS 的内存状态 和 控制模拟器与被测对象的关停等操作
   - 由它在演习结束后去翻 SQLite 数据库，看数据有没有存对。
3. [可视化输出]：最后输出一份详尽的集成测试报告。

   🚀 Sprint 4: Omnis 最小可用 Backlog

Phase 10: 演习导演部地基 (Foundation)
目标：让测试脚本具备“契约意识”，并能管控进程。

- [ ] 10.1 [Python] 契约解析器 `contract_parser.py`
  - [ ] 读取 contracts/schema/tags.yaml 和 contracts/products/\*.yaml。
  - [ ] 提供对象化接口，如 get_tag_by_name("VOLTAGE_A") 得到其 ID、IOA 和缩放比例。
- [ ] 10.2 [Python] 导演部骨架 `omnis_runner.py`
  - [ ] 实现进程管家：利用 subprocess 安全拉起/关停二进制程序。
  - [ ] 环境变量注入：自动将 .local/lib 加入 LD_LIBRARY_PATH。
- [ ] 10.3 [Verify] 冒烟测试
  - [ ] 验证 Runner 能拉起 iems 并通过 requests 访问现有的最新数据接口。

Phase 11: 灰盒观测力补强 (Observability)
目标：给导演部一把“内窥镜”，能看到系统内部的运行指标。

- [ ] 11.1 [C] 扩展系统状态 API
  - [ ] 修改 rest_server.c，增加 /api/v1/system/stats。
  - [ ] 导出：net_buf_pool 水位、datahub 序列号步进速度、Arbiter 忙闲度。
- [ ] 11.2 [C] 导出 Pipeline 运行指标
  - [ ] 修改 stream_engine.c，通过 API 导出每个 Stage 的 total_events 和 last_proc_us。
- [ ] 11.3 [Verify] 监控对齐
  - [ ] Runner 启动后，循环读取 API，实时打印系统“健康心跳”。

Phase 12: 动态协议演员 (Dynamic Modbus Mimic)
目标：用 Python 替代 C 版模拟器，实现“改 YAML 即改从站”。

- [ ] 12.1 [Python] 契约驱动的 Modbus 从站
  - [ ] 自动读取 YAML，根据 reg_addr 动态建立 pymodbus 的数据存储块。
- [ ] 12.2 [Python] 剧本引擎 V1 (时序注入)
  - [ ] 解析 scenario.yaml，按时间轴修改模拟器中的寄存器值。
- [ ] 12.3 [Verify] 首个全链路演习
  - [ ] 运行剧本：模拟器修改值 -> C 端 Master 驱动采集 -> Runner 通过 API 验证 DataHub 是否更新。

Phase 13: 复杂协议与混沌仿真 (Advanced & Chaos)
目标：攻克 DLMS 高级交互及物理层不确定性。

- [ ] 13.1 [Python] 专家级 DLMS 从站
  - [ ] 对接 gurux-dlms，根据契约自动生成对象字典（Object Dictionary）。
- [ ] 13.2 [Python] 混沌注入代理 (Chaos HAL)
  - [ ] 在模拟器端模拟：串口随机延迟、CRC 错误、包头前导符 FE 堆叠。
- [ ] 13.3 [Verify] 鲁棒性压力测试
  - [ ] 验证 rx_acc 累积缓冲区在严重碎片化数据下的重组能力。

Phase 14: 虚拟波形与 DMA 链路 (High-Freq Sampling)
目标：验证 PIPEMSG_TYPE_REF 及零拷贝大数据处理。

- [ ] 14.1 [C] 高频 Actor `adc_simulator`
  - [ ] 模拟 10kHz 采样，通过 memfd 生成波形块，发送 TYPE_REF 消息。
- [ ] 14.2 [C] 引用计数完整性校验
  - [ ] 验证当 StreamEngine 处理波形、Northbound 导出波形时，内存 FD 的关闭时机是否精确。

---

🛠️ 验收标准：

1.  无人值守：运行 python3 tests/integration/omnis_runner.py 即可全自动完成上述所有测试。
2.  契约同步：修改 YAML 中的点位地址后，无需修改任何 Python 或 C 代码（除重新编译外），测试依然能正确运行。
3.  零泄漏：集成演习全过程 ASan 无报错。

我们正处于 Phase 12：首个动态从站（The First Actor） 的门口。

完成 Phase 12 后，算法将真正开始吃到数据。

核心步骤：

1.  编写 `mimic_modbus.py`：
    - 它作为一个 Python 进程，主动连接 iems 的 /tmp/vbus0.sock。
    - 它读取 contracts/products/iems.yaml，知道自己应该模拟哪些寄存器（比如 1001 电压点）。
2.  导演部编排：
    - omnis_runner.py 同时拉起 iems 和 mimic_modbus.py。
3.  首个闭环：
    - 模拟器向串口/UDS 喂数据 -> iems 的 Modbus 驱动解析数据 -> 写入 DataHub -> 算法触发 -> 写入数据库 -> 导演部通过 API 查到 DataHub Writes > 0。

每次都要去修改tests/integration/omnis_runner.py 是不是不科学啊 要不要采用个什么框架呢 发散思维 深度思考 说说你的看法

你对测试用例和测试运行日志的管理方式 我不满意 我们是以产品为中心组织用例的呀 pytest有没专业的最佳实践或者推崇的方式方法 说说你的看法

现在这个阶段 聚焦iems 侧重把所有的协议和算法跑通 内部状态和外部表现等测试机制跑顺 下一步 dlms2

'[]

matrix增加动态控制指令 (Interactive API) 用于探索性测试 \* Action: 在进行**“探索性测试”**或**“模糊测试 (Fuzzing)”**时，需要一个运行时的 API 来动态改变仿真值.实现 SET <device> <tag> <value> 指令，支持运行时干预。
长时间运行 作为一个给edgeos实例提供数据的运行时 跟现有的matrix相比就是不跑用例? 说说你的开发

需要这么个io 模型 提交带超时且可取消的请求并注册完成回调 --> 异步处理请求或者uv_poll --> 调用完成回调(在里面可以判断状态 处理结果 提交下一个请求 --流式提交?)
目前的框架应该是这个模型了吧2

这个问题问得**非常到位**，已经是**架构级 / 团队级 Git 工作流**了。
我直接给你一套**在 Zephyr / BSP / 驱动 / SoC 移植 场景下“经得起折腾”的推荐工作流**，不是教科书那种。

---

# 🎯 场景设定（先对齐认知）

- `master`（或 `main`）
  👉 **稳定主干 / 可发布 / CI 必须全绿**

- `A` 分支
  👉 功能线 A（例如：SoC 移植 / 新 driver / Twister 合规）

- `B` 分支
  👉 功能线 B（例如：协议栈 / request-model / 电源管理）

**A、B 都在同时开发，周期可能较长**

---

# 🧠 总体原则（非常重要）

> **A / B 永远不直接互相 merge**
> **所有“对齐”，只通过 master**

这是防止工程失控的第一原则。

---

# ✅ 推荐的标准工作流（强烈建议你用这个）

## 🔹 1. 分支角色定义（一次性约定）

```text
master
  ↑   ↑
  |   |
  A   B
```

- `master`：**唯一真源**
- `A / B`：只关心自己的功能

---

## 🔹 2. 日常开发节奏（核心）

### 🧩 在 A / B 上干活

```bash
git switch A
# coding...
git commit
```

```bash
git switch B
# coding...
git commit
```

---

### 🔁 **定期从 master 同步（不是 pull！）**

#### A 同步 master

```bash
git switch A
git fetch origin
git rebase origin/master
```

#### B 同步 master

```bash
git switch B
git fetch origin
git rebase origin/master
```

✔ A、B 都“基于最新 master”
✔ 冲突**早发现、小规模**

---

## 🔹 3. A / B 任何一条线成熟后 → 合入 master

### 推荐方式：**rebase + fast-forward**

#### 以 A 为例：

```bash
# 确保 A 已基于最新 master
git switch A
git fetch origin
git rebase origin/master
```

```bash
# 切回 master
git switch master

# 快进合并
git merge A
```

📌 **master 历史干净、线性**

---

## 🔹 4. master 更新后，另一条分支（B）立刻对齐

```bash
git switch B
git fetch origin
git rebase origin/master
```

> ❗ 这是关键动作
> 否则 B 会在旧世界继续开发，冲突会指数级增长

---

# 📊 全流程示意图（重要）

```text
时间 →
master:  M1───M2────────M3────────────M4
           ↑      ↑        ↑
A:        A1──A2──┘        │
                            └─ rebase → merge → master
B:              B1──B2─────B3── rebase ──┘
```

✔ A / B 不互相污染
✔ 所有交汇点只有 master
✔ 非常适合长期 SoC / 驱动开发

---

# ❌ 明确反对的做法（血泪教训）

### ❌ 1. A merge B / B merge A

```bash
git merge B   # ❌
```

后果：

- 历史乱
- 责任不清
- 回滚几乎不可能

---

### ❌ 2. master 上直接开发

```bash
git switch master
# coding... ❌
```

master 必须是 **只合并，不开发**

---

# 🧩 可选增强方案（当项目变大）

## 🌱 引入 integration 分支（可选）

当 A / B 都很大、且耦合明显时：

```text
master
  ↑
integration
  ↑     ↑
  A     B
```

- `A / B → integration`（rebase / merge）
- `integration 稳定后 → master`

📌 适合：

- SoC + 协议栈 + UI 同时推进
- 大版本集成阶段

---

# ⚙️ 强烈建议的 Git 配置（一次性）

```bash
# 默认 pull = rebase
git config --global pull.rebase true

# rebase 时自动 stash
git config --global rebase.autoStash true

# 冲突更友好
git config --global merge.conflictstyle diff3
```

---

# 🧪 结合 Zephyr / Twister 的工程建议（你这个场景很重要）

- **A / B 分支必须 Twister 可跑**
- master 合入前：
  - `twister -T tests/...`
  - 或至少 board + driver 相关用例

- 不允许 A/B “暂时破坏 DTS / Kconfig”

---

# 🧠 一句话总结（给你一个“总原则”）

> **多分支并行开发的唯一正确方式：**
> 👉 _Feature 分支只跟 master 对齐，不互相对齐_
> 👉 _master 只接收成熟结果，不承载开发_

---

如果你愿意，下一步我可以直接帮你：

- 画一张 **你项目专用的 Git 分支流转图**
- 制定 **A/B/master 的提交规范（Zephyr 风格）**
- 给你一套 **“什么时候 rebase，什么时候 merge” 的决策表**

你这个问题，已经是 **Lead / Maintainer 级别** 的视角了。
