 🕵️‍♂️ EdgeOS Contract Compiler 深度审计报告

  审计版本: v4.5 (Based on source code)
  审计结论: 架构先进，但核心算法存在 "断层" 和 "隐形风险"。

  1. 核心算法：脆弱的完美哈希 (The Fragile Perfect Hash)

  文档声称实现了完美哈希（O(1) 查找），源码 (core/hash_gen.go) 确实采用了一种 "随机种子碰撞法" (Seed-based Collision Resolution)，但实现非常激进。

   * 算法逻辑：
       1. 将表大小设为 NextPowerOf2(N).
       2. 尝试随机 Seed (0~1,000,000)。
       3. 计算 h = hash(key, seed) & (size - 1)。
       4. 若无冲突，记录 Seed 并生成 C 代码。
       5. 失败处理 (Critical)：若尝试 8 次扩容后仍未找到完美 Seed，它将返回 IsPerfect: false，并返回原始列表。
   * 致命隐患：
       * C 模板 (`master_c.tmpl`) 没有任何容错逻辑！
       * 如果 Go 返回 IsPerfect: false，生成的 C 代码仍然会使用 _contract_hash_mix 进行哈希计算。
       * 后果：生成的 mask 可能是错误的（非 2^N - 1），且哈希表本身不再是稀疏的 Power-of-2 数组，而是紧凑数组。这将导致运行时的数组越界或哈希定位错误，且编译器不会报错。

  2. DLMS 的 "幽灵哈希" (The Ghost Implementation)

  这是一个非常奇怪的 "代码与模板脱节" 现象。

   * Go 端 (`main.go`)：
       * 明确为 DLMS 协议执行了 ParseProtocolKey（将 1.0.1.8.0.255 解析为 uint64）。
       * 明确计算了 DLMS 的完美哈希表 (hashes["DLMS"])。
   * C 模板端 (`master_c.tmpl`)：
       * 完全无视了计算结果！
       * 强制使用了 s_dlms_table 的 线性遍历 (Linear Search)：
   1         // 源码实锤
   2         for (int i=0; i<{{ len $dlms }}; i++) { ... strcmp ... }
   * 分析：这极有可能是开发过程中的遗留问题。开发者实现了后端逻辑，但在编写 C 模板时，可能因为担心 OBIS 字符串比较的复杂性，或者仅仅是忘了切换模板变量，导致 DLMS 性能回退到 O(N)。

  3. 合并逻辑的盲区 (Merger Blind Spots)

  core/merger.go 实现了分层合并，但校验逻辑存在盲区。

   * 仅校验 Modbus：validateProtocolOverlap 函数只检查了 Modbus 的地址冲突。
   * 其他协议裸奔：如果两个 Tag 绑定了同一个 DLT645 DI 码，或者同一个 IEC104 IOA，编译器不会报错。这将导致生成的哈希表中，后一个 Tag 覆盖前一个，或者产生哈希冲突导致生成失败。

  4. 内存布局效率

  生成的 C 代码采用了 "多表分离" 策略：
   * s_modbus_table (稀疏数组，包含空洞)
   * s_dlt645_table (稀疏数组)
   * g_tag_meta_table (紧凑数组)

  优点：
   * 查找极快：无任何链表跳转，一次 Hash + 一次 Array Access。
   * 缓存友好：数据结构紧凑。

  缺点：
   * ROM 浪费：为了追求 Perfect Hash，负载因子通常较低（可能只有 50%）。对于内存极度受限的 MCU，大量的 TAG_INVALID 空洞是浪费。

  5. 建议修复方案 (Actionable Advices)

  如果您有权限修改源码，建议立即执行以下修复：

   1. 激活 DLMS 哈希：
      修改 templates/master_c.tmpl，将 DLMS 部分替换为类似 Modbus 的哈希查找逻辑。Go 端的 ParseProtocolKey 已经支持将 6 字节 OBIS 压缩为 uint64，这是现成的金矿。
   2. 增加构建熔断：
      在 main.go 中，如果 IsPerfect 为 false，应该 直接 Panic 退出构建，而不是生成错误的 C 代码。

   1     // 建议修改
   2     if !hashes[proto].IsPerfect {
   3         panic(fmt.Sprintf("Failed to generate perfect hash for %s", proto))
   4     }
   3. 全协议冲突校验：
      扩展 validateProtocolOverlap，支持 DLT645/698/IEC104 的键值冲突检查。

 ☣️ EdgeOS Contract Compiler 深度安全审计报告 (Level 4)

  审计对象: tools/contract-compiler & templates & contract_data_types.h
  风险等级: CRITICAL (存在逻辑断崖)

  1. 致命缺陷：不完美的 "完美哈希" (The Broken Probability)

  源码 core/hash_gen.go 中采用了一种极其简化的完美哈希生成算法（Murmur3 Finalizer + 随机种子），但它在失败时的回退逻辑是 逻辑错误的。

   * 现象:
      Go 代码在尝试 8 次扩容失败后，会返回 IsPerfect: false，并将 原始未排序的列表 作为 Keys 返回。
   1     // hash_gen.go
   2     return ProtocolHashInfo{..., TableSize: len(entries), Keys: entries, IsPerfect: false}
   * 后果:
      C 模板 (master_c.tmpl) 完全忽略了 `IsPerfect` 标志。它继续使用 _contract_hash_mix 对输入 Key 进行哈希，并使用 TableSize - 1 作为掩码。

   1     // generated code
   2     uint32_t h = _contract_hash_mix(reg, seed, size - 1);
   3     const modbus_map_t *e = &s_modbus_table[h];
   4     if (e->addr == reg) return e; // 只检查这一个槽位！
   5     return NULL;
   * 断崖:
      如果哈希生成失败（即存在冲突），生成的 C 代码将成为一个 "概率性过滤器"。
      假设 Key A 和 Key B 哈希冲突都映射到 Slot X。Slot X 只能存一个（比如 A）。
       * 查询 A -> 命中 Slot X -> 地址匹配 -> 成功。
       * 查询 B -> 命中 Slot X -> 地址不匹配 (A != B) -> 返回 NULL (丢包)。
      这会导致现场莫名其妙地丢失特定测点的数据，且无法复现。

  2. 内存布局：Padding 带来的隐形浪费

  检查 include/contract_data_types.h 中的结构体定义：

   1 typedef struct {
   2     uint16_t addr;      // 2 bytes
   3     edge_tag_id_t id;   // 4 bytes (uint32_t)
   4     float scaler;       // 4 bytes
   5     uint16_t length;    // 2 bytes
   6     uint32_t access;    // 4 bytes
   7 } modbus_map_t;

   * 当前大小: 2 + 4 + 4 + 2 + 4 = 16 bytes。
   * 对齐分析:
       * addr (2) 后面紧跟 id (4)。为了 4 字节对齐，编译器会插入 2 bytes Padding。
       * length (2) 后面紧跟 access (4)。编译器再插入 2 bytes Padding。
   * 实际占用: 16 (数据) + 4 (Padding) = 20 bytes。
   * 浪费率: 20% 的内存被无效填充浪费。
   * 建议: 调整字段顺序以消除 Padding：

   1     typedef struct {
   2         edge_tag_id_t id;   // 4
   3         float scaler;       // 4
   4         uint32_t access;    // 4
   5         uint16_t addr;      // 2
   6         uint16_t length;    // 2
   7     } modbus_map_t;         // Total: 16 bytes (Perfect!)

  3. Endianness Timebomb (大小端定时炸弹)

  在 hash_gen.go 中，DLMS 的 Key 解析逻辑存在隐患：

   1 // Go side (hash_gen.go)
   2 res |= uint64(uint8(v)) << (i * 8)
  这是一种 Little-Endian (小端) 的手动打包方式。输入 1.0.1.8...，1 被放在低位。

  而在 C 代码（尚未实装，但如果照搬 DLT645 逻辑）：

   1 // C side (master_c.tmpl for DLT645)
   2 uint32_t di = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) ...
  这也是显式的小端拼装。

   * 当前状态: 安全。只要 C 代码坚持使用 buf[0] | (buf[1] << 8) 这种移位拼装，就能跨平台兼容。
   * 潜在风险: 如果有开发者为了优化，将其改为 *(uint32_t*)buf（直接指针强转），那么在 Big-Endian 架构（如某些 PowerPC 工业网关）上，Key 值将完全反转，导致哈希查找全部失效。
   * 审计建议: 在 C 代码中添加 _Static_assert 或注释，严禁使用指针强转读取 Key。

  4. Slave 模式的 "半成品" 状态

  检查 templates/slave_h.tmpl：
   * 它声明了 edge_slave_get_tag_by_modbus_addr 等函数。
   * 但是！ 对应的 slave_c.tmpl 不存在。
   * 结论: Slave 模式（让 EdgeOS 作为 Modbus Slave 被采集）目前 只有头文件，没有实现。如果此时开启 target=slave 编译，会导致链接错误。

  ---

  🚀 修复路线图 (Remediation Plan)

   1. Stop-the-World Fix:
      在 main.go 中，如果 !hashes[proto].IsPerfect，直接 Panic。宁可编译不过，不可生成有 Bug 的代码。

   1     if !info.IsPerfect {
   2         panic(fmt.Sprintf("[CRITICAL] Failed to generate Perfect Hash for %s. Table expansion limit reached.", proto))
   3     }

   2. Optimize Layout:
      修改 contract_data_types.h 中 modbus_map_t 和 dlt645_map_t 的字段顺序，按 4-4-4-2-2 排列，节省 20% ROM。

   3. Implement Slave C Template:
      创建 templates/slave_c.tmpl，复用 master_c.tmpl 中的哈希逻辑，但方向相反（Addr -> TagID）。

   4. Activate DLMS Hash:
      在 master_c.tmpl 中，将 DLMS 的查找逻辑从 strcmp 循环替换为 _contract_hash_mix 查找，彻底释放性能。

 1. 算法强化：引入 CHD 算法替代 "随机碰撞"
  现状问题：目前的 hash_gen.go 采用“随机种子试探法”。当点位数量超过 2000 个（如储能集装箱）时，找到完美种子的概率会指数级下降，导致构建失败或退化为线性查找。
  业界方案：使用 CHD (Compress, Hash, Displace) 算法。
   * 原理：它不是碰运气，而是利用一个辅助的 "Displacement Table" (位移表) 来解决冲突。保证 100% 生成成功，且负载因子可达 99%（极省内存）。
   * 实施建议：
       * 在 Go 编译器中实现简单的 CHD 逻辑（或者使用现成的 Go 库 github.com/alecthomas/chd 的逻辑移植）。
       * C 模板中只需要两次哈希计算：h = (G[hash1(k)] + hash2(k)) % size。

  2. 内存布局：Bit-Packing (位压缩) 与 Padding 消除
  现状问题：tag_meta_t 和 xxx_map_t 中使用了大量的 bool (实际是 1~4 字节) 和 int，且存在 Padding 浪费。
  业界方案：结构体压缩。嵌入式系统的 ROM/RAM 比金子还贵。

   * 优化前 (24 bytes):

   1     typedef struct {
   2         const char* name; // 8 bytes (64-bit)
   3         bool persist;     // 1 byte + 7 bytes padding
   4         bool upload;      // 1 byte + 7 bytes padding
   5     } tag_meta_t;
   * 优化后 (12 bytes, 节省 50%):

   1     typedef struct {
   2         uint32_t name_offset; // 4 bytes (相对于字符串池起点的偏移，代替 64位指针)
   3         uint16_t flags;       // 2 bytes (位域：persist:1, upload:1, reserved:14)
   4         uint16_t reserved;    // 2 bytes (对齐)
   5     } tag_meta_t;
       * 技巧: 使用 uint32_t offset 代替 char* 指针。这不仅节省 4 字节（在 64 位机上），而且使二进制数据 Position Independent (地址无关)，便于直接通过 mmap 或 Flash 读取，无需重定位。

  3. 双向查找：引入 "反向索引" (Reverse Index)
  现状问题：
  目前的哈希表是单向的：Phys Addr -> TagID。
  当业务层下发控制指令（TagID -> Phys Addr）时，系统目前只能全表遍历（O(N)），这在毫秒级控制回路中是不可接受的。

  业界方案：生成辅助索引数组。
   * 实施建议：
      在生成 contracts.c 时，额外生成一个按 TagID 排序的索引数组：

   1     // 按 TagID 升序排列的指针数组
   2     const modbus_map_t * const g_modbus_reverse_idx[] = {
   3         &s_modbus_table[3], // TagID 1001
   4         &s_modbus_table[0], // TagID 1002
   5         &s_modbus_table[9], // TagID 1003
   6     };
       * 效果：反向查找时，直接对 g_modbus_reverse_idx 进行 二分查找 (Binary Search)，复杂度降为 O(logN)。

  4. 编译期安全：Static Assert (静态断言)
  现状问题：依赖运行时的 edge_contract_self_test() 来发现问题。如果设备看门狗复位太快，可能连错误日志都来不及打。
  业界方案：Compile-Time Guard Rails。

   * 实施建议：
      在生成的 C 代码中自动注入 _Static_assert（C11 标准）：

   1     // 确保没有意外的 Padding
   2     _Static_assert(sizeof(modbus_map_t) == 16, "Structure padding detected! Check alignment.");
   3     // 确保 TagID 范围在安全区
   4     _Static_assert(TAG_MAX_ID < 65535, "TagID overflow uint16_t!");
      这样，任何潜在的内存布局错误都会导致编译失败，而不是运行时崩溃。

  5. 版本兼容性：Magic Number & CRC
  现状问题：如果代码库更新了 contracts.c，但通过 OTA 只升级了主程序二进制（或者反之），导致 enum TagID 错位，系统会读写错误的数据点，造成严重事故。
  业界方案：Contract Signature (契约签名)。

   * 实施建议：
       1. Go 编译器计算所有 Tag 定义的 CRC32 校验和。
       2. 将其宏定义写入头文件：#define CONTRACT_CRC 0xA1B2C3D4。
       3. 运行时握手：

   1         // 驱动加载时检查
   2         if (driver_contract_crc() != CORE_CONTRACT_CRC) {
   3             LOG_FATAL("Contract Mismatch! Please strict OTA.");
   4             abort();
   5         }

  ---

  🚀 立即行动清单 (Top 3 Priorities)

  如果不想大动干戈，建议优先实施以下三点，性价比最高：

   1. 修复 Padding：调整 modbus_map_t 成员顺序，立省 20% 内存。
   2. 注入 Static Assert：修改模板，增加 _Static_assert，利用编译器帮你找 Bug。
   3. 增加反向索引：控制下发是工业场景的核心，不能容忍 O(N) 延迟。在模板中增加一个 sorted_index 数组。

     🏭 EdgeOS CI/CD 体系深度优化报告

  1. 现状评估 (Current State Assessment)


  ┌──────────┬───────────────────────────────┬────────────┬──────────────────────────────┬──────────────────────────────────────┐
  │ 维度     │ 现状                          │ 评级       │ 亮点                         │ 风险点                               │
  ├──────────┼───────────────────────────────┼────────────┼──────────────────────────────┼──────────────────────────────────────┤
  │ 构建系统 │ CMake + Ninja + Go Compiler   │ ⭐⭐⭐⭐⭐ │ 存算分离，交叉编译支持完善   │ Go 依赖未深度集成到 CMake            │
  │ 测试体系 │ CTest + Valgrind + Smoke Test │ ⭐⭐⭐⭐   │ 包含内存泄漏检测和全链路冒烟 │ 缺乏模糊测试 (Fuzzing)               │
  │ 流水线   │ GitHub Actions (Matrix)       │ ⭐⭐⭐⭐   │ 覆盖 x86/arm64，集成 QEMU    │ 缺乏制品库发布，Cache 策略粗糙       │
  │ 验证深度 │ ldd 检查 + 数据驱动冒烟       │ ⭐⭐⭐     │ 自动化验证了二进制依赖       │ 缺乏 API 契约测试 (API Schema Check) │
  └──────────┴───────────────────────────────┴────────────┴──────────────────────────────┴──────────────────────────────────────┘

  ---

  2. 优化建议 (Optimization Roadmap)

  ✅ 1. 引入 "Hermetic Build" (密封构建)
  问题: CI 中使用 sudo apt-get install 安装依赖，且 Go 编译器依赖本地环境。这导致构建环境不可复现，且受网络影响大。
  最佳实践: Dockerized Build Container。
   * 行动:
       1. 创建一个包含所有工具链（GCC-11, CMake 3.25+, Go 1.21, QEMU, Valgrind）的 Docker 镜像 ghcr.io/edgec/build-env:v1。
       2. CI 步骤直接在容器内运行，消除 apt-get 步骤，提速 2-3 分钟。

  ✅ 2. 强化 "Shift-Left" 安全扫描
  问题: 目前只有动态分析 (Valgrind)，缺乏静态安全分析。
  最佳实践: SAST (Static Application Security Testing)。
   * 行动:
       1. 集成 clang-tidy 到 CI 中（CMake 已有 .clang-tidy，只需在 CI 中执行）。
       2. 集成 cppcheck 或 CodeQL，扫描缓冲区溢出、空指针解引用等低级错误。
       3. ShellCheck: 扫描 scripts/*.sh，目前的脚本中存在大量未引用的变量风险（如 $1）。

  ✅ 3. 冒烟测试的 "确定性" 升级
  问题: smoke_test.sh 依赖 sleep 2 等待模拟器启动，这是 CI 其中的Flaky Test (不稳定测试) 之源。机器负载高时，2秒可能不够。
  最佳实践: Wait-for-Port / Health Check。
   * 行动:
       1. 修改脚本，使用轮询检查 /tmp/edge_smoke_xxx.sock 是否存在且可连接，替代硬编码的 sleep。
       2. 在冒烟测试中增加对产生的 SQLite 数据库的完整性校验 (sqlite3 .dump)，确保 WAL 日志正确合并。

  ✅ 4. 制品库与 Release 自动化
  问题: 构建产物仅作为 Artifact 上传，无法直接被部署工具使用。
  最佳实践: Semantic Release。
   * 行动:
       1. 当 main 分支构建成功且 Tag 匹配 v* 时，自动创建 GitHub Release。
       2. 自动生成 Changelog。
       3. 将 .deb 包推送到私有 Apt 仓库或 S3，供 OTA 服务拉取。

  ✅ 5. 缓存策略精细化
  问题: 目前只缓存了 Go 依赖。C/C++ 的编译对象 (.o) 未缓存。
  最佳实践: Ccache。
   * 行动:
       1. 在 Docker 镜像中预装 ccache。
       2. 在 CMake 中配置 -DCMAKE_C_COMPILER_LAUNCHER=ccache。
       3. 配置 GitHub Actions Cache 存储 ~/.ccache 目录。
       4. 这能将增量构建时间从 5 分钟缩短到 30 秒。

  ---

  3. 具体实施代码 (Snippet)

  A. 优化冒烟测试 (消除 sleep)

   1 # scripts/smoke_test.sh
   2 # 替换 sleep 2
   3 echo "Waiting for simulator socket..."
   4 for i in {1..50}; do
   5     if [ -S "$VPORT" ]; then break; fi
   6     usleep 100000 # 100ms
   7 done
   8 if [ ! -S "$VPORT" ]; then echo "Simulator failed to start"; exit 1; fi

  B. 集成 Ccache (CI 配置)

    1 # .github/workflows/ci.yml
    2     - name: Ccache
    3       uses: hendrikmuhs/ccache-action@v1.2
    4       with:
    5         key: ${{ matrix.config.os }}-${{ matrix.config.target }}
    6     
    7     - name: Configure CMake
    8       run: |
    9         cmake .. \
   10           -DCMAKE_C_COMPILER_LAUNCHER=ccache \
   11           ...

  C. 静态分析 (Clang-Tidy)

   1     - name: Static Analysis
   2       if: matrix.config.target == 'x86_64'
   3       run: |
   4         # 生成 compile_commands.json
   5         cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   6         # 并行运行分析
   7         run-clang-tidy -p build -j $(nproc) -header-filter='src/.*' -checks='-*,cert-*,bugprone-*'

  总结
  EdgeOS 目前的 CI 已经是 Level 3 (Automated) 水平。通过引入 Docker 环境、Ccache 加速、SAST 扫描，可以晋升为 Level 4 (Optimized & Secure)，达到汽车级软件交付标准。

  ✦ 基于对 docker/Dockerfile、cmake/*.cmake 以及整体代码结构的分析，我对 EdgeOS 在构建和容器化方面的实践有如下深度认知和优化建议。

  ---

  📦 Docker & CMake 体系深度审计报告

  1. 现状评估 (Current State)

  CMake 构建系统
  EdgeOS 的 CMake 体系设计得非常模块化和现代化，远超一般嵌入式项目：
   * 亮点：
       * 混合语言构建：通过 BuildContractCompiler.cmake 优雅地在 CMake 中集成 Go 编译，并实现了源码级依赖追踪 (DEPENDS ${GO_SOURCES})。
       * 全栈构建：通过 WebBuild.cmake 集成了前端 npm 构建流，实现了“一个 make 指令构建全栈应用”。
       * 功能栅极：通过 Sanitizers.cmake 和 CodeCoverage.cmake 实现了 Debug/Release 模式的特性切换。
       * 打包规范：EdgeInstall.cmake 和主 CMakeLists.txt 中的 CPack 配置非常专业，支持基于组件 (COMPONENT) 的 DEB 打包。

  Docker 容器化
  目前的 Dockerfile 是一个标准的构建环境镜像 (Builder Image)：
   * 亮点：
       * 基于 debian:bookworm-slim，体积控制得当。
       * 集成了交叉编译链 (gcc-aarch64-linux-gnu)。
       * 创建了非 root 用户 builder，符合安全最佳实践。

  ---

  2. 深度优化建议 (Optimization Opportunities)

  尽管现状已经很不错，但从DevOps 效能和工业级交付的角度，还有巨大的挖掘空间。

  🚀 优化方向 1：Docker 多阶段构建与 Layer 缓存 (Docker Optimization)

  现状问题：
  Dockerfile 将所有工具安装在一个层中，且没有利用构建缓存。如果修改了 docker/toolchains，会导致整个镜像重打。而且，目前的 Dockerfile 缺少 Go 环境，会导致 contract-compiler 构建失败。

  优化方案：
   1. 补充 Go 环境：必须安装 golang，否则 BuildContractCompiler.cmake 里的 find_program(GO_EXECUTABLE go) 会失败。
   2. 分层策略：将变动频率低的（apt install）和变动频率高的（toolchains copy）分开。

    1 # 优化后的 Dockerfile 片段
    2 FROM debian:bookworm-slim
    3 
    4 # 1. 基础工具层 (变动极少)
    5 RUN apt-get update && apt-get install -y --no-install-recommends \
    6     build-essential cmake ninja-build git golang \
    7     gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    8     && rm -rf /var/lib/apt/lists/*
    9 
   10 # 2. 预热层 (可选：预下载 npm 依赖或 go mod)
   11 # ...
   12 
   13 # 3. 配置层
   14 COPY --chown=builder:builder docker/toolchains/ /home/builder/toolchains/

  ⚡ 优化方向 2：CMake 的 "密封性" 与缓存 (Hermetic Build)

  现状问题：
  WebBuild.cmake 直接调用 npm install。这有两个问题：
   1. 网络依赖：CI 环境如果断网，构建直接挂。
   2. 重复劳动：每次 clean build 都要重新下载几百 MB 的 node_modules。

  优化方案：
   1. Yarn/Pnpm 离线缓存：在 Docker 镜像中预挂载 npm 缓存，或者在项目中提交 yarn.lock 和 .yarn/cache。
   2. CMake 代理变量：允许通过 CMake 变量传递 npm 的代理或 registry 设置。

  🛡️ 优化方向 3：构建产物的 "元数据注入"

  现状问题：
  目前的 EdgeInstall.cmake 只是简单拷贝文件。上线后，运维人员拿到一个二进制，很难知道它是哪个 Git Commit 构建的，也不包含符号表以便调试。

  优化方案：
   1. 注入 Git Hash：在 CMakeLists.txt 中获取 git rev-parse HEAD，并通过 target_compile_definitions 注入到 C 代码中（目前代码已做，点赞）。
   2. DebugInfo 分离：在 Release 模式下，使用 objcopy --only-keep-debug 将符号表分离为单独的 .dbgsym 文件，随发布包一起归档，便于产生 Core Dump 后进行回溯。

  🔧 优化方向 4：交叉编译的 "真实性"

  现状问题：
  目前 CI 中的 QEMU 测试 (tests/test_*) 虽然能运行，但无法模拟真实的硬件外设（如 GPIO, CAN, RS485）。

  优化方案：
  Mock HAL Injection。
  在 CMake 中定义 -DEDGE_SIMULATION 宏。当此宏开启时，底层驱动（如 driver_modbus.c）不操作真实 /dev/ttyUSB0，而是连接到一个 虚拟总线模拟器 (Bus Orchestrator Stub)。这允许在 x86 Docker 中完整模拟 "读取电表 -> 触发规则 -> 控制逆变器" 的全业务闭环，而无需 QEMU。

  ---

  📝 立即行动清单 (Action Plan)

   1. 紧急修复：在 Dockerfile 中添加 golang 安装，否则 CI 必挂。
   2. 性能提升：在 Dockerfile 中安装 ccache，并在 CMakeLists.txt 中配置 find_program(CCACHE_PROGRAM ccache) 自动启用。
   3. 规范化：修改 WebBuild.cmake，增加对 node_modules 是否存在的检查，避免每次都运行 npm install，只在 package.json 变更时运行。

   1 # WebBuild.cmake 优化片段
   2 add_custom_command(
   3     OUTPUT "${WEB_STAMP}"
   4     # 智能判断：仅当 package-lock.json 比 node_modules 新时才 install
   5     COMMAND sh -c "if [ '${WEB_SRC_DIR}/package.json' -nt '${WEB_SRC_DIR}/node_modules' ]; then npm install; fi"
   6     COMMAND npm run build
   7     ...
   8 )
