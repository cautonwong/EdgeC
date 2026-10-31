# cmake/WebBuild.cmake

# ---------------------------------------------------------------------------
# Web 依赖安装是**全局一次**的事，而且不能和构建并行。
#
# npm workspaces 把依赖统一装在 `web/node_modules`（core + products/* 共用）。
# 原来每个产品的构建命令里各带一句 `if [ ! -d node_modules ]; then npm install; fi`，
# 于是 ninja 并行跑多个产品时会出现"一个还在装、另一个已经在 vite build"：
#
#   - Cannot find module './internal/bin-trie-flags.js'      （读到半装的包）
#   - ENOTEMPTY: rmdir .../node_modules/@vue/devtools-api/lib （装的时候被读）
#   - sh: 1: vite: not found
#
# 这些看起来像"npm 抖动"，其实是**竞态**——重跑有时过、有时不过，正是竞态的典型表现。
#
# 修法：把安装抽成一个共享 stamp 目标，所有产品的 web 构建都依赖它。
# 装完之后各产品的 `vite build` 只读 node_modules，可以安全并行。
# ---------------------------------------------------------------------------
set(WEB_WORKSPACE_DIR "${CMAKE_SOURCE_DIR}/web")
set(WEB_NPM_STAMP "${CMAKE_BINARY_DIR}/web_npm_install.stamp")

if(NOT TARGET web_npm_install)
    add_custom_command(
        OUTPUT "${WEB_NPM_STAMP}"
        # 在 workspace 根装一次；aarch64 的 rollup/esbuild 可选依赖同样只补一次
        COMMAND sh -c "if [ ! -d node_modules ]; then npm install --ignore-scripts; fi; if [ \"$(uname -m)\" = \"aarch64\" ]; then if [ ! -d node_modules/@rollup/rollup-linux-arm64-gnu ]; then npm install --save-optional @rollup/rollup-linux-arm64-gnu --ignore-scripts || true; fi; if [ ! -d node_modules/@esbuild/linux-arm64 ]; then npm install --save-optional @esbuild/linux-arm64 --ignore-scripts || true; fi; fi"
        COMMAND ${CMAKE_COMMAND} -E touch "${WEB_NPM_STAMP}"
        WORKING_DIRECTORY "${WEB_WORKSPACE_DIR}"
        COMMENT "Installing web workspace dependencies (once)..."
        VERBATIM
    )
    add_custom_target(web_npm_install DEPENDS "${WEB_NPM_STAMP}")
endif()

function(add_web_product PRODUCT_NAME WEB_SRC_DIR)
    set(WEB_BUILD_DIR "${CMAKE_BINARY_DIR}/products/${PRODUCT_NAME}/www")
    set(WEB_STAMP "${CMAKE_CURRENT_BINARY_DIR}/web_build_${PRODUCT_NAME}.stamp")

    message(STATUS "Configuring Web UI for ${PRODUCT_NAME} from ${WEB_SRC_DIR}")

    # 获取 Web 相关的所有源码文件，用于精确触发构建
    file(GLOB_RECURSE WEB_CORE_SOURCES "${CMAKE_SOURCE_DIR}/web/core/src/*")
    file(GLOB_RECURSE PRODUCT_WEB_SOURCES "${WEB_SRC_DIR}/src/*")
    set(WEB_DEPS
        ${WEB_CORE_SOURCES}
        ${PRODUCT_WEB_SOURCES}
        "${WEB_SRC_DIR}/package.json"
        "${WEB_SRC_DIR}/vite.config.ts"
    )

    # 获取生成文件的路径
    set(GEN_DIR "${CMAKE_BINARY_DIR}/products/${PRODUCT_NAME}/generated")

    # 构建只做 vite build（依赖已由 web_npm_install 装好，这里只读）
    add_custom_command(
        OUTPUT "${WEB_STAMP}"
        COMMAND ${CMAKE_COMMAND} -E env VITE_CONTRACT_GEN_DIR=${GEN_DIR} npm run build
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WEB_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${WEB_SRC_DIR}/dist" "${WEB_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${WEB_STAMP}"
        WORKING_DIRECTORY "${WEB_SRC_DIR}"
        DEPENDS ${WEB_DEPS} "${WEB_NPM_STAMP}"
        COMMENT "Building Web UI for ${PRODUCT_NAME}..."
        VERBATIM
    )

    # 伪目标供产品二进制文件依赖
    add_custom_target(web_ui_${PRODUCT_NAME} ALL DEPENDS "${WEB_STAMP}")
    add_dependencies(web_ui_${PRODUCT_NAME} web_npm_install)

    # 安装时把静态资源放入 www/<product>
    string(TOUPPER ${PRODUCT_NAME} PROD_UPPER)
    install(DIRECTORY "${WEB_BUILD_DIR}/"
            DESTINATION "www/${PRODUCT_NAME}"
            COMPONENT "${PROD_UPPER}")
endfunction()
