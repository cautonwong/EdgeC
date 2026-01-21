构建架构方案：

  ---

  1. 架构目标：差异化定制与统一输出

  我们面临的挑战是：
   * 产品 A (IEMS)：需要 CLI UI（调试用），可能还需要本地 HDMI 屏幕 UI。
   * 产品 B (DCU)：只需要 CLI UI。
   * 产品 C (Sensor)：完全不需要 UI。

  解决方案：“产品级 UI 描述文件” + “CMake 动态注入”。

  ---

  2. 目录结构调整

    1 /home/EdgeC/
    2 ├── src/
    3 │   ├── ui/                 # [核心] UI 框架 (Page, Stack, Event)
    4 │   │   ├── drivers/        # [驱动] ncurses, ssd1306, framebuffer
    5 ├── products/               # [产品定义]
    6 │   ├── iems/
    7 │   │   ├── ui_config/      # [新增] 该产品的 UI 定制
    8 │   │   │   ├── dashboard.c # 定制页面
    9 │   │   │   └── ui_conf.cmake # 定制构建参数
   10 │   │   ├── CMakeLists.txt  # 产品主构建

  ---

  3. CMake 构建逻辑深度解构

  A. 在 cmd/iems/CMakeLists.txt 中集成 UI

  我们不需要独立的脚本，而是利用 add_executable 的灵活性。

    1 # cmd/iems/CMakeLists.txt
    2 
    3 set(PROD_NAME iems)
    4 
    5 # 1. 检查是否开启 CLI-UI
    6 # 这个变量可以在 cmake 命令行通过 -DENABLE_UI=ON 传入
    7 if(ENABLE_UI)
    8     message(STATUS "[${PROD_NAME}] Building CLI-UI Companion Process...")
    9 
   10     # 2. 收集产品特有的 UI 源码
   11     file(GLOB PROD_UI_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.c")
   12 
   13     # 3. 定义 CLI 进程目标
   14     add_executable(${PROD_NAME}-cli
   15         ${CMAKE_SOURCE_DIR}/src/ui/core/cli_main_entry.c # 通用入口
   16         ${PROD_UI_SRCS}                                  # 产品定制页面
   17     )
   18 
   19     # 4. 链接依赖 (只链接数据和 UI 框架，不链接业务)
   20     target_link_libraries(${PROD_NAME}-cli PRIVATE
   21         edge_ui_framework      # 框架
   22         contract_${PROD_NAME}  # 契约 (TagID)
   23         edge_core              # DataHub Client
   24         ncurses
   25     )
   26 
   27     # 5. [关键点] 统一输出目录
   28     # 确保 iems-cli 和 iems 在同一个 bin 目录下
   29     set_target_properties(${PROD_NAME}-cli PROPERTIES
   30         RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/products/${PROD_NAME}/bin"
   31     )
   32 
   33     # 6. 安装规则
   34     install(TARGETS ${PROD_NAME}-cli DESTINATION bin)
   35 endif()

  ---

  4. 如何处理差异化需求？

  场景 1：不同产品有不同的页面
   * 实现：每个产品目录下都有 src/ui/，里面放着 page_dashboard.c, page_settings.c。
   * CMake：add_executable 会自动编译该目录下的文件。
   * 代码：cli_main_entry.c（通用入口）定义一个 weak 函数 ui_product_init(svc)。
       * 在产品的 page_dashboard.c 里实现这个函数，把自己的页面 push 进栈。

  场景 2：不同产品用不同的驱动
   * IEMS：用 ncurses。
   * HMI：用 framebuffer。
   * 实现：在产品的 CMakeLists.txt 里定义宏：
   1     target_compile_definitions(${PROD_NAME}-cli PRIVATE UI_DRIVER_NCURSES)
      框架层 ui_service.c 根据宏来决定初始化哪个驱动。

  ---

  5. 最终的交付形态

  当你运行 ./build.sh（或者 cmake --build .）后，build/products/iems/bin/ 目录下会出现两个文件：

   1. `iems`：守护进程，负责采集、控制、上云。
   2. `iems-cli`：UI 进程，负责给人看。

  使用体验：
   * 部署时，把这俩都拷走。
   * 平时只运行 iems。
   * 调试时，SSH 进去输入 iems-cli，界面瞬间弹出，连接到正在运行的 iems，显示实时数据。退出 iems-cli，iems 毫发无损。

  总结
   1. CMake 脚本：是的，直接写在 CMakeLists.txt 里是正解，利用 RUNTIME_OUTPUT_DIRECTORY 强行统一输出路径。
   2. 差异化：通过“通用框架 + 产品侧注入源码/宏”的方式解决。
   3. 统一目录：这是标准做法，方便打包（CPack）和部署。

  这套方案既满足了“多产品差异”，又保持了架构的“统一与整洁”。