# cmake/WebBuild.cmake
#
# ---------------------------------------------------------------------------
# WebUI 构建接线（v1.6 §4.4 / §11.1）
#
# 三件事必须按顺序发生，否则"构建产物"和"构建输入"会悄悄不一致：
#   1. `npm install` 全局一次（npm workspaces 共用 node_modules，并行构建会打架）；
#   2. 每个产品先跑 `tooling registry` 生成静态 Registry（这一步在产品的 npm build
#      脚本里，失败即中止，不会带着过期 Registry 去 vite build）；
#   3. `vite build` 产出 dist，再整体拷到 `<build>/products/<p>/www`。
#
# 依赖列表刻意包含 manifest / release-scope / 权限目录 / OpenAPI / tooling：
# 改一条权限或关一个功能开关，必须触发前端重建——否则固件里会留着旧路由。
# ---------------------------------------------------------------------------
set(WEB_WORKSPACE_DIR "${CMAKE_SOURCE_DIR}/web")
set(WEB_NPM_STAMP "${CMAKE_BINARY_DIR}/web_npm_install.stamp")

if(NOT TARGET web_npm_install)
    add_custom_command(
        OUTPUT "${WEB_NPM_STAMP}"
        COMMAND sh -c "if [ ! -d node_modules ]; then npm install --ignore-scripts; fi; if [ \"$(uname -m)\" = \"aarch64\" ]; then if [ ! -d node_modules/@rollup/rollup-linux-arm64-gnu ]; then npm install --save-optional @rollup/rollup-linux-arm64-gnu --ignore-scripts || true; fi; if [ ! -d node_modules/@esbuild/linux-arm64 ]; then npm install --save-optional @esbuild/linux-arm64 --ignore-scripts || true; fi; fi"
        COMMAND ${CMAKE_COMMAND} -E touch "${WEB_NPM_STAMP}"
        WORKING_DIRECTORY "${WEB_WORKSPACE_DIR}"
        COMMENT "Installing web workspace dependencies (once)..."
        VERBATIM
    )
    add_custom_target(web_npm_install DEPENDS "${WEB_NPM_STAMP}")
endif()

# 构建输入：任何一处变更都必须触发重建
set(WEB_CONTRACT_INPUTS
    "${WEB_WORKSPACE_DIR}/core/modules.manifest.yaml"
    "${WEB_WORKSPACE_DIR}/permissions/catalog.yaml"
    "${WEB_WORKSPACE_DIR}/permissions/role-presets.yaml"
    "${WEB_WORKSPACE_DIR}/openapi/core.yaml"
)
file(GLOB_RECURSE WEB_TOOLING_SOURCES "${WEB_WORKSPACE_DIR}/tooling/*.mjs")

function(add_web_product PRODUCT_NAME WEB_SRC_DIR)
    set(WEB_BUILD_DIR "${CMAKE_BINARY_DIR}/products/${PRODUCT_NAME}/www")
    set(WEB_STAMP "${CMAKE_CURRENT_BINARY_DIR}/web_build_${PRODUCT_NAME}.stamp")

    message(STATUS "Configuring Web UI for ${PRODUCT_NAME} from ${WEB_SRC_DIR}")

    file(GLOB_RECURSE WEB_CORE_SOURCES "${WEB_WORKSPACE_DIR}/core/src/*")
    file(GLOB_RECURSE PRODUCT_WEB_SOURCES
        "${WEB_SRC_DIR}/src/*"
        "${WEB_SRC_DIR}/pages/*"
        "${WEB_SRC_DIR}/components/*"
    )
    set(WEB_DEPS
        ${WEB_CORE_SOURCES}
        ${PRODUCT_WEB_SOURCES}
        ${WEB_CONTRACT_INPUTS}
        ${WEB_TOOLING_SOURCES}
        "${WEB_SRC_DIR}/package.json"
        "${WEB_SRC_DIR}/vite.config.ts"
        "${WEB_SRC_DIR}/index.html"
    )
    # 构建清单类文件只在存在时才算依赖：smart_box 还没接入平台，没有 product.yaml，
    # 把它写成无条件 DEPENDS 会让 ninja 直接报 "missing and no known rule to make it"。
    foreach(manifest_file product.yaml release-scope.yaml openapi.extension.yaml)
        if(EXISTS "${WEB_SRC_DIR}/${manifest_file}")
            list(APPEND WEB_DEPS "${WEB_SRC_DIR}/${manifest_file}")
        endif()
    endforeach()

    set(GEN_DIR "${CMAKE_BINARY_DIR}/products/${PRODUCT_NAME}/generated")

    # 只做 vite build：Registry 由产品 build 脚本里的 tooling 生成（失败即中止）
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

    add_custom_target(web_ui_${PRODUCT_NAME} ALL DEPENDS "${WEB_STAMP}")
    add_dependencies(web_ui_${PRODUCT_NAME} web_npm_install)

    string(TOUPPER ${PRODUCT_NAME} PROD_UPPER)
    install(DIRECTORY "${WEB_BUILD_DIR}/"
            DESTINATION "www/${PRODUCT_NAME}"
            COMPONENT "${PROD_UPPER}")
endfunction()
