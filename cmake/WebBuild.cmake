# cmake/WebBuild.cmake

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

    # 1. 自动执行 npm install 和 build
    # 通过环境变量传递生成目录，让 Vite 能够找到 tags_type.ts
    add_custom_command(
        OUTPUT "${WEB_STAMP}"
        COMMAND npm install
        COMMAND ${CMAKE_COMMAND} -E env VITE_CONTRACT_GEN_DIR=${GEN_DIR} npm run build
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WEB_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${WEB_SRC_DIR}/dist" "${WEB_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E touch "${WEB_STAMP}"
        WORKING_DIRECTORY "${WEB_SRC_DIR}"
        DEPENDS ${WEB_DEPS} "${GEN_DIR}/tags_type.ts"
        COMMENT "Building Web UI for ${PRODUCT_NAME} (with generated contracts)..."
        VERBATIM
    )

    # 2. 创建一个伪目标供产品二进制文件依赖
    add_custom_target(web_ui_${PRODUCT_NAME} ALL DEPENDS "${WEB_STAMP}")

    # 3. 在安装时，将静态资源放入 www/<product>
    string(TOUPPER ${PRODUCT_NAME} PROD_UPPER)
    install(DIRECTORY "${WEB_BUILD_DIR}/" 
            DESTINATION "www/${PRODUCT_NAME}"
            COMPONENT "${PROD_UPPER}")
endfunction()
